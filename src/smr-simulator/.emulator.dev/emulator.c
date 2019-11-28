/*
  This is a updated versoin (v2) based on log-simulater.
  There are two most important new designs:
  1. FIFO Write adopts APPEND_ONLY, instead of in-place updates.
  2. When the block choosing to evict out of FIFO is a "old version",
     then drop it off，won't activate write-back, and move head pointer to deal with next one.
*/
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>
#include <aio.h>

#include "../report.h"
#include "../cache.h"
#include "../timerUtils.h"
#include "../statusDef.h"
#include "simulator_logfifo.h"
#include "inner_ssd_buf_table.h"


#define OFF_BAND_TMP_PERSISIT   0 // The head 80MB of FIFO for temp persistence band needed to clean.
#define OFF_FIFO                80*1024*1024

#ifdef SIMU_NO_DISK_IO
#define DISK_READ(fd,buf,size,offset) (size)
#define DISK_WRITE(fd,buf,size,offset) (size)
#else
#define DISK_READ(fd,buf,size,offset) (pread(fd,buf,size,offset))
#define DISK_WRITE(fd,buf,size,offset) (pwrite(fd,buf,size,offset))
#endif

/* Default set by configure, fd_fifo_part and fd_smr_part is seperately a partation  of a CMR disk
 * I set the head 0 to 5G of the disk as the partition 1m for fifo, and the rest part of disk is partiton 2 for smr*/
int  fd_fifo_part;
int  fd_smr_part;

static FIFOCtrl global_fifo_ctrl;
static FIFODesc* fifo_desp_array;
static char* BandBuffer;
static blksize_t NSMRBands = 194180;		// smr band cnt = 194180;
static unsigned long BNDSZ = 36*1024*1024;      // bandsize = 36MB  (18MB~36MB)


int ACCESS_FLAG = 1;

pthread_mutex_t simu_smr_fifo_mutex;

static long	band_size_num;
static long	num_each_size;

static FIFODesc* clockGetDespPB();
static void* smr_fifo_monitor_thread();
static void flushFIFO();

static long simu_read_smr_bands;
static long simu_flush_bands;
static long simu_flush_band_size;

static blkcnt_t simu_n_collect_fifo;
static blkcnt_t simu_n_read_fifo;
static blkcnt_t simu_n_write_fifo;
static blkcnt_t simu_n_read_smr;

static blkcnt_t simu_n_fifo_write_HIT = 0;
static double simu_time_read_fifo;
static double simu_time_read_smr;
static double simu_time_write_smr;
static double simu_time_write_fifo;

static double simu_time_collectFIFO = 0; /** To monitor the efficent of read all blocks in same band **/

static void* smr_fifo_monitor_thread();

static int invalidDespInFIFO(FIFODesc* desp);
#define isFIFOEmpty (global_fifo_ctrl.head == global_fifo_ctrl.tail)
#define isFIFOFull  ((global_fifo_ctrl.tail + 1) % (NBLOCK_SMR_FIFO + 1) == global_fifo_ctrl.head)

static unsigned long GetSMRActualBandSizeFromSSD(unsigned long offset);
static unsigned long GetSMRBandNumFromSSD(unsigned long offset);
static off_t GetSMROffsetInBandFromSSD(FIFODesc * ssd_hdr);

/** AIO related for blocks collected in FIFO **/
#define max_aio_count 6000
struct aiocb aiolist[max_aio_count];
struct aiocb* aiocb_addr_list[max_aio_count];/* >= band block size */

FILE* log_wa;
char log_wa_path[] = "/home/outputs/logs/log_wa";


/*
 * init inner ssd buffer hash table, strategy_control, buffer, work_mem
 */
