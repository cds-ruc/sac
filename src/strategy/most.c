#include <stdlib.h>
#include "most.h"
#include "../statusDef.h"
#include "../report.h"
typedef struct Dscptr
{
    long serial_id;
    SSDBufTag ssd_buf_tag;
    unsigned flag;
    long pre, next;
    unsigned long heat;
    long stamp;
    unsigned long zoneId;
} Dscptr;

typedef struct ZoneCtrl
{
    unsigned long zoneId;
    long heat;
    long pagecnt_dirty;
    long pagecnt_clean;
    long head, tail;
    int activate_after_n_cycles;
    unsigned long score;
} ZoneCtrl;

static Dscptr *GlobalDespArray;
static ZoneCtrl *ZoneCtrlArray;

static unsigned long *ZoneSortArray; /* The zone ID array sorted by weight(calculated customized). it is used to determine the open zones */
static int OpenZoneCnt;              /* It represent the number of open zones and the first number elements in 'ZoneSortArray' is the open zones ID */

extern long Cycle_Length;   /* The period lenth which defines the times of eviction triggered */
static long PeriodProgress; /* Current times of eviction in a period lenth */
static long StampGlobal;    /* Current io sequenced number in a period lenth, used to distinct the degree of heat among zones */
static int IsNewPeriod;

static void add2ArrayHead(Dscptr *desp, ZoneCtrl *zoneCtrl);
static void move2ArrayHead(Dscptr *desp, ZoneCtrl *zoneCtrl);
static long stamp(Dscptr *desp);
static void unloadfromZone(Dscptr *desp, ZoneCtrl *zoneCtrl);
static void clearDesp(Dscptr *desp);
static void hit(Dscptr *desp, ZoneCtrl *zoneCtrl);
/** PORE **/
static int redefineOpenZones();
static ZoneCtrl *getEvictZone();
static long stamp(Dscptr *desp);

static unsigned long
getZoneNum(size_t offset)
{
    return offset / ZONESZ;
}

/* Process Function */
int Init_most()
{
    Cycle_Length = NBLOCK_SMR_PB;
    StampGlobal = PeriodProgress = 0;
    IsNewPeriod = 0;
    GlobalDespArray = (Dscptr *)malloc(sizeof(Dscptr) * NBLOCK_SSD_CACHE);
    ZoneCtrlArray = (ZoneCtrl *)malloc(sizeof(ZoneCtrl) * NZONES);

    ZoneSortArray = (unsigned long *)malloc(sizeof(unsigned long) * NZONES);

    int i = 0;
    while (i < NBLOCK_SSD_CACHE)
    {
        Dscptr *desp = GlobalDespArray + i;
        desp->serial_id = i;
        desp->ssd_buf_tag.offset = -1;
        desp->next = desp->pre = -1;
        desp->heat = 0;
        desp->stamp = 0;
        desp->flag = 0;
        i++;
    }
    i = 0;
    while (i < NZONES)
    {
        ZoneCtrl *ctrl = ZoneCtrlArray + i;
        ctrl->zoneId = i;
        ctrl->heat = ctrl->pagecnt_clean = ctrl->pagecnt_dirty = 0;
        ctrl->head = ctrl->tail = -1;
        ctrl->score = -1;
        ZoneSortArray[i] = 0;
        i++;
    }
    return 0;
}

int LogIn_most(long despId, SSDBufTag tag, unsigned flag)
{
    /* activate the decriptor */
    Dscptr *myDesp = GlobalDespArray + despId;
    ZoneCtrl *myZone = ZoneCtrlArray + getZoneNum(tag.offset);
    myDesp->ssd_buf_tag = tag;
    myDesp->flag |= flag;

    /* add into chain */
    add2ArrayHead(myDesp, myZone);

    if ((flag & SSD_BUF_DIRTY) != 0)
        myZone->pagecnt_dirty++;
    else
    {
        myZone->pagecnt_clean++;
    }
    return 1;
}

int LogOut_most(long *out_despid_array, int max_n_batch)
{
    static int periodCnt = 0;
    static ZoneCtrl *chosenOpZone;
    if (PeriodProgress % Cycle_Length == 0 || chosenOpZone->tail < 0)
    {
        redefineOpenZones();
        PeriodProgress = 0;
        periodCnt++;
        chosenOpZone = getEvictZone();
        printf("Period [%d], OpenZones_cnt=%d\n", periodCnt, OpenZoneCnt);
    }

    Dscptr *evitedDesp;
    int evict_grain = 64;
    int cnt = 0;
    while (cnt < evict_grain)
    {
        if (chosenOpZone->tail < 0)
            break;
        evitedDesp = GlobalDespArray + chosenOpZone->tail;
        out_despid_array[cnt] = evitedDesp->serial_id;

        unloadfromZone(evitedDesp, chosenOpZone);
        chosenOpZone->heat -= evitedDesp->heat; /**< Decision indicators */
        if ((evitedDesp->flag & SSD_BUF_DIRTY) != 0)
        {
            chosenOpZone->pagecnt_dirty--; /**< Decision indicators */
        }
        else
        {
            chosenOpZone->pagecnt_clean--; /**< Decision indicators */
        }
        clearDesp(evitedDesp);
        PeriodProgress++;
        cnt++;
    }
    return cnt;
}

