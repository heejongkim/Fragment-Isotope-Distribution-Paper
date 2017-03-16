#!/bin/csh
#BSUB -L /bin/csh
#BSUB -J LSF_combine_models.sh
#BSUB -q week
#BSUB -n 1
#BSUB -M 1

module load python/2.7.1

source ../config.sh

python ${SOURCE_DIR}/scripts/training/combineModels.py $AVERAGINE_OUT_DIR $SULFUR_OUT_DIR $MAX_ISOTOPE $MAX_SULFUR > ${AVERAGINE_OUT_DIR}"/IsotopeSplines.xml"