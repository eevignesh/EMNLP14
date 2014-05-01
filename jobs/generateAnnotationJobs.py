#!/bin/python
import os

show_dir = '/user/vigneshr/scr/EMNLP14/data/scripts/original/'
job_cmd  = './runAnnotationSRL.sh'

tv_shows = os.listdir(show_dir)

job_prefix = 'annotation_jobs/brat_'
ctr = 1
for t in tv_shows:
  file_number = ctr%5
  if os.path.isfile('%s%05d.sh'%(job_prefix, file_number)):
    fid = open('%s%05d.sh'%(job_prefix, file_number), 'a')
  else:
    fid = open('%s%05d.sh'%(job_prefix, file_number), 'w')
    fid.write('#!/bin/bash\n\n')
  fid.write('if [ ! -d \"%s%s/brat_annotation_files\" ]; then\n'%(show_dir, t))
  fid.write('\tmkdir %s%s/brat_annotation_files\n'%(show_dir, t))
  fid.write('fi\n')
  fid.write('cd /juicer/scr91/scr/vigneshr/EMNLP14/annotations/java\n')
  fid.write('%s %s%s/alignments_clean/scene_breaks.txt %s%s/brat_annotation_files/ %s%s/alignments_clean/script_srt_speaker.align\n\n\n' \
      %(job_cmd, show_dir, t, show_dir, t, show_dir, t))
  fid.close()
  os.chmod('%s%05d.sh'%(job_prefix, file_number), 0705);
  ctr = ctr + 1
