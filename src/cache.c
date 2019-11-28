#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>

#include "trace2call.h"
#include "timerUtils.h"
#include "cache.h"
#include "hashtable_utils.h"

#include "strategy/strategies.h"

#include "smr-simulator/simulator_v2.h"

#include "shmlib.h"
#include "report.h"

#include "strategy/costmodel.h"

SSDBufDespCtrl*     ssd_buf_desp_ctrl;
SSDBufDesp*         ssd_buf_desps;

/* If Defined R/W Cache Space Static Allocated */
double Proportion_Dirty;
blkcnt_t Max_Dirty_Cache, Max_Clean_Cache;

static int          init_SSDDescriptorBuffer();
static int          init_StatisticObj();
static void         flushSSDBuffer(SSDBufDesp * ssd_buf_hdr);
static SSDBufDesp*  allocSSDBuf(SSDBufTag ssd_buf_tag, bool * found, int alloc4What, int * isCallBack);
static SSDBufDesp*  pop_freebuf();
static int          push_freebuf(SSDBufDesp* freeDesp);

static int          initStrategySSDBuffer();
static long         Strategy_Desp_LogOut();
static int          Strategy_Desp_HitIn(SSDBufDesp* desp);
static int         Strategy_Desp_LogIn(SSDBufDesp* desp);
//#define isSamebuf(SSDBufTag tag1, SSDBufTag tag2) (tag1 == tag2)
#define CopySSDBufTag(objectTag,sourceTag) (objectTag = sourceTag)
#define IsDirty(flag) ( (flag & SSD_BUF_DIRTY) != 0 )
#define IsClean(flag) ( (flag & SSD_BUF_DIRTY) == 0 )

void                _LOCK(pthread_mutex_t* lock);
void                _UNLOCK(pthread_mutex_t* lock);

/* stopwatch */
static timeval tv_start, tv_stop;
static timeval tv_bastart, tv_bastop;
static timeval tv_cmstart, tv_cmstop;
int IsHit;
microsecond_t msec_r_hdd,msec_w_hdd,msec_r_ssd,msec_w_ssd,msec_bw_hdd=0;

/* Device I/O operation with Timer */
static int dev_pread(int fd, void* buf,size_t nbytes,off_t offset);
static int dev_pwrite(int fd, void* buf,size_t nbytes,off_t offset);
static int dev_simu_read(void* buf,size_t nbytes,off_t offset);
static int dev_simu_write(void* buf,size_t nbytes,off_t offset);

static char* ssd_buffer;

extern struct RuntimeSTAT* STT;
extern struct InitUsrInfo UsrInfo;



/*
 * init buffer hash table, strategy_control, buffer, work_mem
 */
void
CacheLayer_Init()
{
    int r_initdesp          =   init_SSDDescriptorBuffer();
    int r_initstrategybuf   =   initStrategySSDBuffer();
    int r_initbuftb         =   HashTab_Init();
    int r_initstt           =   init_StatisticObj();
    int r_initTSwitcher     =   CM_Init();

    printf("init_Strategy: %d, init_table: %d, init_desp: %d, inti_Stt: %d, init_TSwitcher: %d\n",
           r_initstrategybuf, r_initbuftb, r_initdesp, r_initstt, r_initTSwitcher);

    if(r_initdesp==-1 || r_initstrategybuf==-1 || r_initbuftb==-1 || r_initstt==-1 || r_initTSwitcher == -1)
        exit(EXIT_FAILURE);

    int returnCode = posix_memalign(&ssd_buffer, 512, sizeof(char) * BLKSZ);
    if (returnCode < 0)
    {
        printf("[ERROR] flushSSDBuffer():--------posix memalign\n");
        exit(EXIT_FAILURE);
    }

#ifdef CACHE_PROPORTIOIN_STATIC
    Max_Dirty_Cache = (blkcnt_t)((double)NBLOCK_SSD_CACHE * Proportion_Dirty);
    Max_Clean_Cache = NBLOCK_SSD_CACHE - Max_Dirty_Cache;
#endif // CACHE_PROPORTIOIN_STATIC
}

