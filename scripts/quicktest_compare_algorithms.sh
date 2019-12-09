#!/bin/bash

DATETIME="`date +%Y-%m-%d,%H:%m:%s`"  

cd ..

type ./sac > /dev/null 2>&1

CMD_SAC=$?

if [ $CMD_SAC -ne 0 ]; then 
	echo "error: No executable program file <sac>, please check if the program has been built."
	exit 1
fi

echo "Testing.. SAC"
./sac --no-real-io --use-emulator --workload 11 --workload-mode W --requests 125000000 --algorithm SAC > ./logs/SAC_${DATETIME}.log
echo "SAC Finished. See: logs/SAC_"${DATETIME}".log"
echo "Testing.. MOST"
./sac --no-real-io --use-emulator --workload 11 --workload-mode W --requests 125000000 --algorithm MOST > ./logs/MOST_${DATETIME}.log
echo "MOST Finished. See: logs/MOST_"${DATETIME}".log"
echo "Testing.. LRU"
./sac --no-real-io --use-emulator --workload 11 --workload-mode W --requests 125000000 --algorithm LRU > ./logs/LRU_${DATETIME}.log
echo "LRU Finished. See: logs/LRU_"${DATETIME}".log"

echo "Done. "

