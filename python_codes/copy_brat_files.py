#!/usr/bin/python

import os

tv_dir = '/scail/scratch/u/vigneshr/EMNLP2014/data/scripts/original/'

tv_shows = os.listdir(tv_dir)
brat_dir = '/scail/scratch/u/vigneshr/softwares/apache/installation/htdocs/brat/data/'

for tv_show in tv_shows:
  cmd = 'cp -r %s%s/brat_annotation_files/ %s/%s'%(tv_dir, tv_show, brat_dir, tv_show)
  cmd2 = 'cp %s/visual.conf %s/%s/'%(brat_dir, brat_dir, tv_show)
  print(cmd)
  os.system(cmd)
  os.system(cmd2)
