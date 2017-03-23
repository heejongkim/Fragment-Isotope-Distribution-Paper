#!/bin/csh

source ../config.sh

bsub < LSF_get_sulfur_distribution.sh
bsub < LSF_create_training_data.sh > out
JOBID=`head -1 out | sed 's/.*<\\([0-9]*\\)>.*/\\1/'`
rm out

set MAX_TRAINING_JOBS = `expr $MAX_SULFUR + 2`

bsub < LSF_combine_models.sh  -w 'numended('$JOBID',>='$MAX_TRAINING_JOBS')'