void InitEmulator()
{
    /* initialliz related constants */
    band_size_num = BNDSZ / 1024 / 1024 / 2 + 1;
    num_each_size = NSMRBands / band_size_num;

    global_fifo_ctrl.n_used = 0;
    global_fifo_ctrl.head = global_fifo_ctrl.tail = 0;

    posix_memalign(&fifo_desp_array, 1024,sizeof(FIFODesc) * (NBLOCK_SMR_FIFO + 1));


    FIFODesc* fifo_hdr = fifo_desp_array;
    long i;
    for (i = 0; i < (NBLOCK_SMR_FIFO + 1); fifo_hdr++, i++)
    {
        fifo_hdr->despId = i;
        fifo_hdr->isValid = 0;
    }

    posix_memalign(&BandBuffer, 512, sizeof(char) * BNDSZ * 50);

    pthread_mutex_init(&simu_smr_fifo_mutex, NULL);

    /** AIO related **/

    /** statistic related **/
    simu_read_smr_bands = 0;
    simu_flush_bands = 0;
    simu_flush_band_size = 0;

    simu_n_collect_fifo = 0;
    simu_n_read_fifo = 0;
    simu_n_write_fifo = 0;
    simu_n_read_smr = 0;
    simu_time_read_fifo = 0;
    simu_time_read_smr = 0;
    simu_time_write_smr = 0;
    simu_time_write_fifo = 0;

    initSSDTable(NBLOCK_SMR_FIFO + 1);

    log_wa = fopen(log_wa_path, "w+");
    if(log_wa == NULL)
        error_exit("cannot open log: log_wa.");
//    pthread_t tid;
//    int err = pthread_create(&tid, NULL, smr_fifo_monitor_thread, NULL);
//    if (err != 0)
//    {
//        printf("[ERROR] initSSD: fail to create thread: %s\n", strerror(err));
//        exit(-1);
//    }
    /* cgroup write fifo throttle */
//    pid_t mypid = getpid();
//    int bps_write = 1048576 * 11;
//    char cmd_cg[512];
//
//    int r;
//    r = system("rm -rf /sys/fs/cgroup/blkio/smr_simu");
//    r = system("mkdir /sys/fs/cgroup/blkio/smr_simu");
//    sprintf(cmd_cg,"echo \"8:16 %d\" > /sys/fs/cgroup/blkio/smr_simu/blkio.throttle.write_bps_device",bps_write);
//    r = system(cmd_cg);
//    sprintf(cmd_cg,"echo %d > /sys/fs/cgroup/blkio/smr_simu/tasks",mypid);
//    r = system(cmd_cg);
}

/** \brief
 *  To monitor the FIFO in SMR, and do cleanning operation when idle status.
 */
static void*
smr_fifo_monitor_thread()
{
    pthread_t th = pthread_self();
    pthread_detach(th);
    int interval = 10;
    printf("Simulator Auto-clean thread [%lu], clean interval %d seconds.\n",th,interval);

    while (1)
    {
        pthread_mutex_lock(&simu_smr_fifo_mutex);
        if (!ACCESS_FLAG)
        {
            flushFIFO();
            pthread_mutex_unlock(&simu_smr_fifo_mutex);
            if (DEBUG)
                printf("[INFO] freeStrategySSD():--------after clean\n");
        }
        else
        {
            ACCESS_FLAG = 0;
            pthread_mutex_unlock(&simu_smr_fifo_mutex);
            sleep(interval);
        }
    }
    return NULL;
}

int
emu_smr_read(char *buffer, size_t size, off_t offset)
{
    pthread_mutex_lock(&simu_smr_fifo_mutex);
    DespTag		tag;
    FIFODesc    *ssd_hdr;
    long		i;
    int	        returnCode;
    long		ssd_hash;
    long		despId;
    struct timeval	tv_start,tv_stop;

    for (i = 0; i * BLKSZ < size; i++)
    {
        tag.offset = offset + i * BLKSZ;
        ssd_hash = ssdtableHashcode(tag);
        despId = ssdtableLookup(tag, ssd_hash);

        if (despId >= 0)
        {
            /* read from fifo */
            simu_n_read_fifo++;
            ssd_hdr = fifo_desp_array + despId;

            _TimerLap(&tv_start);
            returnCode = DISK_READ(fd_fifo_part, buffer, BLKSZ, ssd_hdr->despId * BLKSZ + OFF_FIFO);
            if (returnCode < 0)
            {
                printf("[ERROR] smrread():-------read from inner ssd: fd=%d, errorcode=%d, offset=%lu\n", fd_fifo_part, returnCode, ssd_hdr->despId * BLKSZ);
                exit(-1);
            }
            _TimerLap(&tv_stop);
            simu_time_read_fifo += TimerInterval_SECOND(&tv_start,&tv_stop);
        }
        else
        {
            /* read from actual smr */
            simu_n_read_smr++;
            _TimerLap(&tv_start);

            returnCode = DISK_READ(fd_smr_part, buffer, BLKSZ, offset + i * BLKSZ);
            if (returnCode < 0)
            {
                printf("[ERROR] smrread():-------read from smr disk: fd=%d, errorcode=%d, offset=%lu\n", fd_smr_part, returnCode, offset + i * BLKSZ);
                exit(-1);
            }
            _TimerLap(&tv_stop);
            simu_time_read_smr += TimerInterval_SECOND(&tv_start,&tv_stop);
        }
    }
    ACCESS_FLAG = 1;
    pthread_mutex_unlock(&simu_smr_fifo_mutex);
    return 0;
}

