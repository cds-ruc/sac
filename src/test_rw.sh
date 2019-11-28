#!/bin/bash

fifo_nums=(9513 9900 3754 21207 13054 3988 7646 10273 6731 4941 7451)
strategys=(1 2 4 5 9)
zone_size=20971520     # 20MB

for i in "${!fifo_nums[@]}";
do
    if [ $i -eq 2 -o $i -eq 7 -o $i -eq 8 -o $i -eq 9 ];
    then
        for strategy in ${strategys[@]};
        do
            ssd_cache_num=$[${fifo_nums[$i]}*5]
            period_long=${fifo_nums[$i]}
            j=0
            echo ./smr-ssd-cache $ssd_cache_num ${fifo_nums[$i]} 0 4096 $zone_size $period_long $strategy $i
            ./smr-ssd-cache $ssd_cache_num ${fifo_nums[$i]} 0 4096 $zone_size $period_long $strategy $i
            let j++
        done
    fi
done
