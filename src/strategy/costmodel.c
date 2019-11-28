/*
    Writed by Sun Diansen. 18,Nov,2017.

*/
#include "costmodel.h"
#include "../report.h"
/** Statistic Objects **/
static blkcnt_t CallBack_Cnt_Clean, CallBack_Cnt_Dirty;
static blkcnt_t Evict_Cnt_Clean, Evict_Cnt_Dirty;

/* RELATED PARAMETER OF COST MODEL */
static double   CC, CD;                 /* Cost of Clean/Dirty blocks. */
static double   PCB_Clean, PCB_Dirty;   /* Possibility of CallBack the clean/dirty blocks */
static double   WrtAmp;                 /* The write amp of blocks i'm going to evict. */

#define rand_init() (srand((unsigned int)time(0)))
#define random(x) (rand() % x)

/* time of random, sequence, into fifo, average.
 * And each of these collects from different way, such as:
 * Lat_read_avg from when calling the 'CM_CallBack' func and sending a parameter of read block time.
 * T_seq is the inner-disk time of sequenced IO, which depends on the hardware parameter (Set 40 microsecond).
 * Lat_write_avg should be a hardware parameter, but it's inner metadata management can't be seen, so I mearsure it from evicted dirty block.
 * Lat_evict_avg is the average time cost, which is (dirty evicted time sum / evicted count)
 */
static int  Lat_read_avg, Lat_write_avg, Lat_evict_avg, T_seq = 20;
static microsecond_t Lat_read_sum = 0;
static microsecond_t Lat_write_sum = 0;
static microsecond_t Lat_evict_sum = 0;

static blkcnt_t Cnt_read_req = 0;
static blkcnt_t Cnt_evict_sum = 0;

static blkcnt_t Cnt_write_req = 0;
static blkcnt_t T_evict_cnt = 0;
static microsecond_t * Lat_read_avgcollect;
static microsecond_t * t_hitmisscollect;

/** The relavant objs of evicted blocks array belonged the current window. **/
blkcnt_t TS_WindowSize;
blkcnt_t TS_StartSize;
typedef struct
{
    off_t       offset;
    unsigned    flag;
    microsecond_t usetime;
    int         isCallBack;
} WDItem, WDArray;

static WDArray * WindowArray;
static blkcnt_t WDArray_Head, WDArray_Tail;

#define IsFull_WDArray() ( WDArray_Head == (WDArray_Tail + 1) % TS_WindowSize )

/** The relavant objs of the Hash Index of the array **/
typedef off_t       hashkey_t;
typedef blkcnt_t    hashvalue_t;
typedef struct WDBucket
{
    off_t       key;
    blkcnt_t    itemId;
    struct WDBucket*  next;
} WDBucket;

static WDBucket * WDBucketPool, * WDBucket_Top;
static int          push_WDBucket();
static WDBucket *   pop_WDBucket();

static WDBucket ** HashIndex;
static blkcnt_t indexGet_WDItemId(off_t key);
static int      indexInsert_WDItem(hashkey_t key, hashvalue_t value);
static int      indexRemove_WDItem(hashkey_t key);
#define HashMap_KeytoValue(key) ((key / BLKSZ) % TS_WindowSize)
static int random_pick(float weight1, float weight2, float obey);

FILE* log_r3balancer;
char log_r3balancer_path[] = "/home/fei/devel/logs/log_r3balancer";

/** MAIN FUNCTIONS **/
int CM_Init()
{
    #ifndef R3BALANCER_ON
        return 0;
    #endif // R3BALANCER_ON
    TS_WindowSize = NBLOCK_SSD_CACHE  / 16;
    TS_StartSize = TS_WindowSize;
    WindowArray = (WDArray *)malloc(TS_WindowSize * sizeof(WDItem));
    WDArray_Head = WDArray_Tail = 0;

    WDBucketPool = (WDBucket *)malloc(TS_WindowSize * sizeof(WDBucket));
    WDBucket_Top = WDBucketPool;

    blkcnt_t n = 0;
    for(n = 0; n < TS_WindowSize; n++)
    {
        WDBucketPool[n].next = WDBucketPool + n + 1;
    }
    WDBucketPool[TS_WindowSize - 1].next = NULL;

    HashIndex = (WDBucket **)calloc(TS_WindowSize, sizeof(WDItem*));
    CallBack_Cnt_Clean = CallBack_Cnt_Dirty = Evict_Cnt_Clean = Evict_Cnt_Dirty = 0;

    Lat_read_avgcollect = (microsecond_t *)malloc(TS_WindowSize * sizeof(microsecond_t));
    t_hitmisscollect  = (microsecond_t *)malloc(TS_WindowSize * sizeof(microsecond_t));

    if(WindowArray == NULL || WDBucketPool == NULL || HashIndex == NULL || Lat_read_avgcollect == NULL)
        return -1;

    if((log_r3balancer = fopen(log_r3balancer_path, "w+")) == NULL)
        usr_error("Cannot open log: log_r3balancer.");

    return 0;
}

