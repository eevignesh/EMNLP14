%%
% @brief: get the scene actor-description bag labels and tracks for a video. This version also
%         includes scene numbers.

% @paramters:
%   - cacheFile: the cache file with video frame, time information and track relations
%   - script_srt_align_file: the text file with script and srt alignments
%   - faceMatFile: the out_face_tracks_matfile that contains face tracking information
%   - min_conf: conf. for pittpatt face-detector (use 0)
%   - train_file: to store final output train_data

% @output:
%   - train_data: N element struct vector, N is the number of speaker entities in the faces_align_file
%                 train_data(i).names - the complete description associated with the track
%                 train_data(i).tracks - the track-data associated with this bag in num_tracks cell array
%                 train_data(i).tracks{j}(k,:) = [left_x left_y right_x right_y ... frame_num] 
%                           gives the bounding-box of the k^{th} face in the j^{th} track, where 
%                           last entry is the frame-num
%                 train_data(i).face_ids(j) = the face ids which were used to initialize the j^{th} 
%                           track

% sample_data:
% train_file - /scail/u/vigneshr/CVPR2014/datasets/tv_data_shows/castle_1x09/train_data_actor_full.mat
% to just load the train_file: use getActorTracks([],[],[],[],train_file);


function train_data = getActorDataNew(cacheFile, faceMatFile, script_srt_align_file, min_conf, train_file)

try
  load(train_file);
  %error('bah');
catch

  load(cacheFile);
  speaker_bags = getActorLabels(script_srt_align_file);

  if ischar(faceMatFile)
    load(faceMatFile);
  else
    tracks_full = faceMatFile;
  end

  timeinfo = zeros(numel(trackInfo),2);
  confinfo = zeros(numel(trackInfo),1);

  for i = 1:numel(trackInfo)
    timeinfo(i,:) = trackInfo(i).timeinfo;
    confinfo(i) = trackInfo(i).confvalue;
  end

  ctr = 1;
  for i = 1:numel(speaker_bags)

    fprintf('train_data %d/%d\n', i, numel(speaker_bags));

    train_data(ctr).names = {};
    train_data(ctr).tracks = {};
    train_data(ctr).stid = -1;

    valid_ctrs = find((timeinfo(:,1)<=speaker_bags(i).time_end) & (timeinfo(:,2)>=speaker_bags(i).time_beg) & confinfo >= min_conf);

    if ~isempty(valid_ctrs)
      j_ctr = 1;
      for j = 1:numel(valid_ctrs)
        train_data(ctr).tracks{j_ctr}  = zeros(numel(tracks_full(valid_ctrs(j)).faces),6);

        for k = 1:numel(tracks_full(valid_ctrs(j)).faces)
          train_data(ctr).tracks{j_ctr}(k,:) = [tracks_full(valid_ctrs(j)).faces(k).bbox ...
                                           tracks_full(valid_ctrs(j)).faces(k).conf ...
                                           tracks_full(valid_ctrs(j)).faces(k).frames(k)];
        end

        train_data(ctr).face_ids(j_ctr) = valid_ctrs(j_ctr);
        j_ctr = j_ctr + 1;
      end

      if (j_ctr > 1)
        train_data(ctr).names = speaker_bags(i).description;
        train_data(ctr).scene = speaker_bags(i).scene;
        train_data(ctr).stid  = i-1;
        ctr = ctr + 1;
      end
      %keyboard;
    end
  end

  save(train_file, 'train_data');
end
