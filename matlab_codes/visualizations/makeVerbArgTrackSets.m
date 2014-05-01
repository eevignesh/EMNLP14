%% map the vers-arguments from the SRL-coref ann files to 
%  the corresponding track alignments from script_srt_align file
%  finally generate set of video clips for each verg-argument pair
% showing the set of tracks as possible candidates

% params
%   episode_name
%   brat_dir         - the bart annotation directory which contains the *.ann files of each scene
%   output_dir       - the output directory to store the output files in

% (files used)
%   train_file            - the track mapping file obtained by running getActorData.m

function [verb_data, scene_txt] = makeVerbArgTrackSets(episode_name, output_dir)

% get all episode related file names
addpath('../utils');
addpath('../utils/addTextToImage/');
globals;
ep_files = getEpisodeFiles(episode_name);
%globals;

imgDir = sprintf(imgDir, episode_name);
dump_dir = sprintf(dumpDataDir, episode_name);
video_scene_dir = sprintf(videoSceneDir, episode_name);
brat_dir        = sprintf(bratDir, episode_name);
faceMatFile = sprintf(faceMatFile, episode_name);

checkAndMakeDir(video_scene_dir);
checkAndMakeDir(dump_dir);

fprintf('Loading matfile of face-tracks \n');
tic;
load(faceMatFile);
toc;

if isempty(output_dir)
  output_dir   = [ep_files.visual_dir '/VATS/'];
  checkAndMakeDir(ep_files.visual_dir);
  checkAndMakeDir(output_dir);
end

actor_data = getActorDataNew(ep_files.cacheFile, ep_files.faceMatFile, ...
        ep_files.script_srt_align_file, 0, ep_files.train_file);


% parse the annotation file
brat_anno_filelist = dir([brat_dir '/*.ann']);

% get the data for each verb from each scene
%keyboard;

for i = 1:numel(brat_anno_filelist)
  brat_anno_file = [brat_dir sprintf('/scene_%04d.ann', i)];
  brat_text_file = [brat_dir sprintf('/scene_%04d.txt', i)];
  brat_data   = parseBratAnnoFile(brat_anno_file, brat_text_file);  

  if isempty(brat_data.tokens)
    continue;
  end

  actor_data_scenes = actor_data(cat(2, actor_data.scene)==i);
  
  % inti verb data structure
  scene_txt{i} = brat_data.lines;
  % get all verbs
  token_names = {brat_data.tokens.name};
  verb_tokens = find(strcmp(token_names, 'Verb'));
  verb_data{i} = repmat(struct('verb', [], 'args', [], 'face_ids', []), [numel(verb_tokens) 1]);
  num_aligned_verbs = 0;

  scene_face_ids = [];

  for j = 1:numel(verb_tokens)    
   
    if isempty(brat_data.relations)
      continue;
    end


    verb_relations  = find(cat(2, brat_data.relations.source_token)==verb_tokens(j));
    non_empty_rels  = find(arrayfun(@(x)~isempty(x.source_token), brat_data.relations));
    verb_relations  = non_empty_rels(verb_relations);

    verb_beg_char = brat_data.tokens(verb_tokens(j)).beg_char;
    verb_end_char = brat_data.tokens(verb_tokens(j)).end_char;
    
    verb_data{i}(j).verb.name = brat_data.tokens(verb_tokens(j)).original_text;
    verb_data{i}(j).verb.span = [verb_beg_char verb_end_char];

    for k = 1:numel(verb_relations)
      
      % form a short phrase containing the words between the verb and argument
      try
        arg_token_id = brat_data.relations(verb_relations(k)).target_token;
        arg_beg_char = brat_data.tokens(arg_token_id).beg_char;
      catch
        keyboard;
      end
      arg_end_char = brat_data.tokens(arg_token_id).end_char;
      phrase_beg_char = min(arg_beg_char+1, verb_beg_char+1);
      phrase_end_char = max(arg_end_char, verb_end_char);
      verb_arg_phrase =  brat_data.lines(phrase_beg_char:phrase_end_char);
      

      % store the argument data for the verbs
      verb_data{i}(j).args(k).text = brat_data.tokens(arg_token_id).original_text;
      verb_data{i}(j).args(k).type = brat_data.relations(verb_relations(k)).name;
      verb_data{i}(j).args(k).span = [arg_beg_char arg_end_char];
      verb_data{i}(j).args(k).id   = arg_token_id;

      % Check if the actor data descriptions contain the verb-arg-phrase
      for l = 1:numel(actor_data_scenes)
        if isempty(actor_data_scenes(l).names)
          continue;
        end
        if strfind(actor_data_scenes(l).names{1}, verb_arg_phrase)
          verb_data{i}(j).face_ids = actor_data_scenes(l).face_ids;
          scene_face_ids = [scene_face_ids, actor_data_scenes(l).face_ids];
        %else
        %  fprintf('Could not align\n');
        %  keyboard;
        end
      end
      
    end
    if ~isempty(verb_data{i}(j).face_ids)
      num_aligned_verbs = num_aligned_verbs + 1;
    end
  end
  
  fprintf('<Scene %03d>: Weakly aligned %d/%d verbs\n', i, num_aligned_verbs, numel(verb_tokens));
  
  annotated_text = scene_txt{i}; 
  

  % write the newly annotated text and also mark the tracks
  output_html_file = sprintf('%s/scene_%04d.html', output_dir, i);
  writeHtmlData(annotated_text, verb_data{i}, output_html_file, episode_name, i);
  

  if isempty(scene_face_ids)
    continue;
  end

  min_face_id = min(scene_face_ids);
  max_face_id = max(scene_face_ids);
  makeVideoWithTracks(tracks_full, min_face_id:max_face_id, imgDir, dump_dir);
  
  try
    outName = [video_scene_dir sprintf('scene_%04d.ogg', i)];
    vid_cmd = ['ffmpeg -y -r 20 -i ' dump_dir ...
      '/%5d.jpg -b 1500K -vcodec libtheora -acodec vorbis ' outName];
    fprintf('Executing %s\n', vid_cmd);
    %keyboard;

    unix(vid_cmd);
    unix(['rm ' dump_dir '/*']);
    %keyboard;
    unix(sprintf('ln -s %s %s/scene_%04d.ogg', outName, output_dir, i));
  catch
    fprintf('Writing of video file failed\n');
  end

  %keyboard;
