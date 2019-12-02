#!/bin/bash 
# ENV(global)
DEVICE=/mnt/smr/smr-rawdisk
BS=4K 

path_cleanscript=.
exe_clean="${path_cleanscript}/pb_clean.sh /mnt/smr/smr-rawdisk manual 4K 50K"

#Clean the SMR's PB region. 
bash ${exe_clean}

# [job]
SIZE=$((4*1000000))K
cat<< EOF > smr_read_lat.fio
[global]
name=rdm_read_nobuf
filename=${DEVICE}
blocksize=${BS}
filesize=8T
ioengine=sync
direct=1
rw=randread

[job_once]
size=${SIZE}
read_lat_log=./read_${SIZE}
EOF

echo "${SIZE}..."
fio ./smr_read_lat.fio > read-${SIZE}.log
echo "Finish."
#bash ${exe_clean}

