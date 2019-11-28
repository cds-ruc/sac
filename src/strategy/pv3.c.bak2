#include <stdlib.h>
#include "pv3.h"
#include "statusDef.h"
#include "report.h"
#include "costmodel.h"
#include <math.h>
//#define random(x) (rand()%x)
#define IsDirty(flag) ( (flag & SSD_BUF_DIRTY) != 0 )
#define IsClean(flag) ( (flag & SSD_BUF_DIRTY) == 0 )

#define EVICT_DITRY_GRAIN 64 // The grain of once dirty blocks eviction

typedef struct
{
    long            pagecnt_clean;
    long            head,tail;
    pthread_mutex_t lock;
} CleanDespCtrl;

typedef enum
{
    CLEAN_ONLY,
    DIRTY_ONLY,
    HYBRID
} EnumEvictModel;
static blkcnt_t Evicted_Blk_Cnt = 0;
static blkcnt_t  ZONEBLKSZ;

static StrategyDesp_pore*   GlobalDespArray;
static ZoneCtrl*            ZoneCtrlArray;
static CleanDespCtrl        CleanCtrl;

static unsigned long*       ZoneSortArray;      /* The zone ID array sorted by weight(calculated customized). it is used to determine the open zones */
static int                  NonEmptyZoneCnt = 0;
static unsigned long*       OpenZoneSet;        /* The decided open zones in current period, which chosed by both the weight-sorted array and the access threshold. */
static int                  OpenZoneCnt;        /* It represent the number of open zones and the first number elements in 'ZoneSortArray' is the open zones ID */

extern long                 PeriodLenth;        /* Which defines the upper limit of the block amount of selected OpenZone and of Evicted blocks. */
static long                 Progress_Clean, Progress_Dirty;     /* Current times to evict clean/dirty block in a period lenth */
static long                 StampGlobal;      /* Current io sequenced number in a period lenth, used to distinct the degree of heat among zones */
static long                 PeriodStamp;

static void add2ArrayHead(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl);
static void move2ArrayHead(StrategyDesp_pore* desp,ZoneCtrl* zoneCtrl);
//#define stamp(desp, zoneCtrl) \
//    StampGlobal++;\
//    desp->stamp = zoneCtrl->stamp = StampGlobal;
static void stamp(StrategyDesp_pore * desp);

static void unloadfromZone(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl);
static void clearDesp(StrategyDesp_pore* desp);
static void hit(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl);
static void add2CleanArrayHead(StrategyDesp_pore* desp);
static void unloadfromCleanArray(StrategyDesp_pore* desp);
static void move2CleanArrayHead(StrategyDesp_pore* desp);

/** PORE Plus**/
static int redefineOpenZones();

static EnumEvictModel       CurEvictModel;
static int choose_colder(long stampA, long stampB, long minStamp);
static int get_FrozenOpZone_Seq();

static volatile unsigned long
getZoneNum(size_t offset)
{
    return offset / ZONESZ;
}

/* Process Function */
int
Init_poreplus_v3()
{
    ZONEBLKSZ = ZONESZ / BLCKSZ;

    PeriodStamp = StampGlobal = Progress_Clean = Progress_Dirty = 0;
    GlobalDespArray = (StrategyDesp_pore*)malloc(sizeof(StrategyDesp_pore) * NBLOCK_SSD_CACHE);
    ZoneCtrlArray = (ZoneCtrl*)malloc(sizeof(ZoneCtrl) * NZONES);

    NonEmptyZoneCnt = OpenZoneCnt = 0;
    ZoneSortArray = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);
    OpenZoneSet = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);
    int i = 0;
    while(i < NBLOCK_SSD_CACHE)
    {
        StrategyDesp_pore* desp = GlobalDespArray + i;
        desp->serial_id = i;
        desp->ssd_buf_tag.offset = -1;
        desp->next = desp->pre = -1;
        desp->heat = 0;
        desp->stamp = 0;
        desp->flag = 0;
        desp->zoneId = -1;
        i++;
    }
    i = 0;
    while(i < NZONES)
    {
        ZoneCtrl* ctrl = ZoneCtrlArray + i;
        ctrl->zoneId = i;
        ctrl->heat = ctrl->pagecnt_clean = ctrl->pagecnt_dirty = 0;
        ctrl->head = ctrl->tail = -1;
        ctrl->score = 0;
        ZoneSortArray[i] = 0;
        i++;
    }
    CleanCtrl.pagecnt_clean = 0;
    CleanCtrl.head = CleanCtrl.tail = -1;
    return 0;
}

