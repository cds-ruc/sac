./smr-pb-forceclean.sh /dev/sda big

echo "8G..."
DEVICE=/dev/sda BS=4K FILESIZE=8G
cat<< EOF > task.fio
[global]
name=rdm_w_nobuf
filename=${DEVICE}
filesize=${FILESIZE}
blocksize=${BS}
ioengine=sync
direct=1
rw=randwrite

[job_warming]
runtime=180
time_based

[job_real]
stonewall
runtime=300
time_based
write_lat_log=./iolog_${FILESIZE}.log
EOF

fio ./task.fio > R-8G.log
dd if=/dev/zero of=${DEVICE} bs=4K count=2M

echo "128G..."
DEVICE=/dev/sda BS=4K FILESIZE=128G
cat<< EOF > task.fio
[global]
name=rdm_w_nobuf
filename=${DEVICE}
filesize=${FILESIZE}
blocksize=${BS}
ioengine=sync
direct=1
rw=randwrite

[job_warming]
runtime=180
time_based

[job_real]
stonewall
runtime=300
time_based
write_lat_log=./iolog_${FILESIZE}.log
EOF

fio ./task.fio > R-8G.log
dd if=/dev/zero of=/dev/sda bs=4K count=32M

echo "4T..."
DEVICE=/dev/sda BS=4K FILESIZE=4T
cat<< EOF > task.fio
[global]
name=rdm_w_nobuf
filename=${DEVICE}
filesize=${FILESIZE}
blocksize=${BS}
ioengine=sync
direct=1
rw=randwrite

[job_warming]
runtime=180
time_based

[job_real]
stonewall
runtime=300
time_based
write_lat_log=./iolog_${FILESIZE}.log
EOF

fio ./task.fio > R-8G.log
dd if=/dev/zero of=/dev/sda bs=4K count=1G




