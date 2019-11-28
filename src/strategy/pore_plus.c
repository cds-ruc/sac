#include <stdlib.h>
#include "pore.h"
#include "../statusDef.h"
//#include "losertree4pore.h"
#include "../report.h"

#define random(x) (rand() % x)



typedef struct
{
    long            pagecnt_clean;
    long            head,tail;
    pthread_mutex_t lock;
}CleanDespCtrl;

typedef enum{
    CLEAN_ONLY,
    HYBRID_OpZone,
    HYBRID_ALL
}EnumEvictModel;

static blkcnt_t  ZONEBLKSZ;

static Dscptr*   GlobalDespArray;
static ZoneCtrl*            ZoneCtrlArray;
static CleanDespCtrl        CleanCtrl;

static unsigned long*       ZoneSortArray;      /* The zone ID array sorted by weight(calculated customized). it is used to determine the open zones */
static int                  NonEmptyZoneCnt = 0;
static unsigned long*       OpenZoneSet;        /* The decided open zones in current period, which chosed by both the weight-sorted array and the access threshold. */
static int                  OpenZoneCnt;        /* It represent the number of open zones and the first number elements in 'ZoneSortArray' is the open zones ID */

extern long                 Cycle_Length;        /* The period lenth which defines the times of eviction triggered */
static long                 PeriodProgress;     /* Current times of eviction in a period lenth */
static long                 StampGlobal;      /* Current io sequenced number in a period lenth, used to distinct the degree of heat among zones */

static void add2ArrayHead(Dscptr* desp, ZoneCtrl* zoneCtrl);
static void move2ArrayHead(Dscptr* desp,ZoneCtrl* zoneCtrl);
#define stamp(desp, zoneCtrl) \
    StampGlobal++;\
    desp->stamp = StampGlobal;

static void unloadfromZone(Dscptr* desp, ZoneCtrl* zoneCtrl);
static void clearDesp(Dscptr* desp);
static void hit(Dscptr* desp, ZoneCtrl* zoneCtrl);

static void add2CleanArrayHead(Dscptr* desp);
static void unloadfromCleanArray(Dscptr* desp);
static void move2CleanArrayHead(Dscptr* desp);

/** PORE Plus**/
static int redefineOpenZones();

static long plus_Dirty_Threshold;
static long plus_Clean_LowBound, plus_Clean_UpBound;

static EnumEvictModel       CurEvictModel;
static int random_choose(long stampA, long stampB, long minStamp);


static volatile unsigned long
getZoneNum(size_t offset)
{
    return offset / ZONESZ;
}

/* Process Function */
int
InitPORE_plus()
{
    ZONEBLKSZ = ZONESZ / BLKSZ;
    plus_Dirty_Threshold =  ZONEBLKSZ * 0.8;    /* Cover Rate mush be >= 80% */
    plus_Clean_LowBound =   NBLOCK_SSD_CACHE * 0.2;     /* Clean blocks number < 20% of cache size, must to adopt Hybrid Model, even if there is NONE of zones reach the dirty threshold. */
    plus_Clean_UpBound =    NBLOCK_SSD_CACHE * 0.8;     /* Clean blocks number > 80% of cache size, must to adopt Clean-Only Model, even if there EXIST zones reach the dirty threshold. */

    StampGlobal = PeriodProgress = 0;
    GlobalDespArray = (Dscptr*)malloc(sizeof(Dscptr) * NBLOCK_SSD_CACHE);
    ZoneCtrlArray = (ZoneCtrl*)malloc(sizeof(ZoneCtrl) * NZONES);

    NonEmptyZoneCnt = OpenZoneCnt = 0;
    ZoneSortArray = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);
    OpenZoneSet = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);
    int i = 0;
    while(i < NBLOCK_SSD_CACHE)
    {
        Dscptr* desp = GlobalDespArray + i;
        desp->serial_id = i;
        desp->ssd_buf_tag.offset = -1;
        desp->next = desp->pre = -1;
        desp->heat = 0;
        desp->stamp = 0;
        desp->flag = 0;
        i++;
    }
    i = 0;
    while(i < NZONES)
    {
        ZoneCtrl* ctrl = ZoneCtrlArray + i;
        ctrl->zoneId = i;
        ctrl->heat = ctrl->pagecnt_clean = ctrl->pagecnt_dirty = 0;
        ctrl->head = ctrl->tail = -1;
        ctrl->score = -1;
        ZoneSortArray[i] = 0;
        i++;
    }
    CleanCtrl.pagecnt_clean = 0;
    CleanCtrl.head = CleanCtrl.tail = -1;
    return 0;
}

