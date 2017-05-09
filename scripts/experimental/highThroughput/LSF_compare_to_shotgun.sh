#!/bin/csh
#BSUB -L /bin/csh
#BSUB -J LSF_compare_to_shotgun.sh
#BSUB -q day
#BSUB -o /netscr/dennisg/log/LSF_compare_to_shotgun.log.%J
#BSUB -n 1
#BSUB -M 8

module load gcc/4.8.1
module load r/3.2.2

source ../config.sh

set OUT_DIR = ${ROOT_OUT_DIR}"/compare_to_experimental/HT"
mkdir -p $OUT_DIR

${BUILD_DIR}/CompareToShotgun ${DATA_DIR}"/HELA_2017-01-31_5.mzML" ${DATA_DIR}"/HELA_2017-01-31_5.idXML" 0.0 $OUT_DIR
Rscript highThroughput/PlotShotgunResults.R ${OUT_DIR}"/distributionScores.out" $OUT_DIR

set OUT_DIR = ${ROOT_OUT_DIR}"/compare_to_experimental/HT_BSA_DDA"
mkdir -p $OUT_DIR

${BUILD_DIR}/CompareToShotgun ${DATA_DIR}"/BSA_2017-04-29_346_DDA.mzML" ${DATA_DIR}"/BSA_2017-04-29_346_DDA.idXML" 0.0 $OUT_DIR
Rscript highThroughput/PlotShotgunResults.R ${OUT_DIR}"/distributionScores.out" ${OUT_DIR}

set OUT_DIR = ${ROOT_OUT_DIR}"/compare_to_experimental/HT_BSA_SIM_30k"
mkdir -p $OUT_DIR

${BUILD_DIR}/CompareToShotgun ${DATA_DIR}"/BSA_2017-05-05_59_SIM.mzML" ${DATA_DIR}"BSA_2017-05-05_59_SIM.idXML" 0.0 $OUT_DIR
Rscript highThroughput/PlotShotgunResults.R ${OUT_DIR}"/distributionScores.out" ${OUT_DIR}

set OUT_DIR = ${ROOT_OUT_DIR}"/compare_to_experimental/HT_BSA_SIM_15k"
mkdir -p $OUT_DIR

${BUILD_DIR}/CompareToShotgun ${DATA_DIR}"/BSA_2017-05-05_59_SIM_15k.mzML" ${DATA_DIR}"BSA_2017-05-05_59_SIM_15k.idXML" 0.0 $OUT_DIR
Rscript highThroughput/PlotShotgunResults.R ${OUT_DIR}"/distributionScores.out" ${OUT_DIR}