/* To register a evicted block metadata.
 * But notice that, it can not be register a block has been exist in the window array.
 * Which means you can not evict block who has been evicted a short time ago without a call back.
 * If you do the mistake, it will cause the error of metadata managment. Because I didn't set consistency checking mechanism.
 */
int CM_Reg_EvictBlk(SSDBufTag blktag, unsigned flag, microsecond_t usetime)
{
    #ifndef R3BALANCER_ON
        return 0;
    #endif // R3BALANCER_ON

    if(IsFull_WDArray())
    {
        /* Clean all the information related to the oldest block(the head block)  */
        WDItem* oldestItem = WindowArray +  WDArray_Head;

        // clean the index.
        if(!oldestItem->isCallBack)
        {
            indexRemove_WDItem(oldestItem->offset);
        }

        // 2. clean the metadata
        T_evict_cnt --;
        if((oldestItem->flag & SSD_BUF_DIRTY) != 0)
        {
            Cnt_write_req --;
            Lat_write_sum -= oldestItem->usetime;

            Evict_Cnt_Dirty --;
            if(oldestItem->isCallBack)
                CallBack_Cnt_Dirty --;
        }
        else
        {
            Evict_Cnt_Clean --;
            if(oldestItem->isCallBack)
                CallBack_Cnt_Clean--;
        }

        // 3. retrieve WDItem
        WDArray_Head = (WDArray_Head + 1) % TS_WindowSize;
    }

    // Add a new one to the tail.
    WDItem* tail = WindowArray + WDArray_Tail;
    tail->offset = blktag.offset;
    tail->flag = flag;
    tail->usetime = usetime;
    tail->isCallBack = 0;

    indexInsert_WDItem(tail->offset, WDArray_Tail);
    WDArray_Tail = (WDArray_Tail + 1) % TS_WindowSize;

    if((flag & SSD_BUF_DIRTY) != 0)
    {
        Evict_Cnt_Dirty ++;
        Cnt_write_req ++;
        Lat_write_sum += usetime;
    }
    else
        Evict_Cnt_Clean ++;
    T_evict_cnt ++;

    return 0;
}

/* Check if it exist in the evicted array within the window.
 * If exist, then to mark 'callback',
 * and remove it's index to avoid the incorrectness when next time callback.
 */
int CM_TryCallBack(SSDBufTag blktag)
{
    #ifndef R3BALANCER_ON
        return 0;
    #endif // R3BALANCER_ON

    hashkey_t key = blktag.offset;
    blkcnt_t itemId = indexGet_WDItemId(key);
    if(itemId >= 0)
    {
        WDItem* item = WindowArray + itemId;
        item->isCallBack = 1;
        indexRemove_WDItem(key);
        if((item->flag & SSD_BUF_DIRTY) != 0)
            CallBack_Cnt_Dirty ++;
        else
            CallBack_Cnt_Clean ++;
        return 1;
    }
    return 0;
}

static int DirtyWinTimes = 0, CleanWinTimes = 0;
/* brief
 * return:
 * 0 : Clean BLock
 * 1 : Dirty Block
 */
int CM_CHOOSE()
{
    #ifndef R3BALANCER_ON
        return 0;
    #endif // R3BALANCER_ON
    if(WriteOnly) return 1;

    static blkcnt_t counter = 0;
    counter ++ ;

    if(counter % 10000 == 0)
    {
        ReportCM();
    }

    PCB_Clean = (double)CallBack_Cnt_Clean / (Evict_Cnt_Clean + 1);
    PCB_Dirty = (double)CallBack_Cnt_Dirty / (Evict_Cnt_Dirty + 1);

    Lat_read_avg = Lat_read_sum / (Cnt_read_req + 1);
    Lat_write_avg = Lat_write_sum / (Cnt_write_req + 1);
    //double Lat_evict_avg_fifo = Lat_write_sum / T_evict_cnt;

    //double pcb_all = (double)(PCB_Clean + PCB_Dirty) / (Evict_Cnt_Clean + Evict_Cnt_Dirty);
    Lat_evict_avg =  Lat_evict_sum / (Cnt_evict_sum + 1);

    CC = 0 + PCB_Clean * (Lat_read_avg + Lat_evict_avg);
    CD = Lat_write_avg + (PCB_Dirty * Lat_evict_avg);


    int pick = random_pick(CC, CD, 1);
    if(pick == 1)
    {
        CleanWinTimes ++;
        return 0;
    }
    else
    {
        DirtyWinTimes ++;
        return 1;
    }
}

int CM_T_rand_Reg(microsecond_t usetime)
{
    #ifndef R3BALANCER_ON
        return 0;
    #endif // R3BALANCER_ON
    static blkcnt_t head = 0, tail = 0;
    if(head == (tail + 1) % TS_WindowSize)
    {
        // full
        Lat_read_sum -= Lat_read_avgcollect[head];
        Cnt_read_req --;
        head = (head + 1) % TS_WindowSize;
    }

    Lat_read_sum += usetime;
    Cnt_read_req ++;
    Lat_read_avgcollect[tail] = usetime;
    tail = (tail + 1) % TS_WindowSize;

    return 0;
}

