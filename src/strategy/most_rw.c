#include <stdlib.h>
#include "most_rw.h"
#include "../statusDef.h"
#include "losertree4pore.h"
#include "../report.h"

#define IsDirty(flag) ( (flag & SSD_BUF_DIRTY) != 0 )
#define IsClean(flag) ( (flag & SSD_BUF_DIRTY) == 0 )

#define EVICT_DITRY_GRAIN 64 // The grain of once dirty blocks eviction

typedef struct
{
    long            pagecnt_clean;
    long            head,tail;
    pthread_mutex_t lock;
} CleanDespCtrl;

static Dscptr*   GlobalDespArray;
static ZoneCtrl*            ZoneCtrlArray;

static CleanDespCtrl        CleanCtrl;

static unsigned long*       ZoneSortArray;      /* The zone ID array sorted by weight(calculated customized). it is used to determine the open zones */
static int                  OpenZoneCnt;        /* It represent the number of open zones and the first number elements in 'ZoneSortArray' is the open zones ID */

extern long                 Cycle_Length;        /* The period lenth which defines the times of eviction triggered */
static long                 PeriodProgress;     /* Current times of eviction in a period lenth */
static long                 CleanProgress;
static long                 StampGlobal;      /* Current io sequenced number in a period lenth, used to distinct the degree of heat among zones */
static int                  IsNewPeriod;


static void add2ArrayHead(Dscptr* desp, ZoneCtrl* zoneCtrl);
static void move2ArrayHead(Dscptr* desp,ZoneCtrl* zoneCtrl);
static long stamp(Dscptr* desp);
static void unloadfromZone(Dscptr* desp, ZoneCtrl* zoneCtrl);
static void clearDesp(Dscptr* desp);
static void hit(Dscptr* desp, ZoneCtrl* zoneCtrl);
/** PORE **/
static int redefineOpenZones();
static ZoneCtrl* getEvictZone();
static int choose_colder(long stampA, long stampB, long maxStamp);

/* Utilities for Global Clean Descriptors Array */
static void
add2CleanArrayHead(Dscptr* desp);
static void
unloadfromCleanArray(Dscptr* desp);
static void
move2CleanArrayHead(Dscptr* desp);

static volatile unsigned long
getZoneNum(size_t offset)
{
    return offset / ZONESZ;
}

/* Process Function */
int
Init_most_rw()
{
    StampGlobal = PeriodProgress = CleanProgress = 0;
    IsNewPeriod = 0;
    GlobalDespArray = (Dscptr*)malloc(sizeof(Dscptr) * NBLOCK_SSD_CACHE);
    ZoneCtrlArray = (ZoneCtrl*)malloc(sizeof(ZoneCtrl) * NZONES);

    ZoneSortArray = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);

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
LogIn_most_rw(long despId, SSDBufTag tag, unsigned flag)
{
    /* activate the decriptor */
    Dscptr* myDesp = GlobalDespArray + despId;
    ZoneCtrl* myZone = ZoneCtrlArray + getZoneNum(tag.offset);
    myDesp->ssd_buf_tag = tag;
    myDesp->flag |= flag;

    /* add into chain */
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

}

int
LogOut_most_rw(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type)
{
    static long evict_clean_cnt = 0, evict_dirty_cnt = 0;
    static int periodCnt = 0;
    static ZoneCtrl* chosenOpZone;
    if( (PeriodProgress != 0 && PeriodProgress % Cycle_Length == 0) ||
        (PeriodProgress == 0 && CleanProgress % Cycle_Length == 0) ||
        (chosenOpZone->tail < 0))
    {
        printf("This Period Evict Info: clean:%ld, dirty:%ld\n", CleanProgress , PeriodProgress);
	PeriodProgress = CleanProgress = 0;
        periodCnt++;
        redefineOpenZones();
        chosenOpZone = getEvictZone();
        printf("Period [%d], OpenZones_cnt=%d\n",periodCnt,OpenZoneCnt);
    }


    Dscptr*  victimDesp;
    if(suggest_type == ENUM_B_Clean)
    {
        if(CleanCtrl.pagecnt_clean == 0) // Consistency judgment
            usr_warning("Order to evict clean cache block, but it is exhausted in advance.");
        goto FLAG_EVICT_CLEAN;
    }
    else if(suggest_type == ENUM_B_Dirty)
    {
        if(STT->incache_n_dirty == 0)   // Consistency judgment
            usr_warning("Order to evict dirty cache block, but it is exhausted in advance.");

        goto FLAG_EVICT_DIRTYZONE;
    }
    else    // The suggest type is 'Any'.
    {
        if(CleanCtrl.pagecnt_clean  == 0 || STT->incache_n_dirty == 0)
            usr_warning("Order to evict clean or dirty cache block, but it is exhausted in advance.");

        Dscptr * cleanDesp, * dirtyDesp;

        cleanDesp = GlobalDespArray + CleanCtrl.tail;
        dirtyDesp = GlobalDespArray + chosenOpZone->tail;

        if(choose_colder(cleanDesp->stamp, dirtyDesp->stamp, StampGlobal))
            goto FLAG_EVICT_CLEAN;
        else
            goto FLAG_EVICT_DIRTYZONE;
    }

FLAG_EVICT_CLEAN:
    1;
    int i = 0;
    while(i < EVICT_DITRY_GRAIN && CleanCtrl.pagecnt_clean > 0)
    {
        victimDesp = GlobalDespArray + CleanCtrl.tail;
        out_despid_array[i] = victimDesp->serial_id;
        unloadfromCleanArray(victimDesp);
        clearDesp(victimDesp);

        evict_clean_cnt ++;
        CleanCtrl.pagecnt_clean --;
        CleanProgress ++ ;
        i ++;
    }
    return i;

FLAG_EVICT_DIRTYZONE:
    1;
    int cnt = 0;
    while(cnt < EVICT_DITRY_GRAIN)
    {
        if(chosenOpZone->tail < 0)
            break;
        victimDesp = GlobalDespArray + chosenOpZone->tail;
        out_despid_array[cnt] = victimDesp->serial_id;

        unloadfromZone(victimDesp,chosenOpZone);
        chosenOpZone->pagecnt_dirty--;                  /**< Decision indicators */

	clearDesp(victimDesp);
	PeriodProgress++;
	evict_dirty_cnt ++;
	cnt ++ ;
    }
    return cnt;
}

int
Hit_most_rw(long despId, unsigned flag)
{
    Dscptr* myDesp = GlobalDespArray + despId;
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
    myDesp->flag |= flag;
}

/****************
** Utilities ****
*****************/

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
    int n = 0;
    while( n < NZONES )
    {
        ZoneCtrl* ctrl = ZoneCtrlArray + n;
        ctrl->score = ctrl->pagecnt_dirty + ctrl->pagecnt_clean;
        n++;
    }
}


static int
redefineOpenZones()
{
    pause_and_caculate_weight_sizedivhot(); /**< Method 1 */
    OpenZoneCnt = extractNonEmptyZoneId();
    if(OpenZoneCnt == 0)
        return 0;
    else
    {
        qsort_zone(0,OpenZoneCnt-1);

        OpenZoneCnt = 1;
        IsNewPeriod = 1;
        printf("NonEmptyZoneCnt = %ld.\n",OpenZoneCnt);
        return 1;
    }
}

static ZoneCtrl*
getEvictZone()
{
    if(OpenZoneCnt == 0)
        return NULL;
    return  ZoneCtrlArray + ZoneSortArray[0];
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

static int
choose_colder(long stampA, long stampB, long maxStamp)
{
    return (stampA > stampB) ? 0 : 1;
}

