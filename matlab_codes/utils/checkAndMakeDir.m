%% Check if a director exists, else make it

function checkAndMakeDir(dir_name)

if ~exist(dir_name, 'dir')
  fprintf('The director %s does not exists ... making it\n', dir_name);
  unix(['mkdir ' dir_name]);
end
