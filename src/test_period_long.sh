#!/bin/bash

fifo_nums=(9513 9900 3754 21207 13054 3988 7646 10273 6731 4941 7451)
strategys=(7 8 9)
zone_size=20971520      # 20MB
period_long_times=(0.1 0.5 2 5 10)

i=9
for k in "${!period_long_times[@]}";
do
    period_long=$(echo ${fifo_nums[$i]}*${period_long_times[$k]}|bc)
    for strategy in ${strategys[@]};
    do
        ssd_cache_num=$[${fifo_nums[$i]}*5]
        echo ./smr-ssd-cache $ssd_cache_num ${fifo_nums[$i]} 0 4096 $zone_size ${period_long%.*} $strategy $i
        ./smr-ssd-cache $ssd_cache_num ${fifo_nums[$i]} 0 4096 $zone_size ${period_long%.*} $strategy $i
        let j++
    done
done