int
LogIn_poreplus_v3(long despId, SSDBufTag tag, unsigned flag)
{
    /* activate the decriptor */
    StrategyDesp_pore* myDesp = GlobalDespArray + despId;
    unsigned long myZoneId = getZoneNum(tag.offset);
    ZoneCtrl* myZone = ZoneCtrlArray + myZoneId;
    myDesp->zoneId = myZoneId;
    myDesp->ssd_buf_tag = tag;
    myDesp->flag |= flag;

    /* add into chain */
    stamp(myDesp);

    if(IsDirty(flag))
    {
        /* add into Zone LRU as it's dirty tag */
        add2ArrayHead(myDesp, myZone);
        myZone->pagecnt_dirty++;
        //myZone->score ++ ;
    }
    else
    {
        /* add into Global Clean LRU as it's clean tag */
        add2CleanArrayHead(myDesp);
        CleanCtrl.pagecnt_clean++;
    }

    return 1;
}

int
Hit_poreplus_v3(long despId, unsigned flag)
{
    StrategyDesp_pore* myDesp = GlobalDespArray + despId;
    ZoneCtrl* myZone = ZoneCtrlArray + getZoneNum(myDesp->ssd_buf_tag.offset);

    if (IsClean(myDesp->flag) && IsDirty(flag))
    {
        /* clean --> dirty */
        unloadfromCleanArray(myDesp);
        add2ArrayHead(myDesp,myZone);
        myZone->pagecnt_dirty++;
        CleanCtrl.pagecnt_clean--;
        hit(myDesp,myZone);
    }
    else if (IsClean(myDesp->flag) && IsClean(flag))
    {
        /* clean --> clean */
        move2CleanArrayHead(myDesp);
    }
    else
    {
        /* dirty hit again*/
        move2ArrayHead(myDesp,myZone);
        hit(myDesp,myZone);
    }
    stamp(myDesp);
    myDesp->flag |= flag;
}

/** \brief
 *  The default cache-out strategy implemented in PORE+ framework.
    We use the dirty block and clean block isolation management in the way to achieve amplification control.
    There is 3 following phases of the implement:
    1.  Searching phase:
        search and redefine the Open Zones: those dirty blocks content > threshold.

    2.  Deciding phase:
        decieding evicted model according the open zones and the ratio of clean blocks. Hybrid / Clean-Only

    3.  Evicting phase:
        If in the Clean-Only model, evicting clean blocks by global LRU.
        If else the Hybrid model, evicting both of global clean blocks and dirty block in open zones.
 */
