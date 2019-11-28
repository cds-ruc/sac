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

#include "report.h"
#include "shmlib.h"
#include "global.h"
#include "cache.h"
#include "smr-simulator/simulator_logfifo.h"
#include "smr-simulator/simulator_v2.h"
#include "trace2call.h"
#include "daemon.h"
#include "timerUtils.h"
//#include "/home/fei/git/Func-Utils/pipelib.h"

//static char str_program_config[];

unsigned int INIT_PROCESS = 0;
void ramdisk_iotest()
{
    int fdram = open("/mnt/ramdisk/ramdisk", O_RDWR | O_DIRECT);
    printf("fdram=%d\n", fdram);

    char *buf;
    posix_memalign(&buf, 512, 4096);
    size_t count = 512;
    off_t offset = 0;

    int r;
    while (1)
    {
        r = pwrite(fdram, buf, count, offset);
        if (r <= 0)
        {
            printf("write ramdisk error:%d\n", r);
            exit(1);
        }
    }
}
//
char *tracefile[] = {
    "/home/fei/traces/src1_2.csv.req",
    "/home/fei/traces/wdev_0.csv.req",
    "/home/fei/traces/hm_0.csv.req",
    "/home/fei/traces/mds_0.csv.req",
    "/home/fei/traces/prn_0.csv.req", //1 1 4 0 0 106230 5242880 0
    "/home/fei/traces/rsrch_0.csv.req",
    "/home/fei/traces/stg_0.csv.req",
    "/home/fei/traces/ts_0.csv.req",
    "/home/fei/traces/usr_0.csv.req",
    "/home/fei/traces/web_0.csv.req",
    "/home/fei/traces/production-LiveMap-Backend-4K.req", // --> not in used.
    "/home/fei/traces/long.req"                           // default set: cache size = 8M*blksize; persistent buffer size = 1.6M*blksize.
    //"/home/fei/traces/merged_trace_x1.req.csv"
};

//blksize_t trace_req_total[] = {14024860,2654824,8985487,2916662,17635766,3254278,6098667,4216457,12873274,9642398,1,1481448114};

int main(int argc, char **argv)
{

    //FunctionalTest();
    //ramdisk_iotest();

    // 1 1 1 0 0 100000 100000
    // 1 1 0 0 0 100000 100000

    // 1 4 0 0 500000 106230 5242880 LRU 0

    //0 11 1 0 8000000 8000000 30 PAUL -1

    analyze_opts(argc, argv);


    if (argc == 10)
    {
        UserId = atoi(argv[1]);
        TraceId = atoi(argv[2]);
        WriteOnly = atoi(argv[3]);
        StartLBA = atol(argv[4]);

        NBLOCK_MAX_CACHE_SIZE = atol(argv[5]);
        NBLOCK_SSD_CACHE = NTABLE_SSD_CACHE = atol(argv[6]);
        NBLOCK_SMR_FIFO = atol(argv[7]) * (ZONESZ / BLKSZ); //750 * 1024 * 1024 / BLKSZ; //

        if (strcmp(argv[8], "LRU") == 0)
            EvictStrategy = LRU_private;
        else if (strcmp(argv[8], "LRU_RW") == 0)
            EvictStrategy = LRU_rw;
        else if (strcmp(argv[8], "PAUL") == 0)
            EvictStrategy = PAUL;
        else if (strcmp(argv[8], "PORE") == 0)
            EvictStrategy = PORE;
        else if (strcmp(argv[8], "MOST") == 0)
            EvictStrategy = MOST;
        else if (strcmp(argv[8], "MOST_RW") == 0)
            EvictStrategy = MOST_RW;
        else if (strcmp(argv[8], "OLDPORE") == 0)
            EvictStrategy = OLDPORE;
        else
            usr_error("No cache algorithm matched. ");

        if (atoi(argv[9]) < 0)
            Cycle_Length = NBLOCK_SMR_FIFO;
        else
            Cycle_Length = atoi(argv[9]) * (ZONESZ / BLKSZ);
#ifdef CACHE_PROPORTIOIN_STATIC
        Proportion_Dirty = atof(argv[10]);
#endif // Proportion_Dirty

        //EvictStrategy = PORE_PLUS;
    }
    else
    {
        printf("parameters are wrong %d\n", argc);
        exit(EXIT_FAILURE);
    }

#ifdef HRC_PROCS_N
    int forkcnt = 0;
    while (forkcnt < HRC_PROCS_N)
    {
        int pipefd[2];
        int fpid = fork_pipe_create(pipefd);
        if (fpid > 0)
        { /* MAIN Process*/
            printf("pipefd = %d,%d\n", pipefd[0], pipefd[1]);
            Fork_Pid = 0;
            close(pipefd[0]);
            PipeEnds_of_MAIN[forkcnt] = pipefd[1];
            forkcnt++;
            continue;
        }
        else if (fpid == 0)
        { /* Child HRC Process */
            Fork_Pid = forkcnt + 1;
            close(pipefd[1]);
            PipeEnd_of_HRC = pipefd[0];
            break;
        }
        else
        {
            perror("fork_pipe\n");
            exit(EXIT_FAILURE);
        }
    }
#endif // HRC_PROCS_N

    if (!I_AM_HRC_PROC)
    { /* If this is a MAIN process */
        /* Open Device */

        ssd_fd = open(ssd_device, O_RDWR | O_DIRECT);

#ifndef SIMULATION
        /* Real Device */
        hdd_fd = open(smr_device, O_RDWR | O_DIRECT);
        printf("Device ID: hdd=%d, ssd=%d\n", hdd_fd, ssd_fd);
#else
        /* Emulator */
        fd_fifo_part = open(simu_smr_fifo_device, O_RDWR | O_DIRECT);
        fd_smr_part = open(simu_smr_smr_device, O_RDWR | O_DIRECT | O_FSYNC);
        printf("Simulator Device: fifo part=%d, smr part=%d\n", fd_fifo_part, fd_smr_part);
        if (fd_fifo_part < 0 || fd_smr_part < 0)
        {
#ifndef SIMU_NO_DISK_IO
            usr_error("No emulator smr devices.");
#endif
        }
        InitSimulator();
#endif
    }
    else
    { /* If this is a HRC process */
#ifdef HRC_PROCS_N
        NBLOCK_SSD_CACHE = NTABLE_SSD_CACHE = NBLOCK_MAX_CACHE_SIZE / HRC_PROCS_N * Fork_Pid;
#endif // HRC_PROCS_N
    }

    initRuntimeInfo();
    //    STT->trace_req_amount = trace_req_total[TraceId];
    CacheLayer_Init();

    //#ifdef DAEMON_PROC
    //    pthread_t tid;
    //    int err = pthread_create(&tid, NULL, daemon_proc, NULL);
    //    if (err != 0)
    //    {
    //        printf("[ERROR] initSSD: fail to create thread: %s\n", strerror(err));
    //        exit(-1);
    //    }
    //#endif // DAEMON

    trace_to_iocall(tracefile[TraceId], WriteOnly, StartLBA);

#ifdef SIMULATION
    Emu_PrintStatistic();
    CloseSMREmu();
#endif
    close(hdd_fd);
    close(ssd_fd);
    ReportCM();
    wait(NULL);
    exit(EXIT_SUCCESS);
}

