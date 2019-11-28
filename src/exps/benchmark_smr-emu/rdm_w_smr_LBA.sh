#!/bin/bash

./smr-pb-forceclean.sh /dev/sdc big

/home/smr/smr-ssd-cache 0 0 1 0 1 1 1 LRU -1 > ./r_128M.smr
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_128M.smr
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_128M.smr

./smr-pb-forceclean.sh /dev/sdc big
/home/smr/smr-ssd-cache 0 1 1 0 1 1 1 LRU -1 > ./r_512M.smr
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_512M.smr
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_512M.smr

./smr-pb-forceclean.sh /dev/sdc big
/home/smr/smr-ssd-cache 0 2 1 0 1 1 1 LRU -1 > ./r_2G.smr
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_2G.smr
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_2G.smr

./smr-pb-forceclean.sh /dev/sdc big
/home/smr/smr-ssd-cache 0 3 1 0 1 1 1 LRU -1 > ./r_8G.smr
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_8G.smr
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_8G.smr

./smr-pb-forceclean.sh /dev/sdc big
/home/smr/smr-ssd-cache 0 4 1 0 1 1 1 LRU -1 > ./r_32G.smr
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_32G.smr
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_32G.smr

./smr-pb-forceclean.sh /dev/sdc big
/home/smr/smr-ssd-cache 0 5 1 0 1 1 1 LRU -1 > ./r_128G.smr
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_128G.smr
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_128G.smr

./smr-pb-forceclean.sh /dev/sdc big
/home/smr/smr-ssd-cache 0 6 1 0 1 1 1 LRU -1 > ./r_512G.smr
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_512G.smr
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_512G.smr

./smr-pb-forceclean.sh /dev/sdc big
/home/smr/smr-ssd-cache 0 7 1 0 1 1 1 LRU -1 > ./r_2T.smr
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_2T.smr
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_2T.smr

./smr-pb-forceclean.sh /dev/sdc big