int
LogOut_poreplus_v3(long * out_despid_array, int max_n_batch)
{
    static int CurEvictZoneSeq = -1;
    static long evict_clean_cnt = 0, evict_dirty_cnt = 0;
    //static int isWholeZone = 0;
    //static int curZone_evict_cnt = 0;

    /* The condition to restart a new OpenZone period is
        1. flushed enough dirty block by zone
        2. or flushed enough clean block when the OpenZone flushing has still not start.
    */
    if((Progress_Clean >= PeriodLenth && Progress_Dirty == 0) || Progress_Dirty >= PeriodLenth || Progress_Clean + Progress_Dirty == 0)
    {
FLAG_NEWPERIOD:
        printf("This Period Evict Info: clean:%ld, dirty:%ld\n",evict_clean_cnt,evict_dirty_cnt);

        /** 1. Searching Phase **/
        OpenZoneCnt = 0;
        Progress_Clean = Progress_Dirty = 0;
        evict_clean_cnt = evict_dirty_cnt = 0;

        CurEvictZoneSeq = -1;
        PeriodStamp++;
        //isWholeZone = 0;
        redefineOpenZones();

        /** 2. Decide Evict Model Phase **/

        if(OpenZoneCnt > 0)
        {
            CurEvictModel = HYBRID;
        }
        else
            CurEvictModel = CLEAN_ONLY;


        printf("-------------New Period!-----------\n");
        printf("Period [%d], Non-Empty Zone_Cnt=%d, OpenZones_cnt=%d, CleanBlks=%ld(%0.2lf) ",PeriodStamp, NonEmptyZoneCnt, OpenZoneCnt,CleanCtrl.pagecnt_clean, (double)CleanCtrl.pagecnt_clean/NBLOCK_SSD_CACHE);
        switch(CurEvictModel)
        {
        case CLEAN_ONLY:
            printf("Evict Model=%s\n","CLEAN-ONLY");
            break;
        case DIRTY_ONLY:
            printf("Evict Model=%s\n","DIRTY_ONLY");
            break;
        case HYBRID:
            printf("Evict Model=%s\n","HYBRID");
            break;

        }
    }

    /**3. Evict Phase **/
    ZoneCtrl* evictZone;
    if(CurEvictModel == CLEAN_ONLY)
    {
        if(CleanCtrl.pagecnt_clean <= 0)
            goto FLAG_NEWPERIOD;
        //return -1;
        goto FLAG_EVICT_CLEAN;
    }
    else if(CurEvictModel == HYBRID)
    {
        //else
        StrategyDesp_pore * cleanDesp, * dirtyDesp;

        CurEvictZoneSeq = get_FrozenOpZone_Seq();
        if(CurEvictZoneSeq < 0)
            goto FLAG_NEWPERIOD;
        // Compare time stamp;
        if(CleanCtrl.pagecnt_clean <= 0)
            goto FLAG_EVICT_DIRTYZONE;

        cleanDesp = GlobalDespArray + CleanCtrl.tail;
        evictZone = ZoneCtrlArray + OpenZoneSet[CurEvictZoneSeq];
        dirtyDesp = GlobalDespArray + evictZone->tail;

        /* T-Switcher will start working after a while of been evicted NBLOCK_SSD_CACHE blocks. */
        if(Evicted_Blk_Cnt < TS_WindowSize)
        {
            if(choose_colder(cleanDesp->stamp, dirtyDesp->stamp, StampGlobal))
                goto FLAG_EVICT_CLEAN;
            else
                goto FLAG_EVICT_DIRTYZONE;
        }
        else
        {
            cm_token token;
            token.will_evict_clean_blkcnt = EVICT_DITRY_GRAIN;
            token.will_evict_dirty_blkcnt = EVICT_DITRY_GRAIN;
            token.wrtamp = ((double)(ZONEBLKSZ) * 2) / evictZone->pagecnt_dirty ;
            int type = CM_CHOOSE(token);
            if(type == 0)
                goto FLAG_EVICT_CLEAN;
            else
                goto FLAG_EVICT_DIRTYZONE;
        }
    }
    else
    {
        return -2;
    }


FLAG_EVICT_CLEAN:
    1;
    int i = 0;
    while(i < EVICT_DITRY_GRAIN && CleanCtrl.pagecnt_clean > 0)
    {
        StrategyDesp_pore * cleanDesp = GlobalDespArray + CleanCtrl.tail;
        out_despid_array[i] = cleanDesp->serial_id;
        unloadfromCleanArray(cleanDesp);
        clearDesp(cleanDesp);

        Progress_Clean ++;
        evict_clean_cnt ++;
        CleanCtrl.pagecnt_clean --;
        i ++;
    }
    Evicted_Blk_Cnt += i;
    return i;

FLAG_EVICT_DIRTYZONE:
    evictZone = ZoneCtrlArray + OpenZoneSet[CurEvictZoneSeq];

    int j = 0; // batch cache out
    while(j < EVICT_DITRY_GRAIN && evictZone->pagecnt_dirty > 0)
    {
        StrategyDesp_pore* frozenDesp = GlobalDespArray + evictZone->tail;

        unloadfromZone(frozenDesp,evictZone);
        out_despid_array[j] = frozenDesp->serial_id;
//        evictZone->score -= (double) 1 / (1 << frozenDesp->heat);

        Progress_Dirty ++;
        evictZone->pagecnt_dirty--;
        evictZone->heat -= frozenDesp->heat;
        evict_dirty_cnt++;

        clearDesp(frozenDesp);
        j++;
    }
    //printf("pore+V2: batch flush dirty cnt [%d] from zone[%lu]\n", j,evictZone->zoneId);

    Evicted_Blk_Cnt += j;
//    printf("SCORE REPORT: zone id[%d], score[%lu]\n", evictZone->zoneId, evictZone->score);
    return j;
}