int CM_T_hitmiss_Reg(microsecond_t usetime)
{
    #ifndef R3BALANCER_ON
        return 0;
    #endif // R3BALANCER_ON
    static blkcnt_t head = 0, tail = 0;
    if(head == (tail + 1) % TS_WindowSize)
    {
        // full
        Lat_evict_sum -= t_hitmisscollect[head];
        Cnt_evict_sum --;
        head = (head + 1) % TS_WindowSize;
    }

    Lat_evict_sum += usetime;
    Cnt_evict_sum ++;
    t_hitmisscollect[tail] = usetime;
    tail = (tail + 1) % TS_WindowSize;

    return 0;
}

void ReportCM()
{
    #ifndef R3BALANCER_ON
        return 0;
    #endif // R3BALANCER_ON
    printf("---------- Cost Model Runtime Report ----------\n");
    printf("Count C / D:\t");
    printf("[%ld / %ld]\n", Evict_Cnt_Clean, Evict_Cnt_Dirty);
    printf("in cache C / D:\t");
    printf("[%ld / %ld]\n", STT->incache_n_clean, STT->incache_n_dirty);

    printf("PCB C / D:\t");
    printf("[%.2f\% / %.2f\%]\n", PCB_Clean*100, PCB_Dirty*100);

    printf("Lat_read_avg,\tLat_write_avg,\tLat_evict_avg,\tT_seq:\t");
    printf("[%d,%d,%d,%d]\n", Lat_read_avg, Lat_write_avg, Lat_evict_avg, T_seq);

    printf("Effict Cost C / D:\t[%.2f / %.2f]\n", CC, CD);

    printf("Win Times C / D:\t[%d / %d]\n",CleanWinTimes, DirtyWinTimes);

}

void CM_Report_PCB()
{
    #ifndef R3BALANCER_ON
        return 0;
    #endif // R3BALANCER_ON
    static char buf[50];
    sprintf(buf,"%d,%d\n",(int)(PCB_Clean*100), (int)(PCB_Dirty*100));
    _Log(buf, log_r3balancer);
}
/** Utilities of Hash Index **/
static int push_WDBucket(WDBucket * freeBucket)
{
    freeBucket->next = WDBucket_Top;
    WDBucket_Top = freeBucket;
    return 0;
}

static WDBucket * pop_WDBucket()
{
    WDBucket * bucket = WDBucket_Top;
    WDBucket_Top = bucket->next;
    bucket->next = NULL;
    return bucket;
}

static blkcnt_t indexGet_WDItemId(hashkey_t key)
{
    blkcnt_t hashcode = HashMap_KeytoValue(key);
    WDBucket * bucket = HashIndex[hashcode];

    while(bucket != NULL)
    {
        if(bucket->key == key)
        {
            return bucket->itemId;
        }
        bucket = bucket->next;
    }
    return -1;
}

static int indexInsert_WDItem(hashkey_t key, hashvalue_t value)
{
    WDBucket* newBucket = pop_WDBucket();
    newBucket->key = key;
    newBucket->itemId = value;
    newBucket->next = NULL;

    blkcnt_t hashcode = HashMap_KeytoValue(key);
    WDBucket* bucket = HashIndex[hashcode];

    if(bucket != NULL)
    {
        newBucket->next = bucket;
    }

    HashIndex[hashcode] = newBucket;
    return 0;
}

static int indexRemove_WDItem(hashkey_t key)
{
    blkcnt_t hashcode = HashMap_KeytoValue(key);
    WDBucket * bucket = HashIndex[hashcode];

    if(bucket == NULL)
    {
        usr_warning("[CostModel]: Trying to remove unexisted hash index!\n");
        exit(-1);
    }

    // if is the first bucket
    if(bucket->key == key)
    {
        HashIndex[hashcode] = bucket->next;
        push_WDBucket(bucket);
        return 0;
    }

    // else
    WDBucket * next_bucket = bucket->next;
    while(next_bucket != NULL)
    {
        if(next_bucket->key == key)
        {
            bucket->next = next_bucket->next;
            push_WDBucket(next_bucket);
            return 0;
        }
        bucket = next_bucket;
        next_bucket = bucket->next;
    }

    usr_warning("[CostModel]: Trying to remove unexisted hash index!\n");
    exit(-1);
}

static int random_pick(float weight1, float weight2, float obey)
{
    //return (weight1 < weight2) ? 1 : 2;
    // let weight as the standard, set as 1,
    float inc_times = (weight2 / weight1) - 1;
    inc_times *= obey;

    float de_point = 1000 * (1 / (2 + inc_times));
    int token = random(1000);
    static char buf[50];
//    sprintf(buf,">>w1,w2,pick:%.1f,%.1f\n", 1.0, 1.0 + inc_times);
//    _Log(buf, log_r3balancer);

    if(token < de_point)
        return 2;
    return 1;
}
