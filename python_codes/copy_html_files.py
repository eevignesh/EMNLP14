#!/usr/bin/python

import os
import glob

tv_dir = '/scail/scratch/u/vigneshr/EMNLP2014/data/scripts/original/'

tv_shows = os.listdir(tv_dir)
VAT_dir = '/scail/scratch/u/vigneshr/softwares/apache/installation/htdocs/VATS/'
fid = open('%s/index.html'%(VAT_dir), 'w')
fid.write('<html>\n<body>\n')
#tv_shows = ['numb3rs_3x11']
for tv_show in tv_shows:
  cmd = 'cp -r %s%s/visualizations/VATS/ %s/%s'%(tv_dir, tv_show, VAT_dir, tv_show)
  print(cmd)
  os.system(cmd)
  fid.write('<a href="%s/index.html">%s</a><br>'%(tv_show, tv_show))
  fid2 = open('%s/%s/index.html'%(VAT_dir, tv_show), 'w')
  #scene_lists = os.listdir('%s/%s/scene*html'%(VAT_dir, tv_show))
  
  scene_id = 1
  fid2.write('<html>\n<body>\n');
  for scene in glob.glob('%s/%s/scene*html'%(VAT_dir, tv_show)):
    fid2.write('<a href="scene_%04d.html">scene_%04d</a><br>\n'%(scene_id, scene_id))
    scene_id = scene_id + 1
  fid2.write('</body>\n</html>\n');
  fid2.close()
  
fid.write('</body>\n</html>\n')
fid.close()