/****************
** Utilities ****
*****************/
/* Utilities for Dirty descriptors Array in each Zone*/

static void
hit(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl)
{
    desp->heat ++;
    zoneCtrl->heat++;
//    zoneCtrl->score -= (double) 1 / (1 << desp->heat);
}

static void
add2ArrayHead(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl)
{
    if(zoneCtrl->head < 0)
    {
        //empty
        zoneCtrl->head = zoneCtrl->tail = desp->serial_id;
    }
    else
    {
        //unempty
        StrategyDesp_pore* headDesp = GlobalDespArray + zoneCtrl->head;
        desp->pre = -1;
        desp->next = zoneCtrl->head;
        headDesp->pre = desp->serial_id;
        zoneCtrl->head = desp->serial_id;
    }
}

static void
unloadfromZone(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl)
{
    if(desp->pre < 0)
    {
        zoneCtrl->head = desp->next;
    }
    else
    {
        GlobalDespArray[desp->pre].next = desp->next;
    }

    if(desp->next < 0)
    {
        zoneCtrl->tail = desp->pre;
    }
    else
    {
        GlobalDespArray[desp->next].pre = desp->pre;
    }
    desp->pre = desp->next = -1;
}

static void
move2ArrayHead(StrategyDesp_pore* desp,ZoneCtrl* zoneCtrl)
{
    unloadfromZone(desp, zoneCtrl);
    add2ArrayHead(desp, zoneCtrl);
}

static void
clearDesp(StrategyDesp_pore* desp)
{
    desp->ssd_buf_tag.offset = -1;
    desp->next = desp->pre = -1;
    desp->heat = 0;
    desp->stamp = 0;
    desp->flag &= ~(SSD_BUF_DIRTY | SSD_BUF_VALID);
    desp->zoneId = -1;
}

/* Utilities for Global Clean Descriptors Array */
static void
add2CleanArrayHead(StrategyDesp_pore* desp)
{
    if(CleanCtrl.head < 0)
    {
        //empty
        CleanCtrl.head = CleanCtrl.tail = desp->serial_id;
    }
    else
    {
        //unempty
        StrategyDesp_pore* headDesp = GlobalDespArray + CleanCtrl.head;
        desp->pre = -1;
        desp->next = CleanCtrl.head;
        headDesp->pre = desp->serial_id;
        CleanCtrl.head = desp->serial_id;
    }
}

static void
unloadfromCleanArray(StrategyDesp_pore* desp)
{
    if(desp->pre < 0)
    {
        CleanCtrl.head = desp->next;
    }
    else
    {
        GlobalDespArray[desp->pre].next = desp->next;
    }

    if(desp->next < 0)
    {
        CleanCtrl.tail = desp->pre;
    }
    else
    {
        GlobalDespArray[desp->next].pre = desp->pre;
    }
    desp->pre = desp->next = -1;
}

static void
move2CleanArrayHead(StrategyDesp_pore* desp)
{
    unloadfromCleanArray(desp);
    add2CleanArrayHead(desp);
}

/* Decision Method */
/** \brief
 *  Quick-Sort method to sort the zones by score.
    NOTICE!
        If the gap between variable 'start' and 'end', it will PROBABLY cause call stack OVERFLOW!
        So this function need to modify for better.
 */
static void
qsort_zone(long start, long end)
{
    long		i = start;
    long		j = end;

    long S = ZoneSortArray[start];
    ZoneCtrl* curCtrl = ZoneCtrlArray + S;
    unsigned long sScore = curCtrl->score;
    while (i < j)
    {
        while (!(ZoneCtrlArray[ZoneSortArray[j]].score > sScore) && i<j)
        {
            j--;
        }
        ZoneSortArray[i] = ZoneSortArray[j];

        while (!(ZoneCtrlArray[ZoneSortArray[i]].score < sScore) && i<j)
        {
            i++;
        }
        ZoneSortArray[j] = ZoneSortArray[i];
    }

    ZoneSortArray[i] = S;
    if (i - 1 > start)
        qsort_zone(start, i - 1);
    if (j + 1 < end)
        qsort_zone(j + 1, end);
}

