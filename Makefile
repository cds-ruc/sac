CC = gcc -g -g

ifndef PAUL_ROOT
PAUL_ROOT=.
PAUL_SRC=${PAUL_ROOT}/src
endif

CFLAGS += -Wall -pthread
CPPFLAGS += -I$(PAUL_ROOT) -I$(PAUL_SRC) -I$(PAUL_SRC)/smr-emulator -I$(PAUL_SRC)/strategy

RM = rm -rf
#RMSHM = rm -f /dev/shm/* 
OBJS = main.o report.o timerUtils.o shmlib.o hashtable_utils.o cache.o trace2call.o paul.o most.o most_cdc.o lru_cdc.o lru_private.o hashtb_pb.o emulator_v2.o


all: $(OBJS) smr-ssd-cache
	@echo 'Successfully built smr-ssd-cache...'

smr-ssd-cache:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJS) -lrt -lm -o $@

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

main.o: main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

paul.o: strategy/paul.c
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

hashtb_pb.o: smr-emulator/hashtb_pb.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

simulator_v2.o: smr-emulator/emulator_v2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

clean:
	$(RM) *.o
	$(RM) $(PAUL_ROOT)/smr-ssd-cache
#	$(RMSHM)
