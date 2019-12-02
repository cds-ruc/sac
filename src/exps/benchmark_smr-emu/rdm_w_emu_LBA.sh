#!/bin/bash

/home/smr/smr-ssd-cache 0 0 1 0 1 1 1 LRU -1 > ./r_128M.emu
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_128M.emu
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_128M.emu

/home/smr/smr-ssd-cache 0 1 1 0 1 1 1 LRU -1 > ./r_512M.emu
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_512M.emu
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_512M.emu

/home/smr/smr-ssd-cache 0 2 1 0 1 1 1 LRU -1 > ./r_2G.emu
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_2G.emu
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_2G.emu

/home/smr/smr-ssd-cache 0 3 1 0 1 1 1 LRU -1 > ./r_8G.emu
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_8G.emu
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_8G.emu

/home/smr/smr-ssd-cache 0 4 1 0 1 1 1 LRU -1 > ./r_32G.emu
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_32G.emu
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_32G.emu

/home/smr/smr-ssd-cache 0 5 1 0 1 1 1 LRU -1 > ./r_128G.emu
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_128G.emu
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_128G.emu

/home/smr/smr-ssd-cache 0 6 1 0 1 1 1 LRU -1 > ./r_512G.emu
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_512G.emu
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_512G.emu

/home/smr/smr-ssd-cache 0 7 1 0 1 1 1 LRU -1 > ./r_2T.emu
mv /home/outputs/logs/log_lat /home/outputs/logs/log_lat_2T.emu
mv /home/outputs/logs/log_wa /home/outputs/logs/log_wa_2T.emu


