function Font = saveFontNumbers()


Name = 'Courier New';
Size = 48;
Characters = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890''ì!"£$%&/()=?^è+òàù,.-<\|;:_>*@#[]{}';
Padding = 0.05*Size;

Size = ceil(Size);
Padding = ceil(Padding);

Bitmaps = cell(1,length(Characters));

% Use a single figure and axis for maximum speed. White background.
fighandle = figure('Position',[50 50 150+Size 150+Size],'Units','pixels','Color',[1 1 1], 'Visible', 'off');
%keyboard;
axes('Position',[0 0 1 1],'Units','Normalized');
axis off;

for i = 1:length(Characters)
    % Place each character in the middle of the figure
    texthandle = text(0.5,1,Characters(i),'Units','Normalized','FontName',Name,'FontUnits','pixels','FontSize',Size,'HorizontalAlignment','Center','VerticalAlignment','Top','Interpreter','None','Color',[0 0 0]);

    %drawnow;
    % Take a snapshot
    %fprintf('before\n');keyboard;
    Bitmap = getframe(gcf);
	  %fprintf('after\n');keyboard;
    x1 = min(find(min(Bitmap.cdata(:, :, 1)) < 200));
    x2 = max(find(min(Bitmap.cdata(:, :, 1)) < 200));
    y1 = min(find(min(Bitmap.cdata(:, :, 1)') < 200));
    y2 = max(find(min(Bitmap.cdata(:, :, 1)') < 200));
    delete(texthandle);
    % Average RGB to minimise effect of ClearType etc.
    %keyboard;
    Bitmap.cdata = Bitmap.cdata(y1-1:y2+1, x1-1:x2+1,:);
    Bitmap = mean(Bitmap.cdata,3);
    % Crop height as appropriate (in MATLAB images, first dimension is
    % height). Some characters will be larger than Size (eg. y and g) -
    % allow for this.
    %Bitmap = Bitmap(1:find(mean(Bitmap,2)~=255,1,'last'),:);
    % Crop width to remove all white space
    %Bitmap = Bitmap(:,find(mean(Bitmap,1)~=255,1,'first'):find(mean(Bitmap,1)~=255,1,'last'));
    % Pad with kerning value
	  Bitmap(:,end:(end+Padding)) = 255;

    %keyboard;
    % Invert and store in binary format
    Bitmaps{i} = false(size(Bitmap));
    Bitmaps{i}(Bitmap < 160) = true; % This threshold could be changed
end

close(fighandle);

Font.Name = Name;
Font.Size = Size;
Font.Characters = Characters;
Font.Bitmaps = Bitmaps;