static long
extractNonEmptyZoneId()
{
    int zoneId = 0, cnt = 0;
    while(zoneId < NZONES)
    {
        ZoneCtrl* zone = ZoneCtrlArray + zoneId;
        if(zone->pagecnt_dirty > 0)
        {
            ZoneSortArray[cnt] = zoneId;
            cnt++;
        }
        zoneId++;
    }
    return cnt;
}

static volatile void
pause_and_score()
{
    /*  For simplicity, searching all the zones of SMR,
        actually it's only needed to search the zones which had been cached.
        But it doesn't matter because of only 200~500K meta data of zones in memory for searching, it's not a big number.
    */
//    int n = 0;
//    while( n < NZONES )
//    {
//        ZoneCtrl* ctrl = ZoneCtrlArray + n;
//
//        ctrl->score = ((ctrl->pagecnt_dirty) * (ctrl->pagecnt_dirty)) * 1000000 / (ctrl->heat+1);
//        n++;
//    }
    /* Score all zones. */
    blkcnt_t n = 0;
    ZoneCtrl * myCtrl;
    while(n < NonEmptyZoneCnt)
    {
        myCtrl = ZoneCtrlArray + ZoneSortArray[n];
        myCtrl->score = 0;

        /* score each block of the non-empty zone */
        StrategyDesp_pore * desp;
        blkcnt_t despId = myCtrl->head;
        while(despId >= 0)
        {
            desp = GlobalDespArray + despId;
            long idx2 = PeriodStamp - desp->stamp;
            if(idx2 > 15)
                idx2 = 15;

            myCtrl->score += (0x00000001 << idx2);

            despId = desp->next;
        }
        n++ ;
    }
}


static int
redefineOpenZones()
{
    NonEmptyZoneCnt = extractNonEmptyZoneId();
    pause_and_score(); /**< Method 1 */
    qsort_zone(0,NonEmptyZoneCnt-1);

    long n_chooseblk = 0, n = 0;
    long max_n_zones = PeriodLenth / (ZONESZ / BLCKSZ);
    if(max_n_zones == 0)
        max_n_zones = 1;  // This is for Emulation on small traces, some of their fifo size are lower than a zone size.
    OpenZoneCnt = 0;
    while(n < NonEmptyZoneCnt && n < max_n_zones)
    {
        ZoneCtrl* zone = ZoneCtrlArray + ZoneSortArray[n];
        OpenZoneSet[OpenZoneCnt] = zone->zoneId;
        OpenZoneCnt++;
        n++;
    }

    /** lookup sort result **/
//    int i;
//    for(i = 0; i<NonEmptyZoneCnt; i++)
//    {
//        printf("%d: score=%f\t\theat=%ld\t\tndirty=%ld\t\tnclean=%ld\n",
//               i,
//               ZoneCtrlArray[ZoneSortArray[i]].score,
//               ZoneCtrlArray[ZoneSortArray[i]].heat,
//               ZoneCtrlArray[ZoneSortArray[i]].pagecnt_dirty,
//               ZoneCtrlArray[ZoneSortArray[i]].pagecnt_clean);
//    }

    return 0;
}

static int
choose_colder(long stampA, long stampB, long maxStamp)
{
//    srand((unsigned int)time(0));
//   long ran = random(1000);

    // double weightA = (double)(maxStamp - stampA + 1) / (2*maxStamp - stampA - stampB + 2);

    // if(ran < 1000 * weightA)
    //     return 1;
    // else
    //     return 0;

    return (stampA > stampB) ? 0 : 1;
}

static int
get_FrozenOpZone_Seq()
{
    int seq = 0;
    blkcnt_t frozenSeq = -1;
    long frozenStamp = PeriodStamp;
    while(seq < OpenZoneCnt)
    {
        ZoneCtrl* ctrl = ZoneCtrlArray + OpenZoneSet[seq];
        if(ctrl->pagecnt_dirty <= 0)
        {
            seq ++;
            continue;
        }

        StrategyDesp_pore* tail = GlobalDespArray + ctrl->tail;
        if(tail->stamp < frozenStamp)
        {
            frozenStamp = tail->stamp;
            frozenSeq = seq;
        }
        seq ++;
    }

    return frozenSeq;
}

static void stamp(StrategyDesp_pore * desp)
{
    desp->stamp = PeriodStamp;
}
