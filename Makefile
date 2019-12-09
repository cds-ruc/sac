CC = gcc -g -g

ifndef PAUL_ROOT
PAUL_ROOT=.
PAUL_SRC=${PAUL_ROOT}/src
PAUL_ALG=${PAUL_SRC}/strategy
PAUL_EMU=${PAUL_SRC}/smr-emulator

endif

CFLAGS += -Wall -pthread 
CPPFLAGS += -I$(PAUL_ROOT) -I$(PAUL_SRC) -I$(PAUL_SRC)/smr-emulator -I$(PAUL_SRC)/strategy

RM = rm -rf
#RMSHM = rm -f /dev/shm/* 
OBJS = global.o main.o report.o timerUtils.o shmlib.o hashtable_utils.o cache.o trace2call.o sac.o most.o most_cdc.o lru_private.o hashtb_pb.o emulator_v2.o


all: $(OBJS) sac
	@echo 'Successfully built sac...'

sac:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJS) -lrt -lm -o $@
global.o: ${PAUL_SRC}/global.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

report.o: ${PAUL_SRC}/report.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

shmlib.o: ${PAUL_SRC}/shmlib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

timerUtils.o: ${PAUL_SRC}/timerUtils.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

hashtable_utils.o: ${PAUL_SRC}/hashtable_utils.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

cache.o: ${PAUL_SRC}/cache.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

trace2call.o: ${PAUL_SRC}/trace2call.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

main.o: ${PAUL_SRC}/main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

sac.o: ${PAUL_ALG}/sac.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

most.o: ${PAUL_ALG}/most.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

most_cdc.o: ${PAUL_ALG}/most_cdc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

lru.o: ${PAUL_ALG}/lru.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

lru_private.o: ${PAUL_ALG}/lru_private.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

lru_cdc.o: ${PAUL_ALG}/lru_cdc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

hashtb_pb.o: ${PAUL_EMU}/hashtb_pb.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

emulator_v2.o: ${PAUL_EMU}/emulator_v2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

clean:
	$(RM) *.o
	$(RM) $(PAUL_ROOT)/sac
#	$(RMSHM)
