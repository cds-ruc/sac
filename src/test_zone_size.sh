#!/bin/bash

fifo_nums=(9513 9900 3754 21207 13054 3988 7646 10273 6731 4941 7451)
strategys=(7 8 9)
zone_sizes=(2097152 10485760 31457280 52428800)     # 2MB, 10MB, 30MB, 50MB

i=9
period_long=${fifo_nums[$i]}
for strategy in ${strategys[@]};
do
    ssd_cache_num=$[${fifo_nums[$i]}*5]
    j=0
    while [ $j -lt ${#zone_sizes[@]} ]
    do
        echo ./smr-ssd-cache $ssd_cache_num ${fifo_nums[$i]} 0 4096 ${zone_sizes[$j]} $period_long  $strategy $i
        ./smr-ssd-cache $ssd_cache_num ${fifo_nums[$i]} 0 4096 ${zone_sizes[$j]} $period_long $strategy $i
        let j++
    done
done
