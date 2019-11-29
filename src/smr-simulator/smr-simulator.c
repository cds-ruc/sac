//#include <sys/time.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <pthread.h>
//#include <memory.h>
//#include <aio.h>
//
//#include "report.h"
//#include "cache.h"
//#include "smr-simulator.h"
//#include "inner_ssd_buf_table.h"
//#include "timerUtils.h"
//#include "statusDef.h"
//int  fd_fifo_part;
//int  fd_smr_part;
//
//FIFOCtrl	    *global_fifo_ctrl;
//FIFODesc		*fifo_desp_array;
//char* BandBuffer;
//static blksize_t NSMRBands = 194180;		// smr band cnt = 194180;
//static unsigned long BNDSZ = 36*1024*1024;      // bandsize = 36MB  (18MB~36MB)
//
//static off_t SMR_DISK_OFFSET;
//
//int ACCESS_FLAG = 1;
//
//pthread_mutex_t simu_smr_fifo_mutex;
//
//static long	band_size_num;
//static long	num_each_size;
//
//static FIFODesc* getFIFODesp();
//static void* smr_fifo_monitor_thread();
//static volatile void flushFIFO();
//
//static long simu_read_smr_bands;
//static long simu_flush_bands;
//static long simu_flush_band_size;
//
//static blkcnt_t simu_n_collect_fifo;
//static blkcnt_t simu_n_read_fifo;
//static blkcnt_t simu_n_write_fifo;
//static blkcnt_t simu_n_read_smr;
//
//static blkcnt_t simu_n_fifo_write_HIT = 0;
//static double simu_time_read_fifo;
//static double simu_time_read_smr;
//static double simu_time_write_smr;
//static double simu_time_write_fifo;
//
//static double simu_time_collectFIFO = 0; /** To monitor the efficent of read all blocks in same band **/
//
//static void* smr_fifo_monitor_thread();
//
//static void addIntoUseArray(FIFODesc* newDesp);
//static void removeFromUsedArray(FIFODesc* desp);
//static void addIntoFreeArray(FIFODesc* newDesp);
//static FIFODesc* getFreeDesp();
//
//static unsigned long GetSMRActualBandSizeFromSSD(unsigned long offset);
//static unsigned long GetSMRBandNumFromSSD(unsigned long offset);
//static off_t GetSMROffsetInBandFromSSD(FIFODesc * ssd_hdr);
//
///** AIO related for blocks collected in FIFO **/
//#define max_aio_count 6000
//struct aiocb aiolist[max_aio_count];
//struct aiocb* aiocb_addr_list[max_aio_count];/* >= band block size */
//
//static FIFODesc* getFreeDesp();
///*
// * init inner ssd buffer hash table, strategy_control, buffer, work_mem
// */
//void
//initFIFOCache()
//{
//    /* initialliz related constants */
//    SMR_DISK_OFFSET = NBLOCK_SMR_PB * BLKSZ;
//    band_size_num = BNDSZ / 1024 / 1024 / 2 + 1;
//    num_each_size = NSMRBands / band_size_num;
//
//    global_fifo_ctrl = (FIFOCtrl*) malloc(sizeof(FIFOCtrl));
//    global_fifo_ctrl->first_useId = global_fifo_ctrl->last_useId = -1;
//    global_fifo_ctrl->first_freeId = 0;
//    global_fifo_ctrl->last_freeId = NBLOCK_SMR_PB - 1;
//    global_fifo_ctrl->n_used = 0;
//
//    fifo_desp_array = (FIFODesc *) malloc(sizeof(FIFODesc) * NBLOCK_SMR_PB);
//    FIFODesc* fifo_hdr = fifo_desp_array;
//    long i;
//    for (i = 0; i < NBLOCK_SMR_PB; fifo_hdr++, i++)
//    {
//        fifo_hdr->despId = i;
//        fifo_hdr->next_freeId = (i==NBLOCK_SMR_PB-1) ? -1 : i + 1;
//        fifo_hdr->pre_useId = fifo_hdr->next_useId = -1;
//    }
//
//    posix_memalign(&BandBuffer, 512, sizeof(char) * BNDSZ);
//
//    pthread_mutex_init(&simu_smr_fifo_mutex, NULL);
//
//    /** AIO related **/
//
//    /** statistic related **/
//    simu_read_smr_bands = 0;
//    simu_flush_bands = 0;
//    simu_flush_band_size = 0;
//
//    simu_n_collect_fifo = 0;
//    simu_n_read_fifo = 0;
//    simu_n_write_fifo = 0;
//    simu_n_read_smr = 0;
//    simu_time_read_fifo = 0;
//    simu_time_read_smr = 0;
//    simu_time_write_smr = 0;
//    simu_time_write_fifo = 0;
//
//    initSSDTable(NBLOCK_SMR_PB);
//
//    pthread_t tid;
//    int err = pthread_create(&tid, NULL, smr_fifo_monitor_thread, NULL);
//    if (err != 0)
//    {
//        printf("[ERROR] initSSD: fail to create thread: %s\n", strusr_warning(err));
//    }
//}
//
///** \brief
// *  To monitor the FIFO in SMR, and do cleanning operation when idle status.
// */
//static void*
//smr_fifo_monitor_thread()
//{
//    while (1)
//    {
//        pthread_mutex_lock(&simu_smr_fifo_mutex);
//        if (!ACCESS_FLAG)
//        {
//            flushFIFO();
//            pthread_mutex_unlock(&simu_smr_fifo_mutex);
//            if (DEBUG)
//                printf("[INFO] freeStrategySSD():--------after clean\n");
//        }
//        else
//        {
//            ACCESS_FLAG = 0;
//            pthread_mutex_unlock(&simu_smr_fifo_mutex);
//            sleep(5);
//        }
//    }
//    return NULL;
//}
//
//int
//simu_smr_read(char *buffer, size_t size, off_t offset)
//{
//    pthread_mutex_lock(&simu_smr_fifo_mutex);
//    DespTag		tag;
//    FIFODesc    *ssd_hdr;
//    long		i;
//    int	        returnCode;
//    long		ssd_hash;
//    long		despId;
//    struct timeval	tv_start,tv_stop;
//
//    for (i = 0; i * BLKSZ < size; i++)
//    {
//        tag.offset = offset + i * BLKSZ;
//        ssd_hash = ssdtableHashcode(&tag);
//        despId = ssdtableLookup(&tag, ssd_hash);
//
//        if (despId >= 0)
//        {
//            /* read from fifo */
//            simu_n_read_fifo++;
//            ssd_hdr = fifo_desp_array + despId;
//
//            _TimerLap(&tv_start);
//            returnCode = pread(fd_fifo_part, buffer, BLKSZ, ssd_hdr->despId * BLKSZ);
//            if (returnCode < 0)
//            {
//                printf("[ERROR] smrread():-------read from inner ssd: fd=%d, errorcode=%d, offset=%lu\n", fd_fifo_part, returnCode, ssd_hdr->despId * BLKSZ);
//                exit(-1);
//            }
//            _TimerLap(&tv_stop);
//            simu_time_read_fifo += TimerInterval_SECOND(&tv_start,&tv_stop);
//        }
//        else
//        {
//            /* read from actual smr */
//            simu_n_read_smr++;
//            _TimerLap(&tv_start);
//
//            returnCode = pread(fd_smr_part, buffer, BLKSZ, offset + i * BLKSZ);
//            if (returnCode < 0)
//            {
//                printf("[ERROR] smrread():-------read from smr disk: fd=%d, errorcode=%d, offset=%lu\n", fd_smr_part, returnCode, offset + i * BLKSZ);
//                exit(-1);
//            }
//            _TimerLap(&tv_stop);
//            simu_time_read_smr += TimerInterval_SECOND(&tv_start,&tv_stop);
//        }
//    }
//    ACCESS_FLAG = 1;
//    pthread_mutex_unlock(&simu_smr_fifo_mutex);
//    return 0;
//}
//
//int
//simu_smr_write(char *buffer, size_t size, off_t offset)
//{
//    pthread_mutex_lock(&simu_smr_fifo_mutex);
//    DespTag		tag;
//    FIFODesc        *ssd_hdr;
//    long		i;
//    int		returnCode = 0;
//    long		ssd_hash;
//    long		despId;
//    struct timeval	tv_start,tv_stop;
//
//    for (i = 0; i * BLKSZ < size; i++)
//    {
//        tag.offset = offset + i * BLKSZ;
//        ssd_hash = ssdtableHashcode(&tag);
//        despId = ssdtableLookup(&tag, ssd_hash);
//        if (despId >= 0)
//        {
//            ssd_hdr = fifo_desp_array + despId;
//            simu_n_fifo_write_HIT++;
//        }
//        else
//        {
//            ssd_hdr = getFIFODesp();    // If there is no spare desp for allocate, do flush.
//        }
//        ssd_hdr->tag = tag;
//        ssdtableInsert(&tag, ssd_hash, ssd_hdr->despId);
//
//        _TimerLap(&tv_start);
//        returnCode = pwrite(fd_fifo_part, buffer, BLKSZ, ssd_hdr->despId * BLKSZ);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] smrwrite():-------write to smr disk: fd=%d, errorcode=%d, offset=%lu\n", fd_fifo_part, returnCode, offset + i * BLKSZ);
//            exit(-1);
//        }
//        //returnCode = fsync(inner_ssd_fd);
////        if (returnCode < 0)
////        {
////            printf("[ERROR] smrwrite():----------fsync\n");
////            exit(-1);
////        }
//
//        _TimerLap(&tv_stop);
//        simu_time_write_fifo += TimerInterval_SECOND(&tv_start,&tv_stop);
//        simu_n_write_fifo++;
//    }
//    ACCESS_FLAG = 1;
//    pthread_mutex_unlock(&simu_smr_fifo_mutex);
//    return 0;
//}
//
//static void
//removeFromUsedArray(FIFODesc* desp)
//{
//    if(desp->pre_useId < 0)
//    {
//        global_fifo_ctrl->first_useId = desp->next_useId;
//    }
//    else
//    {
//        fifo_desp_array[desp->pre_useId].next_useId = desp->next_useId;
//    }
//
//    if(desp->next_useId < 0)
//    {
//        global_fifo_ctrl->last_useId = desp->pre_useId;
//    }
//    else
//    {
//        fifo_desp_array[desp->next_useId].pre_useId = desp->pre_useId;
//    }
//    desp->pre_useId = desp->next_useId = -1;
//    global_fifo_ctrl->n_used--;
//}
//
//static void
//addIntoUseArray(FIFODesc* newDesp)
//{
//    if(global_fifo_ctrl->first_useId < 0)
//    {
//        global_fifo_ctrl->first_useId = global_fifo_ctrl->last_useId = newDesp->despId;
//    }
//    else
//    {
//        FIFODesc* tailDesp = fifo_desp_array + global_fifo_ctrl->last_useId;
//        tailDesp->next_useId = newDesp->despId;
//        newDesp->pre_useId = tailDesp->despId;
//        global_fifo_ctrl->last_useId = newDesp->despId;
//    }
//    global_fifo_ctrl->n_used++;
//}
//
//static void
//addIntoFreeArray(FIFODesc* newDesp)
//{
//    if(global_fifo_ctrl->first_freeId < 0)
//    {
//        global_fifo_ctrl->first_freeId = global_fifo_ctrl->last_freeId = newDesp->despId;
//    }
//    else
//    {
//        FIFODesc* tailDesp = fifo_desp_array + global_fifo_ctrl->last_freeId;
//        tailDesp->next_freeId = newDesp->despId;
//        global_fifo_ctrl->last_freeId = newDesp->despId;
//    }
//}
//
//static FIFODesc*
//getFreeDesp()
//{
//    if(global_fifo_ctrl->first_freeId < 0)
//        return NULL;
//    FIFODesc* freeDesp = fifo_desp_array + global_fifo_ctrl->first_freeId;
//    global_fifo_ctrl->first_freeId = freeDesp->next_freeId;
//
//    freeDesp->next_freeId = -1;
//    return freeDesp;
//}
//
//static FIFODesc *
//getFIFODesp()
//{
//    FIFODesc* newDesp;
//    /* allocate free block */
//    while((newDesp = getFreeDesp()) == NULL)
//    {
//        /* no more free blocks in fifo, do flush*/
//        flushFIFO();
//    }
//
//    /* add into used fifo array */
//    addIntoUseArray(newDesp);
//
//    return newDesp;
//}
//
//static volatile void
//flushFIFO()
//{
//    if(global_fifo_ctrl->first_useId < 0)
//        return;
//
//    int     returnCode;
//    struct  timeval	tv_start, tv_stop;
//    long    dirty_n_inBand = 0;
//    double  wtrAmp;
//
//    FIFODesc* target = fifo_desp_array + global_fifo_ctrl->first_useId;
//
//    /* Create a band-sized buffer for readind and flushing whole band bytes */
//    long		band_size = GetSMRActualBandSizeFromSSD(target->tag.offset);
//    off_t		band_offset = target->tag.offset - GetSMROffsetInBandFromSSD(target) * BLKSZ;
//
//    /* read whole band from smr to buffer*/
//    _TimerLap(&tv_start);
//    returnCode = pread(fd_smr_part, BandBuffer, band_size,band_offset);
//    if (returnCode < 0)
//    {
//        printf("[ERROR] flushSSD():---------read from smr: fd=%d, errorcode=%d, offset=%lu\n", fd_smr_part, returnCode, band_offset);
//        exit(-1);
//    }
//    _TimerLap(&tv_stop);
//    simu_time_read_smr += TimerInterval_SECOND(&tv_start,&tv_stop);
//    simu_read_smr_bands++;
//
//    /* Combine cached pages from FIFO which are belong to the same band */
//    unsigned long BandNum = GetSMRBandNumFromSSD(target->tag.offset);
//
//    /** ---------------DEBUG BLOCK----------------------- **/
//    struct timeval	tv_collect_start, tv_collect_stop;
//    double collect_time;
//    _TimerLap(&tv_collect_start);
//    /**--------------------------------------------------- **/
//    int aio_read_cnt = 0;
//    long curPos = target->despId;
//    while(curPos >= 0)
//    {
//        FIFODesc* curDesp = fifo_desp_array + curPos;
//        long nextPos = curDesp->next_useId;
//        if (GetSMRBandNumFromSSD(curDesp->tag.offset) == BandNum)
//        {
//            /* The block belongs the same band with the header of fifo. */
//            off_t offset_inband = GetSMROffsetInBandFromSSD(curDesp);
//
//#ifdef SIMULATOR_AIO
//            struct aiocb* aio_n = aiolist + aio_read_cnt;
//            aio_n->aio_fildes = fd_fifo_part;
//            aio_n->aio_offset = curPos * BLKSZ;
//            aio_n->aio_buf = BandBuffer + offset_inband * BLKSZ;
//            aio_n->aio_nbytes = BLKSZ;
//            aio_n->aio_lio_opcode = LIO_READ;
//            aiocb_addr_list[aio_read_cnt] = aio_n;
//            aio_read_cnt++;
//#else
//            _TimerLap(&tv_start);
//            returnCode = pread(fd_fifo_part, BandBuffer + offset_inband * BLKSZ, BLKSZ, curPos * BLKSZ);
//            if (returnCode < 0)
//            {
//                printf("[ERROR] flushSSD():-------read from inner ssd: fd=%d, errorcode=%d, offset=%lu\n", fd_fifo_part, returnCode, curPos * BLKSZ);
//                exit(-1);
//            }
//            _TimerLap(&tv_stop);
//            simu_time_read_fifo += TimerInterval_SECOND(&tv_start,&tv_stop);
//#endif // SIMULATOR_AIO
//            /* clean the meta data */
//            dirty_n_inBand++;
//            unsigned long hash_code = ssdtableHashcode(&curDesp->tag);
//            ssdtableDelete(&curDesp->tag, hash_code);
//
//            removeFromUsedArray(curDesp);
//            addIntoFreeArray(curDesp);
//        }
//        curPos = nextPos;
//    }
//    simu_n_collect_fifo += dirty_n_inBand;
//#ifdef SIMULATOR_AIO
//    _TimerLap(&tv_start);
//    int ret_aio = lio_listio(LIO_WAIT,aiocb_addr_list,aio_read_cnt,NULL);
//    if(ret_aio < 0)
//    {
//        char log[128];
//        sprintf(log,"Flush [%ld] times ERROR: AIO read list from FIFO Failure[%d].\n",simu_flush_bands+1,ret_aio);
//        _Log(log);
//    }
//    _TimerLap(&tv_stop);
//    simu_time_read_fifo += TimerInterval_SECOND(&tv_start,&tv_stop);
//#endif // SIMULATOR_AIO
//    /**--------------------------------------------------- **/
//    _TimerLap(&tv_collect_stop);
//    collect_time = TimerInterval_SECOND(&tv_collect_start, &tv_collect_stop);
//    simu_time_collectFIFO += collect_time;
//    /** ---------------DEBUG BLOCK----------------------- **/
//
//    /* flush whole band to smr */
//    _TimerLap(&tv_start);
//
//    returnCode = pwrite(fd_smr_part, BandBuffer, band_size, band_offset);
//    if (returnCode < 0)
//    {
//        printf("[ERROR] flushSSD():-------write to smr: fd=%d, errorcode=%d, offset=%lu\n", hdd_fd, returnCode, band_offset);
//        exit(-1);
//    }
//    _TimerLap(&tv_stop);
//
//    simu_time_write_smr += TimerInterval_SECOND(&tv_start,&tv_stop);
//    simu_flush_bands++;
//    simu_flush_band_size += band_size;
//
//    wtrAmp = (double)band_size / (dirty_n_inBand * BLKSZ);
//    char log[256];
//    sprintf(log,"flush [%ld] times from fifo to smr,collect time:%lf, cnt=%ld,WtrAmp = %lf\n",simu_flush_bands,collect_time,dirty_n_inBand,wtrAmp);
//    _Log(log);
//}
//
//
//static unsigned long
//GetSMRActualBandSizeFromSSD(unsigned long offset)
//{
//
//    long		i, size, total_size = 0;
//    for (i = 0; i < band_size_num; i++)
//    {
//        size = BNDSZ / 2 + i * 1024 * 1024;
//        if (total_size + size * num_each_size >= offset)
//            return size;
//        total_size += size * num_each_size;
//    }
//
//    return 0;
//}
//
//static unsigned long
//GetSMRBandNumFromSSD(unsigned long offset)
//{
//    long		i, size, total_size = 0;
//    for (i = 0; i < band_size_num; i++)
//    {
//        size = BNDSZ / 2 + i * 1024 * 1024;
//        if (total_size + size * num_each_size > offset)
//            return num_each_size * i + (offset - total_size) / size;
//        total_size += size * num_each_size;
//    }
//
//    return 0;
//}
//
//static off_t
//GetSMROffsetInBandFromSSD(FIFODesc * ssd_hdr)
//{
//    long		i, size, total_size = 0;
//    unsigned long	offset = ssd_hdr->tag.offset;
//
//    for (i = 0; i < band_size_num; i++)
//    {
//        size = BNDSZ / 2 + i * 1024 * 1024;
//        if (total_size + size * num_each_size > offset)
//            return (offset - total_size - (offset - total_size) / size * size) / BLKSZ;
//        total_size += size * num_each_size;
//    }
//
//    return 0;
//}
//
//void PrintSimulatorStatistic()
//{
//    printf("----------------SIMULATOR------------\n");
//    printf("Time:\n");
//    printf("Read FIFO:\t%lf\nWrite FIFO:\t%lf\nRead SMR:\t%lf\nFlush SMR:\t%lf\n",simu_time_read_fifo, simu_time_write_fifo, simu_time_read_smr, simu_time_write_smr);
//    printf("Total I/O:\t%lf\n", simu_time_read_fifo+simu_time_write_fifo+simu_time_read_smr+simu_time_write_smr);
//    printf("FIFO Collect:\t%lf\n",simu_time_collectFIFO);
//    printf("Block/Band Count:\n");
//    printf("Read FIFO:\t%ld\nWrite FIFO:\t%ld\nFIFO Collect:\t%ld\nRead SMR:\t%ld\nFIFO Write HIT:\t%ld\n",simu_n_read_fifo, simu_n_write_fifo,simu_n_collect_fifo, simu_n_read_smr, simu_n_fifo_write_HIT);
//    printf("Read Bands:\t%ld\nFlush Bands:\t%ld\nFlush BandSize:\t%ld\n",simu_read_smr_bands, simu_flush_bands, simu_flush_band_size);
//
//
//    printf("Total WrtAmp:\t%lf\n",(double)simu_flush_band_size / (simu_n_write_fifo * BLKSZ));
//}
