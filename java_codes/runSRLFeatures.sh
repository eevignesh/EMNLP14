#!/bin/bash

INPUT_DIR=$1
OUTPUT_DIR=$2
CAST_LIST=$3
#LOG_FILE=$4


#ant run -Darg0="./sample_scene/" -Darg1="./castList.txt" -Darg2="./sample_output/"

ant run -Darg0=$INPUT_DIR -Darg1="$CAST_LIST" -Darg2=$OUTPUT_DIR
