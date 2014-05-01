#!/bin/bash

INPUT_DIR=$1
OUTPUT_DIR=$2
CAST_LIST=$3
#LOG_FILE=$4


<<<<<<< HEAD
=======
#ant run -Darg0="../sample_data/scene_breaks_new.txt" -Darg1="./sample_scene/" -Darg2="Verb" -Darg3="castList.txt" > out.txt
>>>>>>> ff8d560cc266b2cdbdde1791eb9671c05ea67782
#ant run -Darg0="./sample_scene/" -Darg1="./castList.txt" -Darg2="./sample_output/"

ant run -Darg0=$INPUT_DIR -Darg1="$CAST_LIST" -Darg2=$OUTPUT_DIR