int
emu_smr_write(char *buffer, size_t size, off_t offset)
{
    pthread_mutex_lock(&simu_smr_fifo_mutex);
    DespTag		tag;
    long		i;
    long        target_despId;
    int		returnCode = 0;
    static struct timeval	tv_start,tv_stop;

    for (i = 0; i * BLKSZ < size; i++)
    {
        tag.offset = offset + i * BLKSZ;

        /* check if had old version in PB. */
        long old_despId = ssdtableLookup(tag, ssdtableHashcode(tag));

        if(old_despId >= 0){
            /* Write INPLACE */
            target_despId = old_despId;
        }
        else{ /* Clockwise get a new free location to store. */
            FIFODesc* ssd_hdr = clockGetDespPB();
            ssd_hdr->tag = tag;
            target_despId = ssd_hdr->despId;
        }

        _TimerLap(&tv_start);

        returnCode = DISK_WRITE(fd_fifo_part, buffer, BLKSZ, target_despId * BLKSZ + OFF_FIFO);
        if (returnCode < 0)
        {
            printf("[ERROR] smrwrite():-------write to smr disk: fd=%d, errorcode=%d, offset=%lu\n", fd_fifo_part, returnCode, offset + i * BLKSZ);
            exit(-1);
        }

        _TimerLap(&tv_stop);


        /* Update HashTable and Descriptor array */
        unsigned long ssd_hash = ssdtableHashcode(tag);
        old_despId = ssdtableUpdate(tag, ssd_hash, ssd_hdr->despId);
        if(old_despId>=0){
          FIFODesc* oldDesp = fifo_desp_array + old_despId;
          invalidDespInFIFO(oldDesp); ///invalid the old desp
        }

        simu_time_write_fifo += TimerInterval_SECOND(&tv_start,&tv_stop);
        simu_n_write_fifo ++;
    }
    ACCESS_FLAG = 1;
    pthread_mutex_unlock(&simu_smr_fifo_mutex);


    return 0;
}

static int
invalidDespInFIFO(FIFODesc* desp)
{
    desp->isValid = 0;
    global_fifo_ctrl.n_used--;
    int isHeadChanged = 0;
    while(!fifo_desp_array[global_fifo_ctrl.head].isValid && !isFIFOEmpty)
    {
        global_fifo_ctrl.head = (global_fifo_ctrl.head + 1) % (NBLOCK_SMR_FIFO + 1);
        isHeadChanged = 1;
    }
    return isHeadChanged;
}

static FIFODesc *
clockGetDespPB()
{
    FIFODesc* newDesp;
    if(isFIFOFull)
    {
        /* Log structure array is full fill */
        flushFIFO();
    }

    /* Append to tail */
    newDesp = fifo_desp_array + global_fifo_ctrl.tail;
    newDesp->isValid = 1;
    global_fifo_ctrl.tail = (global_fifo_ctrl.tail + 1) % (NBLOCK_SMR_FIFO + 1);

    return newDesp;
}

