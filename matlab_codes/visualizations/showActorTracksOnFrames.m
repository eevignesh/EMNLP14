%%
% @brief: visualize tracks in a bag of tracks corresponding to a scene
%
% @paramters:
%   - imgDir: the folder containing all the video frames
%   - tracks: a cell array of tracks within a scene-bag, tracks{i} is the i^th track in the bag
%   - isExpand: set to true if you want to visualize the entire upper-body and not the face

function showActorTracksOnFrames(imgDir, tracks, isExpand, dump_dir, face_ids)

time_buf = 50;

if nargin<3
  isExpand = false;
end

min_frame = inf;
max_frame = 0;
for i = 1:numel(tracks)
  min_frame = min(min(tracks{i}(:,end)),min_frame);
  max_frame = max(max(tracks{i}(:,end)),max_frame);
end

imgList = cell(max_frame-min_frame+1,1);
for fr = min_frame:max_frame
  imgList{fr-min_frame+1} = sprintf('%s/%05d.jpg', imgDir, fr);
end

I = imread(imgList{1});
[im_h, im_w, ~] = size(I);
if isExpand
  buf = 20;
  for i = 1:numel(tracks)
    tracks{i} = expandFaceToPerson(tracks{i}, [], buf, im_h, im_w);
  end
end


color_range = 'rgbymcw';

boxes_on_images = repmat(struct('bbox', []), [numel(imgList) 1]);

for i = 1:numel(tracks)
  for j = 1:size(tracks{i}, 1)
     boxes_on_images(tracks{i}(j,end)-min_frame+1).bbox = [ boxes_on_images(tracks{i}(j,end)-min_frame+1).bbox; ...
                                      tracks{i}(j,1:4) face_ids(i) mod(i, numel(color_range))+1];
  end
end

%keyboard;
frame_ctr = 0;
for i = 1:time_buf
  imname = sprintf('%s/%05d.jpg', imgDir, max(1,min_frame-time_buf+i));
  if (exist(imname, 'file'))
    if ~isempty(dump_dir)
      dump_file = sprintf('%s/%05d.jpg', dump_dir, frame_ctr);
      frame_ctr = frame_ctr + 1;
      saveboxes(imread(imname), [], color_range, dump_file);
    else
      showboxes(imread(imname), [], color_range);
    end
  end
end

for i = 1:numel(imgList)
  bb = boxes_on_images(i).bbox;
  imname = imgList{i};

  if (exist(imname, 'file'))
    if ~isempty(dump_dir)
      dump_file = sprintf('%s/%05d.jpg', dump_dir, frame_ctr);
      frame_ctr = frame_ctr + 1;      
      if ~isempty(bb)
        saveboxes(imread(imname), bb, color_range, dump_file, bb(:, end-1));
      else
        saveboxes(imread(imname), bb, color_range, dump_file);
      end
    else
      showboxes(imread(imname), bb, color_range);
    end
  end
end

for i = 1:time_buf
  imname = sprintf('%s/%05d.jpg', imgDir, max_frame+i);
  if (exist(imname, 'file'))
    if ~isempty(dump_dir)
      dump_file = sprintf('%s/%05d.jpg', dump_dir, frame_ctr);
      frame_ctr = frame_ctr + 1;
      saveboxes(imread(imname), [], color_range, dump_file);
    else
      showboxes(imread(imname), [], color_range);
    end
  end
end