static int
init_SSDDescriptorBuffer()
{
    int stat = multi_SHM_lock_n_check("LOCK_SSDBUF_DESP");
    if(stat == 0)
    {
        ssd_buf_desp_ctrl = (SSDBufDespCtrl*)multi_SHM_alloc(SHM_SSDBUF_DESP_CTRL,sizeof(SSDBufDespCtrl));
        ssd_buf_desps = (SSDBufDesp *)multi_SHM_alloc(SHM_SSDBUF_DESPS,sizeof(SSDBufDesp) * NBLOCK_SSD_CACHE);

        ssd_buf_desp_ctrl->n_usedssd = 0;
        ssd_buf_desp_ctrl->first_freessd = 0;
        multi_SHM_mutex_init(&ssd_buf_desp_ctrl->lock);

        long i;
        SSDBufDesp  *ssd_buf_hdr = ssd_buf_desps;
        for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr++, i++)
        {
            ssd_buf_hdr->serial_id = i;
            ssd_buf_hdr->ssd_buf_id = i;
            ssd_buf_hdr->ssd_buf_flag = 0;
            ssd_buf_hdr->next_freessd = i + 1;
            multi_SHM_mutex_init(&ssd_buf_hdr->lock);
        }
        ssd_buf_desps[NBLOCK_SSD_CACHE - 1].next_freessd = -1;
    }
    else
    {
        ssd_buf_desp_ctrl = (SSDBufDespCtrl *)multi_SHM_get(SHM_SSDBUF_DESP_CTRL,sizeof(SSDBufDespCtrl));
        ssd_buf_desps = (SSDBufDesp *)multi_SHM_get(SHM_SSDBUF_DESPS,sizeof(SSDBufDesp) * NBLOCK_SSD_CACHE);
    }
    multi_SHM_unlock("LOCK_SSDBUF_DESP");
    return stat;
}

static int
init_StatisticObj()
{
    STT->hitnum_s = 0;
    STT->hitnum_r = 0;
    STT->hitnum_w = 0;
    STT->load_ssd_blocks = 0;
    STT->flush_ssd_blocks = 0;
    STT->load_hdd_blocks = 0;
    STT->flush_hdd_blocks = 0;
    STT->flush_clean_blocks = 0;

    STT->time_read_hdd = 0.0;
    STT->time_write_hdd = 0.0;
    STT->time_read_ssd = 0.0;
    STT->time_write_ssd = 0.0;
    STT->hashmiss_sum = 0;
    STT->hashmiss_read = 0;
    STT->hashmiss_write = 0;

    STT->wt_hit_rd = STT->rd_hit_wt = 0;
    STT->incache_n_clean = STT->incache_n_dirty = 0;
#ifdef NO_READ_CACHE
    STT->incache_n_clean = -1;
#endif
    return 0;
}