/** Persistent Buffer write-back dirty blocks using RMW Model: Read->Modify->Write. **/
static void
flushFIFO()
{
    if(global_fifo_ctrl.head == global_fifo_ctrl.tail) // Log structure array is empty.
        return;

    int     returnCode;
    struct  timeval	tv_start, tv_stop;
    long    dirty_n_inBand = 0;
    double  wtrAmp;

    FIFODesc* leader = fifo_desp_array + global_fifo_ctrl.head;

    /* Create a band-sized buffer for readind and flushing whole band bytes */
    long		band_size = GetSMRActualBandSizeFromSSD(leader->tag.offset);
    off_t		band_offset = leader->tag.offset - GetSMROffsetInBandFromSSD(leader) * BLKSZ;

    /** R **/
    /* read whole band from smr to buffer*/
    _TimerLap(&tv_start);
    returnCode = DISK_READ(fd_smr_part, BandBuffer, band_size, band_offset);
    if((returnCode) != band_size)
    {
        printf("[ERROR] flushFIFO():---------read from smr: fd=%d, errorcode=%d, offset=%lu\n",
               fd_smr_part, returnCode, band_offset);
        exit(-1);
    }
    _TimerLap(&tv_stop);
    simu_time_read_smr += TimerInterval_SECOND(&tv_start, &tv_stop);
    simu_read_smr_bands++;

//    /* temp persistence whole band from buffer to smr*/
//    if(DISK_WRITE(fd_fifo_part,BandBuffer,band_size,OFF_BAND_TMP_PERSISIT) != band_size && fsync(fd_fifo_part) < 0)
//    {
//        printf("[ERROR] flushFIFO():-------- temp persistence band: fd=%d, errorcode=%d, offset=%lu\n", fd_smr_part, returnCode, band_offset);
//        exit(-1);
//    }
    /** M **/
    /* Combine cached pages from FIFO which are belong to the same band */
    unsigned long thisBandNum = GetSMRBandNumFromSSD(leader->tag.offset);

    /** ---------------DEBUG BLOCK 1----------------------- **/
    struct timeval	tv_collect_start, tv_collect_stop;
    double collect_time;
    _TimerLap(&tv_collect_start);
    /**--------------------------------------------------- **/
    int aio_read_cnt = 0;
    long curPos = leader->despId;
    while(curPos != global_fifo_ctrl.tail)
    {
        FIFODesc* curDesp = fifo_desp_array + curPos;
        long nextPos = (curDesp->despId + 1) % (NBLOCK_SMR_FIFO + 1);

        /* If the block belongs the same band with the header of fifo. */
        if (curDesp->isValid && GetSMRBandNumFromSSD(curDesp->tag.offset) == thisBandNum)
        {
            off_t offset_inband = GetSMROffsetInBandFromSSD(curDesp);
#ifdef SIMULATOR_AIO
            struct aiocb* aio_n = aiolist + aio_read_cnt;
            aio_n->aio_fildes = fd_fifo_part;
            aio_n->aio_offset = curPos * BLKSZ;
            aio_n->aio_buf = BandBuffer + offset_inband * BLKSZ;
            aio_n->aio_nbytes = BLKSZ;
            aio_n->aio_lio_opcode = LIO_READ;
            aiocb_addr_list[aio_read_cnt] = aio_n;
            aio_read_cnt++;
#else
            _TimerLap(&tv_start);
            returnCode = DISK_READ(fd_fifo_part, BandBuffer + offset_inband * BLKSZ, BLKSZ, curPos * BLKSZ + OFF_FIFO);
            if (returnCode < 0)
            {
                printf("[ERROR] flushFIFO():-------read from PB: fd=%d, errorcode=%d, offset=%lu\n", fd_fifo_part, returnCode, curPos * BLKSZ);
                exit(-1);
            }
            _TimerLap(&tv_stop);
            simu_time_read_fifo += TimerInterval_SECOND(&tv_start,&tv_stop);
#endif // SIMULATOR_AIO
            /* clean the meta data */
            dirty_n_inBand++;
            unsigned long hash_code = ssdtableHashcode(curDesp->tag);
            ssdtableDelete(curDesp->tag, hash_code);

            int isHeadChanged = invalidDespInFIFO(curDesp); // Invalidate the current block and check if the head block get changed.
            if(isHeadChanged)
            {
                curPos = global_fifo_ctrl.head;
                continue;
            }
        }
        curPos = nextPos;
    }
    simu_n_collect_fifo += dirty_n_inBand;
#ifdef SIMULATOR_AIO
#ifndef SIMU_NO_DISK_IO
    _TimerLap(&tv_start);
    static int cnt = 0;
    printf("start aio read [%d]...\n",++cnt);
    int ret_aio = lio_listio(LIO_WAIT,aiocb_addr_list,aio_read_cnt,NULL);
    printf("end aio\n",++cnt);
    if(ret_aio < 0)
    {
        char log[128];
        sprintf(log,"Flush [%ld] times ERROR: AIO read list from FIFO Failure[%d].\n",simu_flush_bands+1,ret_aio);
//        _Log(log);
    }
    _TimerLap(&tv_stop);
    simu_time_read_fifo += TimerInterval_SECOND(&tv_start,&tv_stop);
#endif // SIMU_NO_DISK_IO
#endif // SIMULATOR_AIO
    /**--------------------------------------------------- **/
    _TimerLap(&tv_collect_stop);
    collect_time = TimerInterval_SECOND(&tv_collect_start, &tv_collect_stop);
    simu_time_collectFIFO += collect_time;
    /** ---------------DEBUG BLOCK 1----------------------- **/

    /* flush whole band to smr */
    _TimerLap(&tv_start);

    /** W **/
    returnCode = DISK_WRITE(fd_smr_part, BandBuffer, band_size, band_offset);
    if (returnCode < 0)
    {
        printf("[ERROR] flushFIFO():-------write to smr: fd=%d, errorcode=%d, offset=%lu\n", fd_smr_part, returnCode, band_offset);
        exit(-1);
    }
    _TimerLap(&tv_stop);

    simu_time_write_smr += TimerInterval_SECOND(&tv_start,&tv_stop);
    simu_flush_bands++;
    simu_flush_band_size += band_size;

    wtrAmp = (double)band_size / (dirty_n_inBand * BLKSZ);
    STT->wtrAmp_cur = wtrAmp;
    STT->WA_sum += wtrAmp;
    STT->n_RMW ++;

    char log[256];
    sprintf(log,"%f\n", wtrAmp);
    _Log(log, log_wa);
}

