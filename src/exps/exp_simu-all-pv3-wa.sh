#!/bin/bash
#Exp Content: 	SIMU; small traces [LRU] WA
#Setting:	define NO_REAL_DISK_IO, EMU_NO_DISK_IO; undef PORE_BATCH

cache_blksize=(20595 24795 48167 19554 106230 65278 39736 51410 36268 43761)
fifo_blksize=(3987 4941 9513 3754 21206 13054 7645 10273 6731 7451)
#chances_n=(1)

fifo_zonesize=(1 1 2 1 5 3 2 2 2 2)

for i in "${!cache_blksize[@]}";
do
	rm -f /dev/shm/*
	/home/smr/smr-ssd-cache 0 0 $i 1 0 ${cache_blksize[$i]} ${fifo_zonesize[$i]} PV3 -1 > /home/outputs/pore_test_outputfiles/exp/pv3-score-wa-t$i.out
done


