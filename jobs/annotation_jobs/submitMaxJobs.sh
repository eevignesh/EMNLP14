#!/bin/bash

MEM_COUNT=12
MAX_JOB_FILE='num_maxjob.txt'
LOG_FILE=job_submit.log

JOB_PREFIX=align
TOTAL_JOBS=127
JOB_CTR=1

# wait time in  seconds before checking the scheduler
BATCH_WAIT_TIME=15
MAX_JOB=`cat $MAX_JOB_FILE`

while [[ $JOB_CTR -le $TOTAL_JOBS ]]
do
  MAX_JOB=`cat $MAX_JOB_FILE`
  JOB_COUNT=`qstat -u vigneshr | wc | awk {'print $1'}`
  #JOB_COUNT=`qstat -u vigneshr | grep gorgo | wc | awk {'print $1'}`
  if [[ $JOB_COUNT -le $MAX_JOB ]]
  then
    JOB_FILE=$JOB_PREFIX"$(printf "_%05d.sh" $JOB_CTR)"
    SUB_CMD="$(printf "qsub -q visionlab -l pmem=%dgb %s" $MEM_COUNT $JOB_FILE)"
    $SUB_CMD
    LOG_CMD="$(printf "submitted %04d jobs till now: now submitting %s\n" $JOB_COUNT $JOB_FILE)"
    echo "$LOG_CMD" >> $LOG_FILE

    JOB_CTR=$(($JOB_CTR + 1))
  fi

  if [[ $JOB_COUNT -ge $MAX_JOB ]]
  then
    sleep $BATCH_WAIT_TIME
  else
    sleep 1
  fi

done
