#!/bin/bash 
path_cleanscript=/home/fei/devel/smr-ssd-cache_multi-user/src/exps/fio_benchmarks/paperexps
exe_clean="${path_cleanscript}/smr-pb-forceclean.sh /mnt/smr/smr-rawdisk small" 

bash ${exe_clean}

DEVICE=/mnt/smr/smr-rawdisk BS=4K SIZE=$((4*100))K
cat<< EOF > wa-data.fio
[global]
name=rdm_w_nobuf
filename=${DEVICE}
blocksize=${BS}
ioengine=sync
direct=1
rw=randwrite

[job_once]
size=${SIZE}
offset=0

[job_loop]
stonewall
size=20M 
offset=100M 
runtime=100
time_based
write_lat_log=./iolog_loop_${SIZE}.log
EOF

echo "${SIZE}..."
fio ./wa-data > R-${SIZE}.log
echo "Finish."
bash ${exe_clean}

# echo "128G..."
# DEVICE=/dev/sda BS=4K FILESIZE=128G
# cat<< EOF > task.fio
# [global]
# name=rdm_w_nobuf
# filename=${DEVICE}
# blocksize=${BS}
# ioengine=sync
# direct=1
# rw=randwrite

# [job_warming]
# runtime=180
# time_based

# [job_real]
# stonewall
# runtime=300
# time_based
# write_lat_log=./iolog_${FILESIZE}.log
# EOF

# fio ./task.fio > R-8G.log
# dd if=/dev/zero of=/dev/sda bs=4K count=32M

# echo "4T..."
# DEVICE=/dev/sda BS=4K FILESIZE=4T
# cat<< EOF > task.fio
# [global]
# name=rdm_w_nobuf
# filename=${DEVICE}
# filesize=${FILESIZE}
# blocksize=${BS}
# ioengine=sync
# direct=1
# rw=randwrite

# [job_warming]
# runtime=180
# time_based

# [job_real]
# stonewall
# runtime=300
# time_based
# write_lat_log=./iolog_${FILESIZE}.log
# EOF

# fio ./task.fio > R-8G.log
# dd if=/dev/zero of=/dev/sda bs=4K count=1G




