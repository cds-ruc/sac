#!/bin/bash

cache_blksize=(20595 24795 48167 19554 106230 65278 39736 51410 36268 43761)
fifo_blksize=(3987 4941 9513 3754 21206 13054 7645 10273 6731 7451)

#chances_n=(1)

for i in "${!cache_blksize[@]}";
do
	rm -f /dev/shm/*
	./smr-ssd-cache 0 0 $i 0 0 ${cache_blksize[$i]} ${fifo_blksize[$i]} 0 1 > /home/outputs/pore_test_outputfiles/new_pore+$(($i)).l
done

#for i in "${!cache_blksize[@]}";
#do
#	rm -f /dev/shm/*
#	./smr-ssd-cache 0 0 $i 0 0 ${cache_blksize[$i]} ${fifo_blksize[$i]} 1 > /home/outputs/pore_test_outputfiles/most$(($i)).l
#done

