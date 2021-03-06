#!/bin/csh
#BSUB -L /bin/csh
#BSUB -J LSF_plots.sh[1-21]
#BSUB -q hour
#BSUB -o /netscr/dennisg/log/training_plots.log.%J
#BSUB -n 1

module load r/3.2.2

source ../config.sh

set DATA_DIR = ${SPLINE_OUT_DIR}"/"
set OUT_DIR = ${ROOT_OUT_DIR}"/training/"

set PRECURSOR_ISOTOPE = `expr $LSB_JOBINDEX - 1`

Rscript plotModel.R ${DATA_DIR}"spline_eval/" ${DATA_DIR} $PRECURSOR_ISOTOPE -1 ${OUT_DIR}"Average_precursor"${PRECURSOR_ISOTOPE}"_model.eps" 100000
Rscript plotModel.R ${DATA_DIR}"spline_eval/" ${DATA_DIR} $PRECURSOR_ISOTOPE -1 ${OUT_DIR}"Average_zoom_precursor"${PRECURSOR_ISOTOPE}"_model.eps" 10000
Rscript plotModel.R ${DATA_DIR}"spline_eval/" ${DATA_DIR} $PRECURSOR_ISOTOPE $MAX_SULFUR ${OUT_DIR}"Precursor"${PRECURSOR_ISOTOPE}"_model.eps" 10000
