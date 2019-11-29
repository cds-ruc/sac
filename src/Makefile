CC = gcc -g -g

ifndef SMR_SSD_CACHE_DIR
SMR_SSD_CACHE_DIR = .
endif

CFLAGS += -Wall -pthread
CPPFLAGS += -I$(SMR_SSD_CACHE_DIR) -I$(SMR_SSD_CACHE_DIR)/smr-simulator -I$(SMR_SSD_CACHE_DIR)/strategy

RM = rm -rf
#RMSHM = rm -f /dev/shm/*
OBJS = main.o global.o report.o timerUtils.o shmlib.o hashtable_utils.o cache.o trace2call.o daemon.o paul.o most.o most_cdc.o lru_cdc.o lru_private.o hashtb_pb.o simulator_v2.o


all: $(OBJS) smr-ssd-cache
	@echo 'Successfully built smr-ssd-cache...'

smr-ssd-cache:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJS) -lrt -lm -o $@

global.o: global.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

report.o: report.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

shmlib.o: shmlib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

timerUtils.o: timerUtils.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

hashtable_utils.o: hashtable_utils.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

cahce.o: cache.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

trace2call.o: trace2call.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

daemon.o: daemon.c 
	$(CC) $(CPPFLAGS) $(CFLAGE) -c $?

main.o: main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

costmodel.o: strategy/costmodel.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

pore_plus.o: strategy/pore_plus.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

losertree4pore.o: strategy/losertree4pore.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

pore.o: strategy/pore.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

pore_plus_v2.o: strategy/pore_plus_v2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

pv3.o: strategy/pv3.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

clock.o: strategy/clock.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

most.o: strategy/most.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

most_cdc.o: strategy/most_cdc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

lru.o: strategy/lru.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

lru_private.o: strategy/lru_private.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

lru_cdc.o: strategy/lru_cdc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

oldpore.o: strategy/oldpore.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?
paul.o: strategy/paul.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

lru_batch.o: strategy/lru_batch.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?
band_table.o: strategy/band_table.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

smr-simulator.o: smr-simulator/smr-simulator.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

hashtb_pb.o: smr-simulator/hashtb_pb.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

simulator_logfifo.o: smr-simulator/simulator_logfifo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

simulator_v2.o: smr-simulator/simulator_v2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?
clean:
	$(RM) *.o
	$(RM) $(SMR_SSD_CACHE_DIR)/smr-ssd-cache
#	$(RMSHM)
