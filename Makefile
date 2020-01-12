CC = gcc -g -g

ifndef SAC_ROOT
SAC_ROOT=.
SAC_LIB=${SAC_ROOT}/lib
SAC_SRC=${SAC_ROOT}/src
SAC_ALG=${SAC_SRC}/strategy
SAC_EMU=${SAC_SRC}/smr-emulator

endif

CFLAGS += -W -pthread
CPPFLAGS += -I$(SAC_ROOT) -I$(SAC_SRC) -I$(SAC_SRC)/smr-emulator -I$(SAC_SRC)/strategy -I$(SAC_LIB)

RM = rm -rf
#RMSHM = rm -f /dev/shm/*
OBJS = global.o main.o report.o timerUtils.o shmlib.o hashtable_utils.o cache.o trace2call.o sac.o most.o most_cdc.o lru_private.o hashtb_pb.o emulator_v2.o xstrtoumax.o


all: $(OBJS) sac
	@echo 'Successfully built sac...'

sac:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJS) -lrt -lm -o $@
global.o: ${SAC_SRC}/global.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

xstrtoumax.o: ${SAC_LIB}/xstrtoumax.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

report.o: ${SAC_SRC}/report.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

shmlib.o: ${SAC_SRC}/shmlib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

timerUtils.o: ${SAC_SRC}/timerUtils.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

hashtable_utils.o: ${SAC_SRC}/hashtable_utils.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

cache.o: ${SAC_SRC}/cache.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

trace2call.o: ${SAC_SRC}/trace2call.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

main.o: ${SAC_SRC}/main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

sac.o: ${SAC_ALG}/sac.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

most.o: ${SAC_ALG}/most.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

most_cdc.o: ${SAC_ALG}/most_cdc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

lru.o: ${SAC_ALG}/lru.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

lru_private.o: ${SAC_ALG}/lru_private.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

lru_cdc.o: ${SAC_ALG}/lru_cdc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

hashtb_pb.o: ${SAC_EMU}/hashtb_pb.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

emulator_v2.o: ${SAC_EMU}/emulator_v2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $?

clean:
	$(RM) *.o
	$(RM) $(SAC_ROOT)/sac
#	$(RMSHM)
