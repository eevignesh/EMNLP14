function showboxes(im, boxes, color_range, save_name)

% showboxes(im, boxes)
% Draw boxes on top of image.

clf;
figure('visible', 'off');
image(im); 
axis equal;
axis on;
if ~isempty(boxes)
  numfilters = floor(size(boxes, 1));
  for i = 1:numfilters
    x1 = boxes(i,1);
    y1 = boxes(i,2);
    x2 = boxes(i,3);
    y2 = boxes(i,4);
    c = color_range(boxes(i,end));
    line([x1 x1 x2 x2 x1]', [y1 y2 y2 y1 y1]', 'color', c, 'linewidth', 3);
  end
end
drawnow;

if nargin >= 4

  axis off; axis tight;             
  set(gca, 'position', [0 0 1 1], 'visible', 'off');
  set(gcf, 'PaperPositionMode', 'auto');
  h = gcf;
  saveas(h, save_name, 'jpg');

end
