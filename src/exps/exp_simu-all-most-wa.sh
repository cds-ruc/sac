#!/bin/bash
#Exp Content: 	SIMU; small traces [LRU] WA
#Setting:	define NO_REAL_DISK_IO, SIMU_NO_DISK_IO; undef PORE_BATCH

cache_blksize=500000 #2000MB cache size
fifo_zonesize=30

#chances_n=(1)

for i in $(seq 1 9)
do
	rm -f /dev/shm/*
	/home/smr/smr-ssd-cache  0 $i 1 0 $cache_blksize $cache_blksize $fifo_zonesize MOST -1 > /home/outputs/pore_test_outputfiles/exp/most-wa-t$i.out
done

