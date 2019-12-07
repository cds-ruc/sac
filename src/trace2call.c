#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "global.h"
#include "statusDef.h"

#include "timerUtils.h"
#include "cache.h"
#include "strategy/lru.h"
#include "trace2call.h"
#include "report.h"
#include "strategy/strategies.h"
#include "smr-emulator/emulator_v2.h"

//#include "/home/fei/git/Func-Utils/pipelib.h"
// #include "hrc.h"
extern struct RuntimeSTAT *STT;
#define REPORT_INTERVAL 250000 // 1GB for blksize=4KB

static void reportCurInfo();
static void report_ontime();
static void resetStatics();

static timeval tv_trace_start, tv_trace_end;
static double time_trace;

/** single request statistic information **/
#ifdef LOG_SINGLE_REQ
static timeval tv_req_start, tv_req_stop;
double io_latency; // latency of each IO
static char log[256];
static microsecond_t msec_req;
#endif //LOG_SINGLE_REQ

extern microsecond_t msec_r_hdd, msec_w_hdd, msec_r_ssd, msec_w_ssd;
extern int IsHit;
char logbuf[512];
FILE *log_lat;
char log_lat_path[] = "./logs/iolat.log";

void trace_to_iocall(FILE *trace, off_t startLBA)
{
    log_lat = fopen(log_lat_path, "w+");


    if (log_lat == NULL)
        paul_error_exit("log file open failure.");

    char action;
    off_t offset;
    char *ssd_buffer;
    int returnCode;
    int isFullSSDcache = 0;
    char pipebuf[128];
    static struct timeval tv_start_io, tv_stop_io;


// #ifdef CG_THROTTLE
//     static char *cgbuf;
//     int returncode = posix_memalign(&cgbuf, 512, 4096);
// #endif // CG_THROTTLE

    returnCode = posix_memalign((void**)&ssd_buffer, 1024, 16 * sizeof(char) * BLKSZ);
    if (returnCode < 0)
    {
        paul_warning("posix memalign error\n");
        //free(ssd_buffer);
        exit(-1);
    }
    int i;
    for (i = 0; i < 16 * BLKSZ; i++)
    {
        ssd_buffer[i] = '1';
    }

    _TimerLap(&tv_trace_start);

    blkcnt_t total_n_req;
    if(Request_limit > 0)
        total_n_req = Request_limit;
    else
        total_n_req = REPORT_INTERVAL * 500 * 3; //isWriteOnly ? (blkcnt_t)REPORT_INTERVAL*500*3 : REPORT_INTERVAL*500*3;

    blkcnt_t skiprows = 0;                            //isWriteOnly ?  50000000 : 100000000;

    while (!feof(trace) && STT->reqcnt_s < total_n_req)
    {

        returnCode = fscanf(trace, "%c %d %lu\n", &action, &i, &offset);
        if (returnCode < 0)
        {
            paul_warning("error while reading trace file.");
            break;
        }
        if (skiprows > 0)
        {
            skiprows--;
            continue;
        }

        offset = (offset + startLBA) * BLKSZ;
        if (!isFullSSDcache && (STT->flush_clean_blocks + STT->flush_hdd_blocks) > 0)
        {
            reportCurInfo();
            resetStatics(); // Reset the statistics of warming phrase, cuz we don't care.
            isFullSSDcache = 1;
        }

#ifdef LOG_SINGLE_REQ
        _TimerLap(&tv_req_start);
#endif // TIMER_SINGLE_REQ
        _TimerLap(&tv_start_io);
        sprintf(pipebuf, "%c,%lu\n", action, offset);
        if (action == ACT_WRITE && (Workload_Mode & IOMODE_W))
        {
            write_block(offset, ssd_buffer);

            _TimerLap(&tv_stop_io);

            STT->reqcnt_w++;
            STT->reqcnt_s++;

#ifdef LOG_IO_LAT
            io_latency = TimerInterval_SECOND(&tv_start_io, &tv_stop_io);
            sprintf(log, "%f,%c\n", io_latency, action);
            paul_log(log, log_lat);
#endif // LOG_IO_LAT
        }
        else if (action == ACT_READ && (Workload_Mode & IOMODE_R))
        {
            read_block(offset, ssd_buffer);

            _TimerLap(&tv_stop_io);

            STT->reqcnt_r++;
            STT->reqcnt_s++;

#ifdef LOG_IO_LAT
            io_latency = TimerInterval_SECOND(&tv_start_io, &tv_stop_io);
            sprintf(log, "%f,%c\n", io_latency, action);
            paul_log(log, log_lat);
#endif //LOG_IO_LAT
        }
        else if (action != ACT_READ)
        {
            printf("Trace file gets a wrong result: action = %c.\n", action);
            paul_error_exit("Trace file gets a wrong result");
        }
#ifdef LOG_SINGLE_REQ //Legacy
        _TimerLap(&tv_req_stop);
        msec_req = TimerInterval_MICRO(&tv_req_start, &tv_req_stop);
        /*
            print log
            format:
            <req_id, r/w, ishit, time cost for: one request, read_ssd, write_ssd, read_smr, write_smr>
        */
        // sprintf(logbuf,"%lu,%c,%d,%ld,%ld,%ld,%ld,%ld\n",STT->reqcnt_s,action,IsHit,msec_req,msec_r_ssd,msec_w_ssd,msec_r_hdd,msec_w_hdd);
        msec_r_ssd = msec_w_ssd = msec_r_hdd = msec_w_hdd = 0;
#endif // TIMER_SINGLE_REQ

        if (STT->reqcnt_s > 0 && STT->reqcnt_s % REPORT_INTERVAL == 0)
        {
            report_ontime();
            if (STT->reqcnt_s % ((blkcnt_t)REPORT_INTERVAL * 500) == 0)
            {
                reportCurInfo();
                // resetStatics();
                if (EMULATION)
                {
                    Emu_PrintStatistic();
                    // Emu_ResetStatisic();
                }
            }
        }
    }

    _TimerLap(&tv_trace_end);
    time_trace = Mirco2Sec(TimerInterval_MICRO(&tv_trace_start, &tv_trace_end));
    reportCurInfo();

#ifdef HRC_PROCS_N
    for (i = 0; i < HRC_PROCS_N; i++)
    {
        sprintf(pipebuf, "EOF\n");
        pipe_write(PipeEnds_of_MAIN[i], pipebuf, 64);
    }
#endif // HRC_PROCS_N
    free(ssd_buffer);
    fclose(trace);
    fclose(log_lat);
}


