/*
 * main.c
 */
#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <stdint.h>

#include "../lib/xstrtol.h"
#include "report.h"
#include "shmlib.h"
#include "global.h"
#include "cache.h"
#include "smr-emulator/emulator_v2.h"
#include "trace2call.h"
#include "timerUtils.h"

int analyze_opts(int argc, char **argv);

unsigned int INIT_PROCESS = 0;

const char *tracefile[] = {
    "./traces/src1_2.csv.req",
    "./traces/wdev_0.csv.req",
    "./traces/hm_0.csv.req",
    "./traces/mds_0.csv.req",
    "./traces/prn_0.csv.req",
    "./traces/rsrch_0.csv.req",
    "./traces/stg_0.csv.req",
    "./traces/ts_0.csv.req",
    "./traces/usr_0.csv.req",
    "./traces/web_0.csv.req",
   // "./traces/production-LiveMap-Backend-4K.req", // --> not in used.
    "./traces/long.csv.req"                           // default set: cache size = 8M*blksize; persistent buffer size = 1.6M*blksize.
};
/* Return the value of STR, interpreted as a non-negative decimal integer,
   optionally multiplied by various values.
   Assign nonzero to *INVALID if STR does not represent a number in
   this format. */

static uintmax_t
parse_integer (const char *str, int *invalid)
{
  uintmax_t n;
  char *suffix;
  enum strtol_error e = xstrtoumax (str, &suffix, 10, &n, "bcEGkKMPTwYZ0");

  if (e == LONGINT_INVALID_SUFFIX_CHAR && *suffix == 'x')
    {
      uintmax_t multiplier = parse_integer (suffix + 1, invalid);

      if (multiplier != 0 && n * multiplier / multiplier != n)
	{
	  *invalid = 1;
	  return 0;
	}

      n *= multiplier;
    }
  else if (e != LONGINT_OK)
    {
      *invalid = 1;
      return 0;
    }

  return n;
}

