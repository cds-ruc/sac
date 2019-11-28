#!/bin/bash

TEST_DEV=$1
TYPE=$2
BS=$3
COUNT=$4

if [ ${TYPE} = "fulldisk" ]; then
    echo "fulldisk cleanning..."
    echo "force cleaning smr PB, fdev=${TEST_DEV}"
    #STEP 1. random write 4KB * 250K = 1GB data with target LAB range of 0~8GB.
#        to make sure smr PB has RMW all the old data, and PB now is full with the (0~8)GB data. 
    fio --name=pb_forvceclean --filename=${TEST_DEV} --bs=4K --filesize=8G --size=1G --ioengine=sync --direct=1 --rw=randwrite --numjobs=1

    #STEP 2. seq write to the 0~8GB range. note that, smr knows you are random writing, so it will let your seq write directly flush into target position, do not need to pass by the PB, and FTL will invaild those data in PB. Okay, the PB is cleaned up. 

    dd if=/dev/zero of=${TEST_DEV} bs=4K count=2M
    sync

elif [ ${TYPE} = "manual" ]; then
    echo "Manual cleanning..."
    dd if=/dev/zero of=${TEST_DEV} bs=${BS} count=${COUNT}
    sync
else
    echo "set the clean scale, 'fulldisk' or 'manual <blksize> <count>?'" 
fi 

echo "PB cleaned up!"


