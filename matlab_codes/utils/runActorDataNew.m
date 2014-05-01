function runActorDataNew(episode_name)

% get all episode related file names
ep_files = getEpisodeFiles(episode_name);

min_conf = 0;

% create cache dir if not present
checkAndMakeDir(ep_files.cache_dir_episode);


getActorDataNew(ep_files.cacheFile, ep_files.faceMatFile, ...
              ep_files.script_srt_align_file, min_conf, ep_files.train_file);
