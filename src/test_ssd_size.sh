#!/bin/bash

fifo_nums=(9513 9900 3754 21207 13054 3988 7646 10273 6731 4941 7451)
strategys=(1 2 9)
ssd_size_times=(2.5 7.5 10 12.5)
zone_size=20971520     # 20MB

for i in "${!fifo_nums[@]}";
do
    if [ $i -eq 2 -o $i -eq 7 -o $i -eq 8 -o $i -eq 9 ];
    then
        period_long=${fifo_nums[$i]}
        for strategy in ${strategys[@]};
        do
            j=0
            while [ $j -lt ${#ssd_size_times[@]} ]
            do
                ssd_cache_num=$(echo ${fifo_nums[$i]}*${ssd_size_times[$j]}|bc)
                echo ./smr-ssd-cache ${ssd_cache_num%.*} ${fifo_nums[$i]} 0 4096 $zone_size $period_long $strategy $i
                ./smr-ssd-cache ${ssd_cache_num%.*} ${fifo_nums[$i]} 0 4096 $zone_size $period_long $strategy $i
                let j++
            done
        done
    fi
done
