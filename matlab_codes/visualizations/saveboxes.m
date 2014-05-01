function saveboes(im, boxes, color_range, save_name, track_nums)

bitFont = load('FontCourierNew_48.mat');

[r,c,d] = size(im);
boxes = floor(boxes);
%bitFont = BitmapFont('Arial', 20, '12');
if ~isempty(boxes)
  numfilters = floor(size(boxes, 1));
  for i = 1:numfilters
    x0 = boxes(i,1);
    y0 = boxes(i,2);
    w  = boxes(i,3) - x0 + 1;
    h  = boxes(i,4) - y0 + 1;
    col = color_range(boxes(i,end));

    try
    x = [x0:x0+w x0*ones(1,h+1) x0:x0+w (x0+w)*ones(1,h+1)];
    y = [y0*ones(1,w+1) y0:y0+h (y0+h)*ones(1,w+1) y0:y0+h];
  catch
    fprintf('bug here\n');
    keyboard;
  end
    x = [x x-1 x+1];
    y = [y y-1 y+1];
    x = max(x, 1); x = min(x, c);
    y = max(y, 1); y = min(y, r);
    colrgb = color2rgb(col);

    index = sub2ind([r c],y,x);
    im(index) = colrgb(1);
    im(index+r*c) = colrgb(2);
    im(index+2*r*c) = colrgb(3);

    if(nargin>4)
      im = AddTextToImage(im, num2str(track_nums(i)), [y0+5, x0+5], colrgb/255, bitFont.Font);      
    end
    %keyboard;
  end
end


if (isempty(save_name))
  imshow(im);
  drawnow;
else
  imwrite(im, save_name);
end
