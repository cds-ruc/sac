#!/bin/bash
#Exp Content: 	SIMU; small traces [LRU] WA
#Setting:	define NO_REAL_DISK_IO, EMU_NO_DISK_IO; undef PORE_BATCH

cache_blksize=8000000 #32GB cache size
fifo_zonesize=30

#chances_n=(1)

sleep 10m
/home/smr/smr-ssd-cache  0 11 0 0 $cache_blksize $cache_blksize $fifo_zonesize LRU_CDC -1 0.2 > /home/outputs/pore_test_outputfiles/exp/prop8-2.out

sleep 10m
/home/smr/smr-ssd-cache  0 11 0 0 $cache_blksize $cache_blksize $fifo_zonesize LRU_CDC -1 0.4 > /home/outputs/pore_test_outputfiles/exp/prop6-4.out
      
sleep 10m  
/home/smr/smr-ssd-cache  0 11 0 0 $cache_blksize $cache_blksize $fifo_zonesize LRU_CDC -1 0.6 > /home/outputs/pore_test_outputfiles/exp/prop4-6.out
        
sleep 10m
/home/smr/smr-ssd-cache  0 11 0 0 $cache_blksize $cache_blksize $fifo_zonesize LRU_CDC -1 0.8 > /home/outputs/pore_test_outputfiles/exp/prop2-8.out