static void reportCurInfo()
{
    printf(" totalreqNum:%lu\n read_req_count: %lu\n write_req_count: %lu\n",
           STT->reqcnt_s, STT->reqcnt_r, STT->reqcnt_w);

    printf(" hit num:%lu\n hitnum_r:%lu\n hitnum_w:%lu\n",
           STT->hitnum_s, STT->hitnum_r, STT->hitnum_w);

    printf(" read_ssd_blocks:%lu\n flush_ssd_blocks:%lu\n read_hdd_blocks:%lu\n flush_dirty_blocks:%lu\n flush_clean_blocks:%lu\n",
           STT->load_ssd_blocks, STT->flush_ssd_blocks, STT->load_hdd_blocks, STT->flush_hdd_blocks, STT->flush_clean_blocks);

    //    printf(" hash_miss:%lu\n hashmiss_read:%lu\n hashmiss_write:%lu\n",
    //           STT->hashmiss_sum, STT->hashmiss_read, STT->hashmiss_write);

    printf(" total run time (s): %lf\n time_read_ssd : %lf\n time_write_ssd : %lf\n time_read_smr : %lf\n time_write_smr : %lf\n",
           time_trace, STT->time_read_ssd, STT->time_write_ssd, STT->time_read_hdd, STT->time_write_hdd);
    printf(" Batch flush HDD time:%u\n", msec_bw_hdd);

    printf(" Cache Proportion(R/W): [%ld/%ld]\n", STT->incache_n_clean, STT->incache_n_dirty);
    printf(" wt_hit_rd: %lu\n rd_hit_wt: %lu\n", STT->wt_hit_rd, STT->rd_hit_wt);
}

static void report_ontime()
{
    //    _TimerLap(&tv_checkpoint);
    //    double timecost = Mirco2Sec(TimerInterval_SECOND(&tv_trace_start,&tv_checkpoint));

    //     printf("totalreq:%lu, readreq:%lu, hit:%lu, readhit:%lu, flush_ssd_blk:%lu flush_hdd_blk:%lu, hashmiss:%lu, readhassmiss:%lu writehassmiss:%lu\n",
    //           STT->reqcnt_s,STT->reqcnt_r, STT->hitnum_s, STT->hitnum_r, STT->flush_ssd_blocks, STT->flush_hdd_blocks, STT->hashmiss_sum, STT->hashmiss_read, STT->hashmiss_write);
    printf("totalreq:%lu, readreq:%lu, wrtreq:%lu, hit:%lu, readhit:%lu, flush_ssd_blk:%lu flush_dirty_blk:%lu\n",
           STT->reqcnt_s, STT->reqcnt_r, STT->reqcnt_w, STT->hitnum_s, STT->hitnum_r, STT->flush_ssd_blocks, STT->flush_hdd_blocks);
    _TimerLap(&tv_trace_end);
    double timecost = Mirco2Sec(TimerInterval_MICRO(&tv_trace_start, &tv_trace_end));
    printf("current run time: %.0f\n", timecost);
}

static void resetStatics()
{

    //    STT->hitnum_s = 0;
    //    STT->hitnum_r = 0;
    //    STT->hitnum_w = 0;
    STT->load_ssd_blocks = 0;
    STT->flush_ssd_blocks = 0;
    STT->flush_hdd_blocks = 0;
    STT->flush_clean_blocks = 0;
    STT->load_hdd_blocks = 0;

    STT->reqcnt_r = STT->reqcnt_w = 0;
    STT->hitnum_s = STT->hitnum_r = STT->hitnum_w = 0;

    STT->time_read_hdd = 0.0;
    STT->time_write_hdd = 0.0;
    STT->time_read_ssd = 0.0;
    STT->time_write_ssd = 0.0;
    STT->hashmiss_sum = 0;
    STT->hashmiss_read = 0;
    STT->hashmiss_write = 0;
    msec_bw_hdd = 0;
}
