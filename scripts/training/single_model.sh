#!/bin/csh

source ../config.sh

set file = $1
set S = $2
set max_mass = $3
set OUT_DIR = $4

set precursor = `echo "$file" | sed -n "s/.*\/[a-zA-Z]*\([0-9]*\).*/\1/p"`

set out_res_hist = ${OUT_DIR}/spline/hist/Precursor${precursor}.pdf
set out_scatter = ${OUT_DIR}/spline/scatter/Precursor${precursor}.pdf
set out_res = ${OUT_DIR}/spline/res/Precursor${precursor}.pdf
set out_gof = ${OUT_DIR}/spline/gof/Precursor${precursor}.txt
set out_models = ${OUT_DIR}/spline/model/Precursor${precursor}.xml
set out_spline_eval = ${OUT_DIR}/spline/eval/Precursor${precursor}.tab

set param = "IsotopeSpline("${SPLINE_BREAKS_SIZE}",'"${S}"','"${precursor}"','"${file}"','"${out_res_hist}"','"${out_scatter}"','"${out_res}"','"${out_gof}"','"${out_models}"','"${out_spline_eval}"');quit force"

cd ${SOURCE_DIR}/scripts/training/
matlab -nodisplay -nosplash -nodesktop -r $param
