#!/bin/bash
#Exp Content: 	SIMU; small traces [LRU] WA
#Setting:	define NO_REAL_DISK_IO, EMU_NO_DISK_IO; undef PORE_BATCH

cache_blksize=(2000000 4000000 6000000 8000000 10000000)

fifo_zonesize=35

#for i in "${!cache_blksize[@]}";
#do
#	/home/smr/smr-ssd-cache 0 0 11 1 0 ${cache_blksize[$i]} 35 PV3 30 > /home/outputs/pore_test_outputfiles/exp/pv3-diff-cachesize$i.out
#done

for i in "${!cache_blksize[@]}";
do
        /home/smr/smr-ssd-cache 0 0 11 1 0 ${cache_blksize[$i]} 35 MOST 1 > /home/outputs/pore_test_outputfiles/exp/most-diff-cachesize$i.out
done

for i in "${!cache_blksize[@]}";
do
        /home/smr/smr-ssd-cache 0 0 11 1 0 ${cache_blksize[$i]} 35 LRU 30 > /home/outputs/pore_test_outputfiles/exp/lru-diff-cachesize$i.out
done


