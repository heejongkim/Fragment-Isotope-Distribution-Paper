#!/usr/bin/env python
import sys
import os
import re
from collections import defaultdict
from math import floor
from math import isnan

root_dir = sys.argv[1]
num_bins = float(sys.argv[2])
prefix = sys.argv[3]
job_id = int(sys.argv[4])-1
num_jobs = int(sys.argv[5])

comp2bin2count = defaultdict(dict)

max_val = -float("inf")
min_val = float("inf")

for f in os.listdir(root_dir):
    fp = root_dir+"/"+f
    if os.path.isfile(fp) and ".out" in f and prefix in f:
        job = int(f.split("_")[-1].split(".")[0])
        if job % num_jobs != job_id: continue

        infile = open(fp)
        for line in infile:
            [residual, comp] = line.strip().split("\t")
            residual = float(residual)
            max_val = max(max_val, residual)
            min_val = min(min_val, residual)
        close(fp)

bin_size = (max_val-min_val) / num_bins

for f in os.listdir(root_dir):
    fp = root_dir+"/"+f
    if os.path.isfile(fp) and ".out" in f and prefix in f:
        job = int(f.split("_")[-1].split(".")[0])
        if job % num_jobs != job_id: continue

        infile = open(fp)
        for line in infile:
            [residual, comp] = line.strip().split("\t")
            bin = round(float(residual)/bin_size)*bin_size
            if not comp2bin2count[comp].has_key(bin):
                comp2bin2count[comp][bin]=0
            comp2bin2count[comp][bin]+=1
        close(fp)

for comp in comp2bin2count:
    for bin in comp2bin2count[comp]:
        print "\t".join([comp, str(bin), str(comp2bin2count[comp][bin])])