int
LogInPoreBuffer_plus(long despId, SSDBufTag tag, unsigned flag)
{
    /* activate the decriptor */
    Dscptr* myDesp = GlobalDespArray + despId;
    ZoneCtrl* myZone = ZoneCtrlArray + getZoneNum(tag.offset);
    myDesp->ssd_buf_tag = tag;
    myDesp->flag |= flag;

    /* add into chain */
    stamp(myDesp, myZone);

    if((flag & SSD_BUF_DIRTY) != 0){
        /* add into Zone LRU as it's dirty tag */
        add2ArrayHead(myDesp, myZone);
        myZone->pagecnt_dirty++;
    }
    else{
        /* add into Global Clean LRU as it's clean tag */
        add2CleanArrayHead(myDesp);
        CleanCtrl.pagecnt_clean++;
    }

    return 1;
}

void
HitPoreBuffer_plus(long despId, unsigned flag)
{
    Dscptr* myDesp = GlobalDespArray + despId;
    ZoneCtrl* myZone = ZoneCtrlArray + getZoneNum(myDesp->ssd_buf_tag.offset);

    if((myDesp->flag & SSD_BUF_DIRTY) == 0 && (flag & SSD_BUF_DIRTY) != 0){
        /* clean --> dirty */
        unloadfromCleanArray(myDesp);
        add2ArrayHead(myDesp,myZone);
        myZone->pagecnt_dirty++;
        CleanCtrl.pagecnt_clean--;
        hit(myDesp,myZone);
    }
    else if((myDesp->flag & SSD_BUF_DIRTY) == 0 && (flag & SSD_BUF_DIRTY) == 0){
        /* clean --> clean */
        move2CleanArrayHead(myDesp);
    }
    else{
        /* dirty hit again*/
        move2ArrayHead(myDesp,myZone);
        hit(myDesp,myZone);
    }
    stamp(myDesp, myZone);
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
long
LogOutDesp_pore_plus()
{
    static int periodCnt = 0;
    static int CurEvictZoneSeq = 0;
    static long evict_clean_cnt = 0, evict_dirty_cnt = 0;
    static int isWholeZone = 0;
    //static int curZone_evict_cnt = 0;

    if(PeriodProgress >= Cycle_Length  && !isWholeZone)
    {
        printf("This Period Evict Info: clean:%ld, dirty:%ld\n",evict_clean_cnt,evict_dirty_cnt);

NEWPERIOD:
        /** 1. Searching Phase **/
        OpenZoneCnt = 0;
        PeriodProgress = 1;
        evict_clean_cnt = evict_dirty_cnt = 0;

        CurEvictZoneSeq = 0;
        periodCnt++;
        isWholeZone = 0;
        redefineOpenZones();

        /** 2. Decide Evict Model Phase **/
        if(CleanCtrl.pagecnt_clean < plus_Clean_LowBound)
        {
            if(OpenZoneCnt > 0)
            {
                CurEvictModel = HYBRID_OpZone;
            }
            else
            {
                CurEvictModel = HYBRID_ALL;
                int i;
                for(i = 0; i < NonEmptyZoneCnt; i++)
                {
                    OpenZoneSet[i] = ZoneSortArray[i];
                }
                OpenZoneCnt = NonEmptyZoneCnt;
            }
        }
        else if(CleanCtrl.pagecnt_clean > plus_Clean_UpBound)
        {
            CurEvictModel = CLEAN_ONLY;
        }
        else
        {
            /* Lower Bound < clean block number < Upper Bound */
            if(OpenZoneCnt > 0)
            {
                CurEvictModel = HYBRID_OpZone;
            }
            else
                CurEvictModel = CLEAN_ONLY;
        }

        printf("-------------New Period!-----------\n");
        printf("Period [%d], Non-Empty Zone_Cnt=%d, OpenZones_cnt=%d, CleanBlks=%ld(%0.2lf) ",periodCnt, NonEmptyZoneCnt, OpenZoneCnt,CleanCtrl.pagecnt_clean, (double)CleanCtrl.pagecnt_clean/NBLOCK_SSD_CACHE);
        switch(CurEvictModel)
        {
            case CLEAN_ONLY:    printf("Evict Model=%s\n","CLEAN-ONLY"); break;
            case HYBRID_ALL:    printf("Evict Model=%s\n","HYBRID_ALL"); break;
            case HYBRID_OpZone: printf("Evict Model=%s\n","HYBRID_OPZ"); break;

        }
    }

    /**3. Evict Phase **/
    Dscptr * evitedDesp;
    Dscptr * cleanDesp, * dirtyDesp;
    ZoneCtrl* evictZone = ZoneCtrlArray + OpenZoneSet[CurEvictZoneSeq];

    if(CurEvictModel == CLEAN_ONLY)
    {
        if(CleanCtrl.pagecnt_clean <= 0)
            goto NEWPERIOD;
            //return -1;
        goto EVICT_CLEAN;
    }
    else if(CurEvictModel == HYBRID_OpZone || CurEvictModel == HYBRID_ALL)
    {
        dirtyDesp = GlobalDespArray + evictZone->tail;
        if (isWholeZone)
            goto EVICT_DIRTYZONE;


        if(CleanCtrl.pagecnt_clean <= 0)
        {
            isWholeZone = 1;
            goto EVICT_DIRTYZONE;
        }
        else
            cleanDesp = GlobalDespArray + CleanCtrl.tail;

        if(random_choose(cleanDesp->stamp, dirtyDesp->stamp, StampGlobal))
            goto EVICT_CLEAN;
        else
        {
            isWholeZone = 1;
            goto EVICT_DIRTYZONE;
        }
    }
    else
    {
        return -2;
    }

EVICT_CLEAN:
    cleanDesp = GlobalDespArray + CleanCtrl.tail;
    unloadfromCleanArray(cleanDesp);
    CleanCtrl.pagecnt_clean--;

    evict_clean_cnt++;
    evitedDesp = cleanDesp;
    goto EVICT_RETURN;

EVICT_DIRTYZONE:
    unloadfromZone(dirtyDesp,evictZone);
    evictZone->pagecnt_dirty--;
    evictZone->heat -= dirtyDesp->heat;

    evict_dirty_cnt++;
    evitedDesp = dirtyDesp;

    //if( evictZone->head < 0 || curZone_evict_cnt >= ZONEBLKSZ)
    if(evictZone->pagecnt_dirty <= 0)
    {
        /* While finish to cache out whole zone, Cache out next Open Zone or restart new period. */
        isWholeZone = 0;
        if(CurEvictZoneSeq < OpenZoneCnt -1)
        {
            CurEvictZoneSeq++;
            //curZone_evict_cnt = 0;
        }
        else
            PeriodProgress = Cycle_Length + 1; // restart
    }
    goto EVICT_RETURN;

EVICT_RETURN:
    PeriodProgress++;
    clearDesp(evitedDesp);
    return evitedDesp->serial_id;
}

/****************
** Utilities ****
*****************/
/* Utilities for Dirty descriptors Array in each Zone*/

static void
hit(Dscptr* desp, ZoneCtrl* zoneCtrl)
{
    desp->heat++;
    zoneCtrl->heat++;
}

static void
add2ArrayHead(Dscptr* desp, ZoneCtrl* zoneCtrl)
{
    if(zoneCtrl->head < 0)
    {
        //empty
        zoneCtrl->head = zoneCtrl->tail = desp->serial_id;
    }
    else
    {
        //unempty
        Dscptr* headDesp = GlobalDespArray + zoneCtrl->head;
        desp->pre = -1;
        desp->next = zoneCtrl->head;
        headDesp->pre = desp->serial_id;
        zoneCtrl->head = desp->serial_id;
    }
}

static void
unloadfromZone(Dscptr* desp, ZoneCtrl* zoneCtrl)
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
move2ArrayHead(Dscptr* desp,ZoneCtrl* zoneCtrl)
{
    unloadfromZone(desp, zoneCtrl);
    add2ArrayHead(desp, zoneCtrl);
}

static void
clearDesp(Dscptr* desp)
{
    desp->ssd_buf_tag.offset = -1;
    desp->next = desp->pre = -1;
    desp->heat = 0;
    desp->stamp = 0;
    desp->flag &= ~(SSD_BUF_DIRTY | SSD_BUF_VALID);
}

/* Utilities for Global Clean Descriptors Array */
static void
add2CleanArrayHead(Dscptr* desp)
{
    if(CleanCtrl.head < 0)
    {
        //empty
        CleanCtrl.head = CleanCtrl.tail = desp->serial_id;
    }
    else
    {
        //unempty
        Dscptr* headDesp = GlobalDespArray + CleanCtrl.head;
        desp->pre = -1;
        desp->next = CleanCtrl.head;
        headDesp->pre = desp->serial_id;
        CleanCtrl.head = desp->serial_id;
    }
}

static void
unloadfromCleanArray(Dscptr* desp)
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
move2CleanArrayHead(Dscptr* desp)
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
    long sWeight = curCtrl->score;
    while (i < j)
    {
        while (!(ZoneCtrlArray[ZoneSortArray[j]].score > sWeight) && i<j)
        {
            j--;
        }
        ZoneSortArray[i] = ZoneSortArray[j];

        while (!(ZoneCtrlArray[ZoneSortArray[i]].score < sWeight) && i<j)
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
pause_and_caculate_weight_sizedivhot()
{
    /*  For simplicity, searching all the zones of SMR,
        actually it's only needed to search the zones which had been cached.
        But it doesn't matter because of only 200~500K meta data of zones in memory for searching, it's not a big number.
    */
    int n = 0;
    while( n < NZONES )
    {
        ZoneCtrl* ctrl = ZoneCtrlArray + n;
        ctrl->score = ((ctrl->pagecnt_dirty) * (ctrl->pagecnt_dirty)) * 1000000 / (ctrl->heat+1);
        n++;
    }
}


static int
redefineOpenZones()
{
    pause_and_caculate_weight_sizedivhot(); /**< Method 1 */
    NonEmptyZoneCnt = extractNonEmptyZoneId();
    qsort_zone(0,NonEmptyZoneCnt-1);

    long n_chooseblk = 0, n = 0;
    OpenZoneCnt = 0;
    while(n < NonEmptyZoneCnt)
    {
        ZoneCtrl* curZone = ZoneCtrlArray + ZoneSortArray[n];
        if(curZone->pagecnt_dirty + n_chooseblk > Cycle_Length)
            break;

        if(curZone->pagecnt_dirty >= plus_Dirty_Threshold)
        {
            n_chooseblk += curZone->pagecnt_dirty;
            OpenZoneSet[OpenZoneCnt] = curZone->zoneId;
            OpenZoneCnt++;
        }
        n++;
    }

    /** lookup sort result **/
//    int i;
//    for(i = 0; i<OpenZoneCnt; i++)
//    {
//        printf("%d: weight=%ld\t\theat=%ld\t\tndirty=%ld\t\tnclean=%ld\n",
//               i,
//               ZoneCtrlArray[OpenZoneSet[i]].weight,
//               ZoneCtrlArray[OpenZoneSet[i]].heat,
//               ZoneCtrlArray[OpenZoneSet[i]].pagecnt_dirty,
//               ZoneCtrlArray[OpenZoneSet[i]].pagecnt_clean);
//    }

    return 0;
}

static int
random_choose(long stampA, long stampB, long maxStamp)
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
