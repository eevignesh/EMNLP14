#!/bin/python
import os

show_dir = '/scail/scratch/u/vigneshr/EMNLP2014/data/scripts/original/'
cmd_dir = '/scail/scratch/u/vigneshr/EMNLP2014/matlab_codes/visualizations/'
tv_shows = os.listdir(show_dir)

job_prefix = 'visual_jobs/visual_'
ctr = 1
for t in tv_shows:
  fid = open('%s%05d.sh'%(job_prefix, ctr), 'w')
  fid.write('#!/bin/bash\n\n')
  fid.write('cd %s\n'%(cmd_dir))
  fid.write('echo "makeVerbArgTrackSets(\'%s\', [])" | matlab_r2013b -nosplash -nodesktop -nodisplay\n'%(t))
  fid.close()
  os.chmod('%s%05d.sh'%(job_prefix, ctr), 0705);
  ctr = ctr + 1