static void
flushSSDBuffer(SSDBufDesp * ssd_buf_hdr)
{
    if (IsClean(ssd_buf_hdr->ssd_buf_flag))
    {
        STT->flush_clean_blocks++;
        CM_Reg_EvictBlk(ssd_buf_hdr->ssd_buf_tag, ssd_buf_hdr->ssd_buf_flag, 0);
        return;
    }

    dev_pread(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
    msec_r_ssd = TimerInterval_MICRO(&tv_start,&tv_stop);
    STT->time_read_ssd += Mirco2Sec(msec_r_ssd);
    STT->load_ssd_blocks++;
#ifdef SIMULATION
    dev_simu_write(ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_tag.offset);
#else
    dev_pwrite(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_tag.offset);
#endif
    msec_w_hdd = TimerInterval_MICRO(&tv_start,&tv_stop);
    STT->time_write_hdd += Mirco2Sec(msec_w_hdd);
    STT->flush_hdd_blocks++;

    CM_Reg_EvictBlk(ssd_buf_hdr->ssd_buf_tag, ssd_buf_hdr->ssd_buf_flag, msec_w_hdd + msec_r_ssd);

    static char log[256];
    static unsigned long cnt = 0;
    cnt ++;
    sprintf(log,"%ld, %ld\n", cnt, msec_w_hdd);
    _Log(log, log_lat_pb);
}

int ResizeCacheUsage()
{
    blksize_t needEvictCnt = STT->cacheUsage - STT->cacheLimit;
    if(needEvictCnt <= 0)
        return 0;

    while(needEvictCnt-- > 0)
    {
        long unloadId = Strategy_Desp_LogOut();
        SSDBufDesp* ssd_buf_hdr = &ssd_buf_desps[unloadId];

        // TODO Flush
        _LOCK(&ssd_buf_hdr->lock);
        flushSSDBuffer(ssd_buf_hdr);

        ssd_buf_hdr->ssd_buf_flag &= ~(SSD_BUF_VALID | SSD_BUF_DIRTY);
        ssd_buf_hdr->ssd_buf_tag.offset = -1;
        _UNLOCK(&ssd_buf_hdr->lock);

        _LOCK(&ssd_buf_desp_ctrl->lock);
        ssd_buf_hdr->next_freessd = ssd_buf_desp_ctrl->first_freessd;
        ssd_buf_desp_ctrl->first_freessd = ssd_buf_hdr->serial_id;
        _UNLOCK(&ssd_buf_desp_ctrl->lock);
    }
    return 0;
}

static void flagOp(SSDBufDesp * ssd_buf_hdr, int opType)
{
    ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID;
    if(opType)
    {
        // write operation
        ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_DIRTY;
    }

}

static SSDBufDesp*
allocSSDBuf(SSDBufTag ssd_buf_tag, bool * found, int alloc4What, int * isCallBack)
{

    /* Lookup if already cached. */
    SSDBufDesp      *ssd_buf_hdr; //returned value.
    unsigned long   ssd_buf_hash = HashTab_GetHashCode(ssd_buf_tag);
    long            ssd_buf_id = HashTab_Lookup(ssd_buf_tag, ssd_buf_hash);

    /* Cache HIT IN */
    if (ssd_buf_id >= 0)
    {
        ssd_buf_hdr = &ssd_buf_desps[ssd_buf_id];
        _LOCK(&ssd_buf_hdr->lock);

        /* count wt_hit_rd and rd_hit_wt */
        if (alloc4What == 0 && IsDirty(ssd_buf_hdr->ssd_buf_flag))
            STT->rd_hit_wt ++;
        else if (alloc4What != 0 && IsClean(ssd_buf_hdr->ssd_buf_flag))
        {
            STT->wt_hit_rd ++;
            STT->incache_n_clean --;
            STT->incache_n_dirty ++;
        }

        flagOp(ssd_buf_hdr,alloc4What);   // tell strategy block's flag changes.
        Strategy_Desp_HitIn(ssd_buf_hdr); // need lock.

        STT->hitnum_s++;
        *found = 1;

        return ssd_buf_hdr;

    }

    /* Cache MISS */
    *found = 0;
    *isCallBack = CM_TryCallBack(ssd_buf_tag);
    enum_t_vict suggest_type = ENUM_B_Any;

#ifdef CACHE_PROPORTIOIN_STATIC
#ifdef NO_READ_CACHE
    if(alloc4What == 0)
    {
        static char read_buf[8192];
        return read_buf;
    }
#endif
    if (STT->incache_n_clean >= Max_Clean_Cache)
    {
        suggest_type = ENUM_B_Clean;
        goto FLAG_CACHEOUT;
    }
    else if (STT->incache_n_dirty >= Max_Dirty_Cache)
    {
        suggest_type = ENUM_B_Dirty;
        goto FLAG_CACHEOUT;
    }
#endif // CACHE_PROPORTIOIN_STATIC

//    _LOCK(&ssd_buf_desp_ctrl->lock);
    ssd_buf_hdr = pop_freebuf();
//    _UNLOCK(&ssd_buf_desp_ctrl->lock);

    if (ssd_buf_hdr != NULL)
    {
        _LOCK(&ssd_buf_hdr->lock);
        // if there is free SSD buffer left.
    }
    else
    {
        /** When there is NO free SSD space for cache **/
        // TODO Choose a buffer by strategy/
#ifdef R3BALANCER_ON
        if((STT->flush_hdd_blocks + STT->flush_clean_blocks) >= TS_StartSize)
        {   /* Decide victim type by T_Switcher */
            int type = CM_CHOOSE();
            if(type == 0)
                suggest_type = ENUM_B_Clean;
            else
                suggest_type = ENUM_B_Dirty;
        }
#endif // R3BALANCER_ON

FLAG_CACHEOUT:
        if(STT->incache_n_clean == 0)
            suggest_type = ENUM_B_Dirty;
        else if(STT->incache_n_dirty == 0)
            suggest_type = ENUM_B_Clean;

        static int max_n_batch = 1024;
        long buf_despid_array[max_n_batch];
        int n_evict;
        switch (EvictStrategy)
        {
//            case PORE_PLUS_V2 :
//                n_evict = LogOut_poreplus_v2(buf_despid_array, max_n_batch, suggest_type);
//                break;
            case PAUL :
                n_evict = LogOut_PAUL(buf_despid_array, max_n_batch, suggest_type);
                break;
//            case OLDPORE :
//                n_evict = LogOut_oldpore(buf_despid_array, max_n_batch, suggest_type);
//                break;
//            case PORE:
//                int i;
//                n_evict = 64;
//                for(i=0;i<n_evict;i++)
//                {
//                    buf_despid_array[i] = LogOutDesp_pore();
//                }
//                break;
            case MOST :
                n_evict = LogOut_most(buf_despid_array, max_n_batch);
                break;
            case MOST_RW :
                n_evict = LogOut_most_rw(buf_despid_array,max_n_batch,suggest_type);
                break;
            case LRU_private:
                n_evict = Unload_Buf_LRU_private(buf_despid_array, max_n_batch);
                break;
            case LRU_rw:
                n_evict = Unload_Buf_LRU_rw(buf_despid_array, max_n_batch,suggest_type);
                break;
            default:
                usr_warning("Current cache algorithm dose not support batched process.");
                exit(EXIT_FAILURE);
        }

        int k = 0;
        while(k < n_evict)
        {
            long out_despId = buf_despid_array[k];
            ssd_buf_hdr = &ssd_buf_desps[out_despId];

            _LOCK(&ssd_buf_hdr->lock);
            // Clear Hashtable item.
            SSDBufTag oldtag = ssd_buf_hdr->ssd_buf_tag;
            unsigned long hash = HashTab_GetHashCode(oldtag);
            HashTab_Delete(oldtag,hash);
            // TODO Flush
            flushSSDBuffer(ssd_buf_hdr);

            IsDirty(ssd_buf_hdr->ssd_buf_flag) ? STT->incache_n_dirty -- : STT->incache_n_clean -- ;
            ssd_buf_hdr->ssd_buf_flag &= ~(SSD_BUF_VALID | SSD_BUF_DIRTY);
            // Push back to free list
            push_freebuf(ssd_buf_hdr);
            k++;
        }
        ssd_buf_hdr = pop_freebuf();
//
//#else
///* NO Longer support to evict only one blocks per time. */
////        long out_despId;
////        out_despId = Strategy_Desp_LogOut(flag); //need look
////        ssd_buf_hdr = &ssd_buf_desps[out_despId];
////        _LOCK(&ssd_buf_hdr->lock);
////        // Clear Hashtable item.
////        SSDBufTag oldtag = ssd_buf_hdr->ssd_buf_tag;
////        unsigned long hash = HashTab_GetHashCode(oldtag);
////        HashTab_Delete(oldtag,hash);
////        // TODO Flush
////        flushSSDBuffer(ssd_buf_hdr);
////
////        IsDirty(ssd_buf_hdr->ssd_buf_flag) ? STT->incache_n_dirty -- : STT->incache_n_clean -- ;
////        ssd_buf_hdr->ssd_buf_flag &= ~(SSD_BUF_VALID | SSD_BUF_DIRTY);
//
//#endif // WRITE_IN_BATCH
    }

    flagOp(ssd_buf_hdr,alloc4What);
    CopySSDBufTag(ssd_buf_hdr->ssd_buf_tag,ssd_buf_tag);

    HashTab_Insert(ssd_buf_tag, ssd_buf_hash, ssd_buf_hdr->serial_id);
    Strategy_Desp_LogIn(ssd_buf_hdr);
    IsDirty(ssd_buf_hdr->ssd_buf_flag) ? STT->incache_n_dirty ++ : STT->incache_n_clean ++ ;

    return ssd_buf_hdr;
}

static int
initStrategySSDBuffer()
{
    switch(EvictStrategy)
    {
    case LRU_private:
        return initSSDBufferFor_LRU_private();
    case LRU_rw:
        return initSSDBufferFor_LRU_rw();
//        case Most:              return initSSDBufferForMost();
//    case PORE:
//        return InitPORE();
//    case PORE_PLUS:
//        return InitPORE_plus();
//    case PORE_PLUS_V2:
//        return Init_poreplus_v2();
    case PAUL:
        return Init_PUAL();
//    case OLDPORE:
//        return Init_oldpore();
    case MOST:
        return Init_most();
    case MOST_RW:
        return Init_most_rw();

    }
    return -1;
}

static long
Strategy_Desp_LogOut(unsigned flag)
{
    STT->cacheUsage--;
    switch(EvictStrategy)
    {
//        case LRU_global:        return Unload_LRUBuf();
    case LRU_private:
        usr_warning("LRU wrong time function revoke, please use BATHCH configure.\n");
    case LRU_rw:
        usr_warning("LRU_RW wrong time function revoke\n");
//       case Most:              return LogOutDesp_most();
//    case PORE:
//        return LogOutDesp_pore();
//    case PORE_PLUS:
//        return LogOutDesp_pore_plus();
//    case PORE_PLUS_V2:
//        usr_warning("PORE_PLUS_V2 wrong time function revoke\n");
    case PAUL:
        usr_warning("PAUL wrong time function revoke\n");
    case MOST:
        usr_warning("MOST wrong time function revoke\n");
    }
    return -1;
}

static int
Strategy_Desp_HitIn(SSDBufDesp* desp)
{
    switch(EvictStrategy)
    {
//        case LRU_global:        return hitInLRUBuffer(desp->serial_id);
    case LRU_private:
        return hitInBuffer_LRU_private(desp->serial_id);
    case LRU_rw:
        return hitInBuffer_LRU_rw(desp->serial_id, desp->ssd_buf_flag);
//        case LRU_batch:         return hitInBuffer_LRU_batch(desp->serial_id);
//        case Most:              return HitMostBuffer();
//    case PORE:
//        return HitPoreBuffer(desp->serial_id, desp->ssd_buf_flag);
//    case PORE_PLUS:
//        return HitPoreBuffer_plus(desp->ssd_buf_flag, desp->ssd_buf_flag);
//    case PORE_PLUS_V2:
//        return Hit_poreplus_v2(desp->serial_id, desp->ssd_buf_flag);
    case PAUL:
        return Hit_PAUL(desp->serial_id, desp->ssd_buf_flag);
//    case OLDPORE:
//        return Hit_oldpore(desp->serial_id, desp->ssd_buf_flag);    
    case MOST:
        return Hit_most(desp->serial_id, desp->ssd_buf_flag);
    case MOST_RW:
        return Hit_most_rw(desp->serial_id, desp->ssd_buf_flag);

    }
    return -1;
}

static int
Strategy_Desp_LogIn(SSDBufDesp* desp)
{
    STT->cacheUsage++;
    switch(EvictStrategy)
    {
//        case LRU_global:        return insertLRUBuffer(serial_id);
    case LRU_rw:
        return insertBuffer_LRU_rw(desp->serial_id, desp->ssd_buf_flag);

    case LRU_private:
        return insertBuffer_LRU_private(desp->serial_id);
//        case LRU_batch:         return insertBuffer_LRU_batch(serial_id);
//        case Most:              return LogInMostBuffer(desp->serial_id,desp->ssd_buf_tag);
//    case PORE:
//        return LogInPoreBuffer(desp->serial_id, desp->ssd_buf_tag, desp->ssd_buf_flag);
//    case PORE_PLUS:
//        return LogInPoreBuffer_plus(desp->serial_id, desp->ssd_buf_tag, desp->ssd_buf_flag);
//    case PORE_PLUS_V2:
//        return LogIn_poreplus_v2(desp->serial_id, desp->ssd_buf_tag, desp->ssd_buf_flag);
    case PAUL:
        return LogIn_PAUL(desp->serial_id, desp->ssd_buf_tag, desp->ssd_buf_flag);
//   case OLDPORE:
//        return LogIn_oldpore(desp->serial_id, desp->ssd_buf_tag, desp->ssd_buf_flag);
    case MOST:
        return LogIn_most(desp->serial_id, desp->ssd_buf_tag, desp->ssd_buf_flag);
    case MOST_RW:
        return LogIn_most_rw(desp->serial_id, desp->ssd_buf_tag, desp->ssd_buf_flag);
    }
    return -1;
}
/*
 * read--return the buf_id of buffer according to buf_tag
 */

void
read_block(off_t offset, char *ssd_buffer)
{

#ifdef NO_CACHE
#ifdef SIMULATION
    dev_simu_read(ssd_buffer, SSD_BUFFER_SIZE, offset);
#else
    dev_pread(hdd_fd, ssd_buffer, BLKSZ, offset);
#endif // SIMULATION
    msec_r_hdd = TimerInterval_MICRO(&tv_start,&tv_stop);
    STT->time_read_hdd += Mirco2Sec(msec_r_hdd);
    STT->load_hdd_blocks++;
    return;
#else
    _TimerLap(&tv_cmstart);
    bool found = 0;
    int isCallBack;
    static SSDBufTag ssd_buf_tag;
    static SSDBufDesp* ssd_buf_hdr;

    ssd_buf_tag.offset = offset;
    if (DEBUG)
        printf("[INFO] read():-------offset=%lu\n", offset);

    ssd_buf_hdr = allocSSDBuf(ssd_buf_tag, &found, 0, &isCallBack);

    IsHit = found;
    if (found)
    {
        dev_pread(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
        msec_r_ssd = TimerInterval_MICRO(&tv_start,&tv_stop);

        STT->hitnum_r++;
        STT->time_read_ssd += Mirco2Sec(msec_r_ssd);
        STT->load_ssd_blocks++;
    }
    else
    {
#ifdef SIMULATION
        dev_simu_read(ssd_buffer, SSD_BUFFER_SIZE, offset);
#else
        dev_pread(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, offset);
#endif // SIMULATION
        msec_r_hdd = TimerInterval_MICRO(&tv_start,&tv_stop);
        STT->time_read_hdd += Mirco2Sec(msec_r_hdd);
        STT->load_hdd_blocks++;
        /* ----- Cost Model Reg------------- */
        if(isCallBack)
        {
            CM_T_rand_Reg(msec_r_hdd);
        }
        _TimerLap(&tv_cmstop);
        microsecond_t miss_usetime = TimerInterval_MICRO(&tv_cmstart, &tv_cmstop);
        CM_T_hitmiss_Reg(miss_usetime);
        /* ------------------ */

        dev_pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
        msec_w_ssd = TimerInterval_MICRO(&tv_start,&tv_stop);
        STT->time_write_ssd += Mirco2Sec(msec_w_ssd);
        STT->flush_ssd_blocks++;

    }

    _UNLOCK(&ssd_buf_hdr->lock);
#endif // NO_CACHE
}

/*
 * write--return the buf_id of buffer according to buf_tag
 */
void
write_block(off_t offset, char *ssd_buffer)
{
#ifdef NO_CACHE
#ifdef SIMULATION
    dev_simu_write(ssd_buffer, BLKSZ, offset);
#else
    dev_pwrite(hdd_fd, ssd_buffer, BLKSZ, offset);
#endif // SIMULATION
    //IO by no cache.

    msec_w_hdd = TimerInterval_MICRO(&tv_start,&tv_stop);
    STT->time_write_hdd += Mirco2Sec(msec_w_hdd);
    STT->flush_hdd_blocks++;
    return;
#else

    _TimerLap(&tv_cmstart);
    bool	found;
    int isCallBack;
    static SSDBufTag ssd_buf_tag;
    static SSDBufDesp   *ssd_buf_hdr;

    ssd_buf_tag.offset = offset;
    ssd_buf_hdr = allocSSDBuf(ssd_buf_tag, &found, 1, &isCallBack);

    //if(!found && isCallBack)
    if(!found)
    {
        /* ----- Cost Model Reg------------- */
        _TimerLap(&tv_cmstop);
        microsecond_t miss_usetime = TimerInterval_MICRO(&tv_cmstart, &tv_cmstop);
        CM_T_hitmiss_Reg(miss_usetime);
        /* ------------------ */
    }


    IsHit = found;
    STT->hitnum_w += IsHit;

    dev_pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
    msec_w_ssd = TimerInterval_MICRO(&tv_start,&tv_stop);
    STT->time_write_ssd += Mirco2Sec(msec_w_ssd);
    STT->flush_ssd_blocks++ ;

    _UNLOCK(&ssd_buf_hdr->lock);
#endif // NO_CAHCE


}

/******************
**** Utilities*****
*******************/

static int dev_pread(int fd, void* buf,size_t nbytes,off_t offset)
{
    if(I_AM_HRC_PROC)
        return nbytes;
#ifdef NO_REAL_DISK_IO
    return nbytes;
#else
    int r;
    _TimerLap(&tv_start);
    r = pread(fd,buf,nbytes,offset);
    _TimerLap(&tv_stop);
    if (r < 0)
    {
        printf("[ERROR] read():-------read from device: fd=%d, errorcode=%d, offset=%lu\n", fd, r, offset);
        exit(-1);
    }
    return r;
#endif
}

static int dev_pwrite(int fd, void* buf,size_t nbytes,off_t offset)
{
    if(I_AM_HRC_PROC)
        return nbytes;
#ifdef NO_REAL_DISK_IO
    return nbytes;
#else
    int w;
    _TimerLap(&tv_start);
    w = pwrite(fd,buf,nbytes,offset);
    _TimerLap(&tv_stop);
    if (w < 0)
    {
        printf("[ERROR] read():-------write to device: fd=%d, errorcode=%d, offset=%lu\n", fd, w, offset);
        exit(-1);
    }
    return w;
#endif
}

static int dev_simu_write(void* buf,size_t nbytes,off_t offset)
{
    if(I_AM_HRC_PROC)
        return nbytes;

    int w;
    _TimerLap(&tv_start);
    w = simu_smr_write(buf,nbytes,offset);
    _TimerLap(&tv_stop);
    return w;
}

static int dev_simu_read(void* buf,size_t nbytes,off_t offset)
{
    if(I_AM_HRC_PROC)
        return nbytes;

    int r;
    _TimerLap(&tv_start);
    r = simu_smr_read(buf,nbytes,offset);
    _TimerLap(&tv_stop);
    return r;
}

static SSDBufDesp*
pop_freebuf()
{
    if(ssd_buf_desp_ctrl->first_freessd < 0)
        return NULL;
    SSDBufDesp* ssd_buf_hdr = &ssd_buf_desps[ssd_buf_desp_ctrl->first_freessd];
    ssd_buf_desp_ctrl->first_freessd = ssd_buf_hdr->next_freessd;
    ssd_buf_hdr->next_freessd = -1;
    ssd_buf_desp_ctrl->n_usedssd++;
    return ssd_buf_hdr;
}
static int
push_freebuf(SSDBufDesp* freeDesp)
{
    freeDesp->next_freessd = ssd_buf_desp_ctrl->first_freessd;
    ssd_buf_desp_ctrl->first_freessd = freeDesp->serial_id;
    return ssd_buf_desp_ctrl->first_freessd;
}

void
_LOCK(pthread_mutex_t* lock)
{
#ifdef MULTIUSER
    SHM_mutex_lock(lock);
#endif // MULTIUSER
}

void
_UNLOCK(pthread_mutex_t* lock)
{
#ifdef MULTIUSER
    SHM_mutex_unlock(lock);
#endif // MULTIUSER
}