static unsigned long
GetSMRActualBandSizeFromSSD(unsigned long offset)
{
    long		i, size, total_size = 0;
    for (i = 0; i < band_size_num; i++)
    {
        size = BNDSZ / 2 + i * 1024 * 1024;
        if (total_size + size * num_each_size >= offset)
            return size;
        total_size += size * num_each_size;
    }
    return 0;
}

static unsigned long
GetSMRBandNumFromSSD(unsigned long offset)
{
    long		i, size, total_size = 0;
    for (i = 0; i < band_size_num; i++)
    {
        size = BNDSZ / 2 + i * 1024 * 1024;
        if (total_size + size * num_each_size > offset)
            return num_each_size * i + (offset - total_size) / size;
        total_size += size * num_each_size;
    }
    return 0;
}

static off_t
GetSMROffsetInBandFromSSD(FIFODesc * ssd_hdr)
{
    long		i, size, total_size = 0;
    unsigned long	offset = ssd_hdr->tag.offset;

    for (i = 0; i < band_size_num; i++)
    {
        size = BNDSZ / 2 + i * 1024 * 1024;
        if (total_size + size * num_each_size > offset)
            return (offset - total_size - (offset - total_size) / size * size) / BLKSZ;
        total_size += size * num_each_size;
    }

    return 0;
}

void PrintSimulatorStatistic()
{
    printf("----------------SIMULATOR------------\n");
    printf("Time:\n");
    printf("Read FIFO:\t%lf\nWrite FIFO:\t%lf\nRead SMR:\t%lf\nFlush SMR:\t%lf\n",simu_time_read_fifo, simu_time_write_fifo, simu_time_read_smr, simu_time_write_smr);
    printf("Total I/O:\t%lf\n", simu_time_read_fifo+simu_time_write_fifo+simu_time_read_smr+simu_time_write_smr);
    printf("FIFO Collect:\t%lf\n",simu_time_collectFIFO);
    printf("Block/Band Count:\n");
    printf("Read FIFO:\t%ld\nWrite FIFO:\t%ld\nFIFO Collect:\t%ld\nRead SMR:\t%ld\nFIFO Write HIT:\t%ld\n",simu_n_read_fifo, simu_n_write_fifo,simu_n_collect_fifo, simu_n_read_smr, simu_n_fifo_write_HIT);
    printf("Read Bands:\t%ld\nFlush Bands:\t%ld\nFlush BandSize:\t%ld\n",simu_read_smr_bands, simu_flush_bands, simu_flush_band_size);


    printf("WA AVG:\t%lf\n",(float)(simu_flush_band_size / BLKSZ) / STT->flush_hdd_blocks);
}

void CloseSMREmu()
{
    fclose(log_wa);
}
