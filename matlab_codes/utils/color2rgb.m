function colorvalues = color2rgb(col)

switch col
case 'y'
  colorvalues = [1 1 0];
case 'c'
  colorvalues = [0 1 1];
case 'r'
  colorvalues = [1 0 0];
case 'g'
  colorvalues = [0 1 0];
case 'b' 
  colorvalues = [0 0 1];
case 'w'
  colorvalues = [1 1 1];
case 'k'
  colorvalues = [0 0 0];
case 'm'
  colorvalues = [1 0 1];
otherwise
  warning('unknown color input\n');
  colorvalues = [ 0 0 0];
end

colorvalues = colorvalues*255;