int Hit_most(long despId, unsigned flag)
{
    Dscptr *myDesp = GlobalDespArray + despId;
    ZoneCtrl *myZone = ZoneCtrlArray + getZoneNum(myDesp->ssd_buf_tag.offset);

    move2ArrayHead(myDesp, myZone);
    hit(myDesp, myZone);
    stamp(myDesp);
    if ((myDesp->flag & SSD_BUF_DIRTY) == 0 && (flag & SSD_BUF_DIRTY) != 0)
    {
        myZone->pagecnt_dirty++;
        myZone->pagecnt_clean--;
    }
    myDesp->flag |= flag;

    return 1;
}

/****************
** Utilities ****
*****************/

static void
hit(Dscptr *desp, ZoneCtrl *zoneCtrl)
{
    desp->heat++;
    zoneCtrl->heat++;
}

static void
add2ArrayHead(Dscptr *desp, ZoneCtrl *zoneCtrl)
{
    if (zoneCtrl->head < 0)
    {
        //empty
        zoneCtrl->head = zoneCtrl->tail = desp->serial_id;
    }
    else
    {
        //unempty
        Dscptr *headDesp = GlobalDespArray + zoneCtrl->head;
        desp->pre = -1;
        desp->next = zoneCtrl->head;
        headDesp->pre = desp->serial_id;
        zoneCtrl->head = desp->serial_id;
    }
}

static void
unloadfromZone(Dscptr *desp, ZoneCtrl *zoneCtrl)
{
    if (desp->pre < 0)
    {
        zoneCtrl->head = desp->next;
    }
    else
    {
        GlobalDespArray[desp->pre].next = desp->next;
    }

    if (desp->next < 0)
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
move2ArrayHead(Dscptr *desp, ZoneCtrl *zoneCtrl)
{
    unloadfromZone(desp, zoneCtrl);
    add2ArrayHead(desp, zoneCtrl);
}

static void
clearDesp(Dscptr *desp)
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
    long i = start;
    long j = end;

    long S = ZoneSortArray[start];
    ZoneCtrl *curCtrl = ZoneCtrlArray + S;
    unsigned long sWeight = curCtrl->score;
    while (i < j)
    {
        while (!(ZoneCtrlArray[ZoneSortArray[j]].score > sWeight) && i < j)
        {
            j--;
        }
        ZoneSortArray[i] = ZoneSortArray[j];

        while (!(ZoneCtrlArray[ZoneSortArray[i]].score < sWeight) && i < j)
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
    while (zoneId < NZONES)
    {
        ZoneCtrl *zone = ZoneCtrlArray + zoneId;
        if (zone->pagecnt_dirty + zone->pagecnt_clean > 0)
        {
            ZoneSortArray[cnt] = zoneId;
            cnt++;
        }
        zoneId++;
    }
    return cnt;
}

static void
pause_and_caculate_weight_sizedivhot()
{
    int n = 0;
    while (n < NZONES)
    {
        ZoneCtrl *ctrl = ZoneCtrlArray + n;
        ctrl->score = ctrl->pagecnt_dirty + ctrl->pagecnt_clean;
        n++;
    }
}

static int
redefineOpenZones()
{
    pause_and_caculate_weight_sizedivhot(); /**< Method 1 */
    long nonEmptyZoneCnt = extractNonEmptyZoneId();
    qsort_zone(0, nonEmptyZoneCnt - 1);

    /** lookup sort result **/
    //    int i;
    //    for(i = 0; i<100; i++)
    //    {
    //        printf("%d: weight=%ld\t\theat=%ld\t\tndirty=%ld\t\tnclean=%ld\n",
    //               i,
    //               ZoneCtrlArray[ZoneSortArray[i]].weight,
    //               ZoneCtrlArray[ZoneSortArray[i]].heat,
    //               ZoneCtrlArray[ZoneSortArray[i]].pagecnt_dirty,
    //               ZoneCtrlArray[ZoneSortArray[i]].pagecnt_clean);
    //    }

    OpenZoneCnt = 1;
    IsNewPeriod = 1;
    printf("NonEmptyZoneCnt = %ld.\n", nonEmptyZoneCnt);
    return 0;
}

static ZoneCtrl *
getEvictZone()
{
    return ZoneCtrlArray + ZoneSortArray[0];
}

static long
stamp(Dscptr *desp)
{
    desp->stamp = ++StampGlobal;
    return StampGlobal;
}