int analyze_opts(int argc, char **argv)
{
    static struct option long_options[] = {
        {"cache-dev", required_argument, NULL, 'C'},  // FORCE
        {"smr-dev", required_argument, NULL, 'S'},    // FORCE
        {"no-cache", no_argument, NULL, 'N'},
        {"use-emulator", no_argument, NULL, 'E'},
        {"workload", required_argument, NULL, 'W'},   // FORCE
        {"workload-file", required_argument, NULL, 'T'},  // FORCE
        {"workload-mode", required_argument, NULL, 'M'},
        {"no-real-io", no_argument, NULL, 'D'},
        {"cache-size", required_argument, NULL, 'c'},
        {"pb-size", required_argument, NULL, 'p'},
        {"algorithm", required_argument, NULL, 'A'},
        {"offset", required_argument, NULL, 'O'},
        {"requests", required_argument, NULL, 'R'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    const char *optstr = "NE:W:M:F:DC:S:T:c:p:A:O:R:h";
    int longIndex;

    while (1)
    {
        int opt = getopt_long(argc, argv, optstr, long_options, &longIndex);
        if (opt == -1)
            break;
        //printf("opt=%c,\nlongindex=%d,\nnext arg index: optind=%d,\noptarg=%s,\nopterr=%d,\noptopt=%c\n",
        //opt, longIndex, optind, optarg, opterr, optopt);

        uintmax_t n = 0;
        int invalid = 0;
        switch (opt)
        {
        case 'N': // no-cache
            NO_CACHE = 1;
            printf("[User Setting] Not to use the cache layer.\n");
            break;

        case 'E': // use emulator
            EMULATION = 1;
            printf("[User Setting] Use SMR emulator on file: %s (Be sure your emulator file has at least 5TiB capacity.)\n", optarg);
            break;

        case 'A': // algorithm
            if (strcmp(optarg, "LRU") == 0)
                EvictStrategy = LRU_private;
            else if (strcmp(optarg, "SAC") == 0)
                EvictStrategy = SAC;
            else if (strcmp(optarg, "MOST") == 0)
                EvictStrategy = MOST;
            else if (strcmp(optarg, "MOST_CDC") == 0)
                EvictStrategy = MOST_CDC;
            else
                sac_error_exit("No such algorithm matched: %s.", optarg);

	    printf("[User Setting] Cache Algorithm: %s.\n", optarg);
            break;

        case 'W': // workload
            TraceID = atoi(optarg);
            if (TraceID > 11 || TraceID <= 0)
            {
                printf("ERROR: Workload number is illegal, please tpye with 1~11: \n.");
                printf("[1]: src1_2.csv.req\n");
                printf("[2]: wdev_0.csv.req\n");
                printf("[3]: hm_0.csv.req\n");
                printf("[4]: mds_0.csv.req\n");
                printf("[5]: prn_0.csv.req\n");
                printf("[6]: rsrch_0.csv.req\n");
                printf("[7]: stg_0.csv.req\n");
                printf("[8]: ts_0.csv.req\n");
                printf("[9]: usr_0.csv.req\n");
                printf("[10]: web_0.csv.req\n");
		printf("[11]: long.req\n");
                sac_exit(EXIT_FAILURE);
            }

            if ((TraceFile = fopen(tracefile[TraceID - 1], "rt")) == NULL)
            {

                printf("ERROR: Failed to open the trace file: %s\n", tracefile[TraceID - 1]);
                sac_exit(EXIT_FAILURE);
            }
	    printf("[User Setting] Workload file path: %s\n", tracefile[TraceID - 1]);
            break;

        case 'T': // workload file
            printf("[User Setting] Workload file path: %s\n", optarg);
            if ((TraceFile = fopen(optarg, "rt")) == NULL)
            {
                sac_error_exit("ERROR: Failed to open the trace file: %s\n", optarg);
            }
            break;

        case 'M': // workload I/O mode
            printf("[User Setting] Workload mode ");
            if (strcmp(optarg, "r") == 0 || strcmp(optarg, "R") == 0)
            {
                Workload_Mode = IOMODE_R;
                printf("[r]: read-only. \n");
            }
            else if (strcmp(optarg, "w") == 0 || strcmp(optarg, "W") == 0)
            {
                Workload_Mode = IOMODE_W;
                printf("[w]: write-only\n");
            }
            else if (strcmp(optarg, "rw") == 0 || strcmp(optarg, "RW") == 0)
            {
                Workload_Mode = IOMODE_RW;
                printf("[rw]: read-write\n");
            }
            else{
                printf("ERROR: unrecongnizd workload mode \"%s\", please assign mode [r]: read-only, [w]: write-only or [rw]: read-write.\n",
                       optarg);
                sac_exit(EXIT_FAILURE);
                }
            break;

        case 'D': // no-real-io
            NO_REAL_DISK_IO = 1;
            printf("[User Setting] No real disk IO. This option discards the request before it is sent to the device, so no real IO will be generated, \n\tbut the data structure including the cache algorithm and SMR Emulator statistics still works.\n");
            break;

        case 'C': // cache-dev
	    printf("optarg 1 = %s\n",optarg);
            cache_dev_path = optarg;

            printf("[User Setting] Cache device file: %s\n\t(You can still use ramdisk or memory fs for testing.)\n", cache_dev_path);
            break;

        case 'S': // SMR-dev
            smr_dev_path = optarg;

            printf("[User Setting] SMR device file: %s\n\t(You can still use conventional hard drives for testing.)\n", optarg);
            break;
        case 'c': // blkcnt of cache
            n = parse_integer(optarg, &invalid);
            if(invalid){
                sac_error_exit("invalid cache size number %s", optarg);
            }
            NBLOCK_SSD_CACHE = NTABLE_SSD_CACHE = n / BLKSZ;
            printf("[User Setting] Cache Size = %s, NBLOCK_SSD_CACHE = %ld.\n", optarg, NBLOCK_SSD_CACHE);
            break;

        case 'p': // blkcnt of smr's pb
            n = parse_integer(optarg, &invalid);
            if(invalid){
                sac_error_exit("invalid PB size number %s", optarg);
            }
            NBLOCK_SMR_PB = n / BLKSZ;
            printf("[User Setting] PB Size = %s, NBLOCK_SMR_PB = %ld.\n", optarg, NBLOCK_SMR_PB);
            break;

        case 'O': // offset, the started LBA of the workload.
            StartLBA = atol(optarg);
            printf("[User Setting] offset (the started LBA of the workload) is = %ld.\n", StartLBA);
            break;

        case 'R': // request number
            Request_limit = atol(optarg);
            printf("[User Setting] request number = %ld.\n", Request_limit);
            break;
        case 'h':
            printf("\
\n\
Usage: sac [OPTIONS] [Arguments]\n\
\n\
The Options include: \n\
\t--cache-dev		Device file path of the ssd/ramdisk for cache layer. \n\
\t--smr-dev		Device file path of the SMR drive or HDD for SMR emulator. \n\
\t--algorithm		One of [SAC], [LRU], [MOST], [MOST_CDC]. \n\
\t--no-cache		No cache layer, i.e. SMR only. \n\
\t--use-emulator	Use emulator. \n\
\t--no-real-io	No real disk I/O generated. \n\
\t--workload		Workload number for [1~11] corresponding to different trace files, in which the [11] is the big dataset. \n\
\t--workload-file		Or you can specofy the trace file path manually. \n\
\t--workload-mode		Three workload mode: [R]:read-only, [W]:write-only, [RW]:read-write	RW. \n\
\t--cache-size		Cache size: [size]{+M,G}. E.g. 32G. \n\
\t--pb-size		SMR Persistent Buffer size: [size]{+M,G}. E.g. 600M. \n\
\t--offset		Start LBA offset of the SMR: [size]{+M,G}. E.g. 10G. \n\
\t--requests		Requst number you want to run: [Nunmber]. \n\
\t--help          show this menu. \n\
");
            sac_exit(EXIT_SUCCESS);

        case '?':
            printf("There is an unrecognized option or option without argument: %s\n", argv[optind - 1]);
            sac_exit(EXIT_FAILURE);
            break;

        default:
            printf("There is an unrecognized option: %c\n", opt);
            break;
        }
    }

    /* Default Setting. */
    if (TraceFile == NULL)
    {
        printf("ERROR: No workload file, you need to point out the workload number by the arg: --workload N, where N is 1 ~ 10: \n");
        printf("[1]: src1_2.csv.req\n");
        printf("[2]: wdev_0.csv.req\n");
        printf("[3]: hm_0.csv.req\n");
        printf("[4]: mds_0.csv.req\n");
        printf("[5]: prn_0.csv.req\n");
        printf("[6]: rsrch_0.csv.req\n");
        printf("[7]: stg_0.csv.req\n");
        printf("[8]: ts_0.csv.req\n");
        printf("[9]: usr_0.csv.req\n");
        printf("[10]: web_0.csv.req\n");
        sac_exit(EXIT_FAILURE);
    }
    if(NO_REAL_DISK_IO != 1)
    {
        if((cache_fd = open(cache_dev_path, O_RDWR | O_DIRECT)) < 0){
            sac_error_exit("Unable to open CACHE device file: %s", optarg);
        }
        if((smr_fd = open(smr_dev_path, O_RDWR | O_DIRECT)) < 0){
            sac_error_exit("Unable to open SMR device file: %s", smr_dev_path);
        }
    }


    Cycle_Length = NBLOCK_SMR_PB;
    /* checking user option. */

    return 0;
}


int initRuntimeInfo()
{
    STT = (struct RuntimeSTAT *)multi_SHM_alloc(str_STT, sizeof(struct RuntimeSTAT));
    if (STT == NULL)
        return errno;

    STT->traceId = TraceID;
    STT->startLBA = StartLBA;
    STT->workload_mode = Workload_Mode;
    STT->cacheUsage = 0;
    STT->cacheLimit = NBLOCK_SSD_CACHE;

    STT->wtrAmp_cur = 0;
    STT->WA_sum = 0;
    STT->n_RMW = 0;
    return 0;
}


int main(int argc, char **argv)
{
    analyze_opts(argc, argv);

    /* Open Device */
    initRuntimeInfo();
    CacheLayer_Init();

    if (EMULATION)
    {
        /* Emulator */
        InitEmulator();
    }

    trace_to_iocall(TraceFile, StartLBA);

    // Finish Job. Exit Safely.
    if (EMULATION)
    {
        Emu_PrintStatistic();
        CloseSMREmu();
    }
    if(NO_REAL_DISK_IO)
    {
        close(smr_fd);
        close(cache_fd);
    }

    exit(EXIT_SUCCESS);
}
