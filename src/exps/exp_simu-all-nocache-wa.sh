#!/bin/bash

# track the every WA when Emulator do the RMW. Output to /home/outputs/logs/
# Exp Content: 	SIMU; small traces [NO Cache] WA
# Setting:	
#	define [NO_REAL_DISK_IO], [NO_CACHE],  [SIMU_NO_DISK_IO] [LOG_ALLOW]

cache_blksize=(20595 24795 48167 19554 106230 65278 39736 51410 36268 43761)
fifo_blksize=(3987 4941 9513 3754 21206 13054 7645 10273 6731 7451)

#chances_n=(1)

for i in "${!cache_blksize[@]}";
do
	rm -f /dev/shm/*
	/home/smr/smr-ssd-cache 0 0 $i 0 0 ${cache_blksize[$i]} ${fifo_blksize[$i]} LRU 1 0 > /home/outputs/pore_test_outputfiles/exp/nocache-wa-$i.out
done

#for i in "${!cache_blksize[@]}";
#do
#	rm -f /dev/shm/*
#	./smr-ssd-cache 0 0 $i 0 0 ${cache_blksize[$i]} ${fifo_blksize[$i]} 1 > /home/outputs/pore_test_outputfiles/most$(($i)).l
#done

