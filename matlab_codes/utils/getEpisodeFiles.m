%% return the different file names which store data related to an episode
%
%  params
%   episode_name - the episode name

%  output
%   episode_files - a structure containing the file names of different episode related files

function episode_files = getEpisodeFiles(episode_name)

globals;
episode_files.cacheFile = ...
  [tv_data_dir_eccv episode_name '/trackInfo_v2.mat'];
episode_files.faceMatFile = ...
  [tv_data_dir_eccv episode_name '/faces/out_face_tracks_matfile.mat'];
episode_files.script_srt_align_file = ...
  [tv_data_dir_new episode_name '/alignments_clean/script_srt.align'];
episode_files.cache_dir_episode = sprintf(cache_dir, episode_name);

checkAndMakeDir(episode_files.cache_dir_episode);
episode_files.train_file = [episode_files.cache_dir_episode '/train_data_actor_full_new.mat'];
episode_files.visual_dir = [tv_data_dir_new episode_name '/visualizations/'];

