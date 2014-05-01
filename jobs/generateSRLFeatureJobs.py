#!/bin/python
import os

<<<<<<< HEAD
show_dir = '/scail/scratch/u/vigneshr/EMNLP2014/data/scripts/original/'
=======
show_dir = '/user/vigneshr/scr/EMNLP14/data/scripts/original/'
>>>>>>> ff8d560cc266b2cdbdde1791eb9671c05ea67782
job_cmd  = './runSRLFeatures.sh'

tv_shows = os.listdir(show_dir)

<<<<<<< HEAD
job_prefix = 'srl_feature_jobs/srl_feat_'
ctr = 1
for t in tv_shows:
  file_number = ctr
=======
job_prefix = 'srl_feature_jobs/brat_'
ctr = 1
for t in tv_shows:
  file_number = ctr%5
>>>>>>> ff8d560cc266b2cdbdde1791eb9671c05ea67782
  if os.path.isfile('%s%05d.sh'%(job_prefix, file_number)):
    fid = open('%s%05d.sh'%(job_prefix, file_number), 'a')
  else:
    fid = open('%s%05d.sh'%(job_prefix, file_number), 'w')
    fid.write('#!/bin/bash\n\n')
  fid.write('if [ ! -d \"%s%s/srl_feature_files\" ]; then\n'%(show_dir, t))
  fid.write('\tmkdir %s%s/srl_feature_files\n'%(show_dir, t))
  fid.write('fi\n')
<<<<<<< HEAD
  fid.write('cd /scail/scratch/u/vigneshr/EMNLP2014/java_codes/\n')
=======
  fid.write('cd /juicer/scr91/scr/vigneshr/EMNLP14/annotations/java\n')
>>>>>>> ff8d560cc266b2cdbdde1791eb9671c05ea67782
  fid.write('%s %s%s/brat_annotation_files/ %s%s/srl_feature_files/ %s%s/alignments_clean/script_srt_speaker.align\n\n\n' \
      %(job_cmd, show_dir, t, show_dir, t, show_dir, t))
  fid.close()
  os.chmod('%s%05d.sh'%(job_prefix, file_number), 0705);
  ctr = ctr + 1
