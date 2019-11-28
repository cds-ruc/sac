#!/bin/bash
#每次测试前需要确保SMR的Persistent Buffer(PB)是否已经被完全清空，若不是，请顺序写遍整个磁盘。
echo "1T..."
fio --filename=/dev/sdc --filesize=1T ./rdm_w_nobuf.fio > R-1T.log
dd if=/dev/zero of=/dev/sdc bs=4K count=256M

echo "2T..."
fio --filename=/dev/sdc --filesize=2T ./rdm_w_nobuf.fio > R-2T.log
dd if=/dev/zero of=/dev/sdc bs=4K count=512M

echo "4T..."
fio --filename=/dev/sdc --filesize=4T ./rdm_w_nobuf.fio > R-4T.log
dd if=/dev/zero of=/dev/sdc bs=4K count=1G

echo "finish!"
