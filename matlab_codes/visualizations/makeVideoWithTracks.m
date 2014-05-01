%% Show the video with tracks

function makeVideoWithTracks(tracks_full, face_ids, imgDir, dump_dir)

if isempty(face_ids)
  fprintf('No tracks\n');
  return;
end

tracks = cell(numel(face_ids), 1);

for i = 1 : numel(face_ids)
  tracks{i} = zeros(numel(tracks_full(face_ids(i)).faces), 5);
  try
    tracks{i}(:, 1:4) = cat(1, tracks_full(face_ids(i)).faces.bbox);
  catch
    fprintf('bug here\n');
    keyboard;
  end
  tracks{i}(:, 5)   = tracks_full(face_ids(i)).faces.frames;  
end

showActorTracksOnFrames(imgDir, tracks, 0, dump_dir, face_ids);


