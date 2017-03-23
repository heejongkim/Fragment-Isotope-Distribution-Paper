#!/bin/csh
#BSUB -L /bin/csh
#BSUB -J LSF_create_training_data.sh[1-7]
#BSUB -q week
#BSUB -o /netscr/dennisg/log/Isotopes.log.%J
#BSUB -n 1

module load gcc/4.8.1
module load r/3.2.2
module load matlab

source ../config.sh

set S = `expr $LSB_JOBINDEX - 2`

set OUT_DIR = ${SPLINE_OUT_DIR}"/S${S}"

mkdir -p $OUT_DIR

rm -r ${OUT_DIR}/data/

mkdir ${OUT_DIR}/data/

rm -r ${OUT_DIR}/spline/

mkdir ${OUT_DIR}/spline/
mkdir ${OUT_DIR}/spline/model/
mkdir ${OUT_DIR}/spline/gof/
mkdir ${OUT_DIR}/spline/hist/
mkdir ${OUT_DIR}/spline/res/
mkdir ${OUT_DIR}/spline/scatter/

${BUILD_DIR}/GenerateTrainingData 0 $FASTA ${OUT_DIR}/data/ $MAX_SAMPLED_MASS $NUM_SAMPLES $S $MAX_ISOTOPE_DEPTH

chmod 775 ${OUT_DIR}/data/*

${SOURCE_DIR}/scripts/training/LSF_submit_models.sh $S $MAX_SAMPLED_MASS