int initRuntimeInfo()
{
    char str_STT[50];
    sprintf(str_STT, "STAT_b%d_u%d_t%d", BatchId, UserId, TraceId);
    STT = (struct RuntimeSTAT *)multi_SHM_alloc(str_STT, sizeof(struct RuntimeSTAT));
    if (STT == NULL)
        return errno;

    STT->batchId = BatchId;
    STT->userId = UserId;
    STT->traceId = TraceId;
    STT->startLBA = StartLBA;
    STT->isWriteOnly = WriteOnly;
    STT->cacheUsage = 0;
    STT->cacheLimit = 0x7fffffffffffffff;

    STT->wtrAmp_cur = 0;
    STT->WA_sum = 0;
    STT->n_RMW = 0;
    return 0;
}

int analyze_opts(int argc, char **argv);
int proc_paul_exit(int flag)
{
    exit(flag);
}

int analyze_opts(int argc, char **argv)
{
    static struct option long_options[] = {
        {"cache-dev", required_argument, NULL, 'C'},
        {"smr-dev", required_argument, NULL, 'S'},
        {"no-cache", no_argument, NULL, 'N'},
        {"use-emulator", required_argument, NULL, 'E'},
        {"workload", required_argument, NULL, 'W'},
        {"workload-mode", required_argument, NULL, 'M'},
        {"no-real-io", no_argument, NULL, 'D'},
        {0, 0, 0, 0}};
    const char *optstr = "NE:W:M:F:DC:S:";
    int longIndex;

    while (1)
    {
        int opt = getopt_long(argc, argv, optstr, long_options, &longIndex);
        if (opt == -1)
            break;

        //printf("opt=%c,\nlongindex=%d,\nnext arg index: optind=%d,\noptarg=%s,\nopterr=%d,\noptopt=%c\n",
        //opt, longIndex, optind, optarg, opterr, optopt);

        switch (opt)
        {
        case 'N': // no-cache
            printf("[User Setting] Not to use the cache layer.\n");
            break;

        case 'E': // use emulator
            printf("[User Setting] Use SMR emulator on file: %s (Be sure your emulator file has at least 5TiB capacity.)\n", optarg);
            break;

        case 'W':
            printf("[User Setting] Workload file: %s\n", optarg);
            break;

        case 'M': // trace-filter
            printf("[User Setting] Workload mode ");
            if (strcmp(optarg, "r") == 0)
                printf("[r]: read-only. \n");
            else if (strcmp(optarg, "w") == 0)
                printf("[w]: write-only\n");
            else if (strcmp(optarg, "rw") == 0)
                printf("[rw]: read-write\n");
            else
                printf("ERROR: unrecongnizd workload mode \"%s\", please assign mode [r]: read-only, [w]: write-only or [rw]: read-write.\n",
                       optarg);
            proc_paul_exit(EXIT_FAILURE);
            break;

        case 'D':
            printf("[User Setting] No real disk IO. This option discards the request before it is sent to the device, so no real IO will be generated, \n\tbut the data structure including the cache algorithm and simulator statistics still works.\n");
            break;

        case 'C':
            printf("[User Setting] Cache device file: %s\n\t(You can still use ramdisk or memory fs for testing.)\n", optarg);
            break;

        case 'S':
            printf("[User Setting] SMR device file: %s\n\t(You can still use conventional hard drives for testing.)\n", optarg);
            break;

        case '?':
            printf("There is an unrecognized option or option without argument: %s\n", argv[optind - 1]);
            proc_paul_exit(EXIT_FAILURE);

        default:
            printf("There is an unrecognized option: %c\n", opt);
            break;
        }
    }

    /* checking user option. */

    return 0;
}
