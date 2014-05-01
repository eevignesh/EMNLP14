#!/bin/bash

SCENE_BREAK_FILE=$1
OUTPUT_DIR=$2
CAST_LIST=$3
#LOG_FILE=$4


#ant run -Darg0="../sample_data/scene_breaks_new.txt" -Darg1="./sample_scene/" -Darg2="Verb" -Darg3="castList.txt" > out.txt
ant run -Darg0=$SCENE_BREAK_FILE -Darg1=$OUTPUT_DIR -Darg2="Verb" -Darg3=$CAST_LIST
