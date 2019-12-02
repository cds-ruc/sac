#!/bin/bash 
# ENV(global)
DEVICE=/mnt/smr/smr-rawdisk
BS=4K 

path_cleanscript=.
#exe_clean="${path_cleanscript}/pb_clean.sh /mnt/smr/smr-rawdisk manual 4K 50K"
exe_clean="${path_cleanscript}/pb_clean.sh fulldisk"

#Clean the SMR's PB region. 
bash ${exe_clean}

# [job]
SIZE=$((4*100))G
NR=4608

cat<< EOF > wa-data.fio
[global]
name=rdm_w_nobuf
filename=${DEVICE}
filesize=${SIZE}
blocksize=${BS}
ioengine=sync
direct=1
rw=randwrite:${NR}

[job]
write_lat_log=./iolog_${SIZE}_${OFF_INC}
EOF

echo "${SIZE}..."
fio ./wa-data.fio > R-${SIZE}-${OFF_INC}.log
echo "Finish."
bash ${exe_clean}

