#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "global.h"
#include "timerUtils.h"
#include "daemon.h"

extern struct RuntimeSTAT* STT;
static timeval tv_start, tv_end;
void* daemon_proc()
{
    char path[] = "/tmp/daemon_cachesys.out";
    unlink(path);
    FILE* file = fopen(path, "at+"); // append mode
    char str_runtime[4096];

    pthread_t th = pthread_self();
    pthread_detach(th);

    time_t rawtime;
    struct tm * timeinfo;

    struct RuntimeSTAT lastSTT, curSTT;
    lastSTT = *STT;

    /* Bandwidth */
    double bw_s, bw_r, bw_w; // KB/s
    double bw_smr_r, bw_smr_w;
    
    _TimerLap(&tv_start);
    while(1)
    {
        sleep(1);
	_TimerLap(&tv_end);
	double interval = TimerInterval_SECOND(&tv_start, &tv_end);
	_TimerLap(&tv_start);

        /* Load runtime arverage IO speed(KB/s) */
        curSTT = *STT;

        bw_s = (double)(curSTT.reqcnt_s - lastSTT.reqcnt_s) * 4 / interval;
        bw_r = (double)(curSTT.reqcnt_r - lastSTT.reqcnt_r) * 4 / interval;
        bw_w = (double)(curSTT.reqcnt_w - lastSTT.reqcnt_w) * 4 / interval;

        bw_smr_r = (double)(curSTT.load_hdd_blocks - lastSTT.load_hdd_blocks) * 4 / interval;
        bw_smr_w = (double)(curSTT.flush_hdd_blocks - lastSTT.flush_hdd_blocks) * 4 /interval;

        lastSTT = curSTT;

        /* Current WrtAmp */
        double wrtamp = STT->wtrAmp_cur;
        /* Process Percentage */
        double progress = (double)STT->reqcnt_s / STT->trace_req_amount * 100;

        /* Output */
        time(&rawtime);
        timeinfo = localtime (&rawtime);

        sprintf(str_runtime, "%sReqcnt(s,r,w):\t%ld\t%ld\t%ld\n\tfrom HDD(r,w):\t\t%ld\t%ld\n\tfrom SSD(r,w):\t\t%ld\t%ld\n",
        	asctime(timeinfo),curSTT.reqcnt_s,curSTT.reqcnt_r,curSTT.reqcnt_w,curSTT.load_hdd_blocks,curSTT.flush_hdd_blocks,curSTT.load_ssd_blocks,curSTT.flush_ssd_blocks);
	fwrite(str_runtime,strlen(str_runtime),1,file);
	sprintf(str_runtime, "bandwidth(s,r,w):\t%d\t%d\t%d\n", (int)bw_s, (int)bw_r,(int)bw_w);
        fwrite(str_runtime,strlen(str_runtime),1,file);
        sprintf(str_runtime, "\tSMR(r,w):\t\t%d,\t%d\n", (int)bw_smr_r, (int)bw_smr_w);
        fwrite(str_runtime,strlen(str_runtime),1,file);
        sprintf(str_runtime, "\twrtAmp:%d\tprogress:%d\%\n",(int)wrtamp,(int)progress);
        fwrite(str_runtime,strlen(str_runtime),1,file);
        fflush(file);
    }
}

