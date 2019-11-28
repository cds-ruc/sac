#/bin/bash
DISK1=$1
DISK2=$2

echo "making RAID0 from [$DISK1], [$DISK2]"

mdadm -C /dev/md0 -a yes -l 0 -n 2 $DISK1 $DISK2

mdadm -D /dev/md0