end


%%

% params
%   annotated_text - the first annotated text component to be written
%   verb_data      -
%      verb.name   - name of the verb
%      verb.span   - the character span of the verb
%      args.id
%      args.type
%      args.text
%      face_ids

function writeHtmlData(annotated_text, verb_data, file_name, episode_name, scene_id)

fid = fopen(file_name, 'w');
fprintf(fid, '<html>\n');
fprintf(fid, '<head> <link rel="stylesheet" href="../styles/style.css"> </head>\n');

fprintf(fid, '\t<body>\n');
fprintf(fid, '\t\t<h1>Annotated Text </h1>\n');
fprintf(fid, '\t\t<p align="justify">\n');
fprintf(fid, '\t\t%s\n\n\n', annotated_text);
fprintf(fid, '\t\t</p>\n\n');


fprintf(fid, '\t\t<br><br>\n');
fprintf(fid, '\t\t<p align="center"\n>');
fprintf(fid, '\t\t<video width=\"560\" height=\"340\" controls>\n');
fprintf(fid, '\t\t<source src="scene_%04d.ogg" type=''video/ogg; codecs="theora, vorbis"''>\n', scene_id);
fprintf(fid, '\t\t</video>\n\n');
fprintf(fid, '\t\t</p>\n\n');

fprintf(fid, '\t\t<p align="center"\n>');
fprintf(fid, '\t\t<a href="scene_%04d.html" style="margin-right: 30px">[PREV]</a>\n', min(1,scene_id-1));
fprintf(fid, '\t\t<a href="http://visionlab11.stanford.edu:8080/brat/#/%s/scene_%04d" target="_blank" style="margin-right: 30px"> BRAT </a>\n', episode_name, scene_id);
fprintf(fid, '\t\t<a href="scene_%04d.html">[NEXT]</a>\n', scene_id+1);
fprintf(fid, '\t\t</p>\n\n');

fprintf(fid, '\t\t\t<br><br><br>\n');

for i = 1:numel(verb_data)

  if isempty(verb_data(i).verb)
    continue;
  end

  fprintf(fid, '\t\t<div style="float: left; margin-right: 30px; margin-bottom: 20px;"/>\n');
  fprintf(fid, '\t\t<table>\n');
  fprintf(fid, '\t\t<tr>\n');
  fprintf(fid, '\t\t<th> <b> VERB </b>: </th> <th> %s </th>\n', verb_data(i).verb.name);
  fprintf(fid, '\t\t</tr>\n');
  
  % print the arguments
  fprintf(fid, '\t\t<tr> <th> <b> ARGS </b>: </th> ');
  for j = 1:numel(verb_data(i).args)
    fprintf(fid, '<th> %s: %s </th>\n', verb_data(i).args(j).type, verb_data(i).args(j).text);
  end
  fprintf(fid, '\t\t</tr>\n');
    
  % print the tracks
  fprintf(fid, '\t\t<tr> <th> <b> TRACKS </b> </th> ');
  fprintf(fid, '<th> ');
  for j = 1:numel(verb_data(i).face_ids)
    fprintf(fid, '%d', verb_data(i).face_ids(j));
    if j < numel(verb_data(i).face_ids)
      fprintf(fid, ', ');
    end
  end
  fprintf(fid, '</th>\n');
  fprintf(fid, '\t\t</table>\n\t\t</div>\n');
  %fprintf(fid, '\t\t<br><br>\n');
end

fprintf(fid, '\t</body>\n');
fprintf(fid, '</html>');
fclose(fid);

