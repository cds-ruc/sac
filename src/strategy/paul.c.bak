#include <stdlib.h>
#include "../timerUtils.h"
#include "paul.h"
#include "../statusDef.h"
#include "../report.h"
#include <math.h>
//#define random(x) (rand()%x)
#define IsDirty(flag) ( (flag & SSD_BUF_DIRTY) != 0 )
#define IsClean(flag) ( (flag & SSD_BUF_DIRTY) == 0 )

#define EVICT_DITRY_GRAIN 64 // The grain of once dirty blocks eviction

typedef struct CleanDespCtrl
{
    blkcnt_t            pagecnt_clean;
    blkcnt_t            head,tail;
    pthread_mutex_t lock;
} CleanDespCtrl;

static blkcnt_t  ZONEBLKSZ;

static Dscptr_paul*         GlobalDespArray;
static ZoneCtrl_pual*       ZoneCtrl_pualArray;
static CleanDespCtrl        CleanCtrl;

static unsigned long*       ZoneSortArray;      /* The zone ID array sorted by weight(calculated customized). it is used to determine the open zones */
static int                  NonEmptyZoneCnt = 0;
static unsigned long*       OpenZoneSet;        /* The decided open zones in current period, which chosed by both the weight-sorted array and the access threshold. */
static int                  OpenZoneCnt;        /* It represent the number of open zones and the first number elements in 'ZoneSortArray' is the open zones ID */

static long                 CycleID;
extern long                 Cycle_Length;        /* Which defines the upper limit of the block amount of selected OpenZone and of Evicted blocks. */
static long                 Cycle_Progress;     /* Current times to evict clean/dirty block in a period lenth */
static long                 StampGlobal;      /* Current io sequenced number in a period lenth, used to distinct the degree of heat among zones */
#define stamp(desp) (desp->stamp = StampGlobal ++)


static void add2ArrayHead(Dscptr_paul* desp, ZoneCtrl_pual* ZoneCtrl_pual);
static void move2ArrayHead(Dscptr_paul* desp,ZoneCtrl_pual* ZoneCtrl_pual);

static int start_new_cycle();

static void unloadfromZone(Dscptr_paul* desp, ZoneCtrl_pual* ZoneCtrl_pual);
static void clearDesp(Dscptr_paul* desp);
static void hit(Dscptr_paul* desp, ZoneCtrl_pual* ZoneCtrl_pual);
static void add2CleanArrayHead(Dscptr_paul* desp);
static void unloadfromCleanArray(Dscptr_paul* desp);
static void move2CleanArrayHead(Dscptr_paul* desp);

/*
    Out of Date(OOD)
    Alpha: We call those blocks(drt or cln) which are already out of the recent history window as Out of Date (OOD),
    and 'think' of they are won't be reused. All the blocks stamp less than 'OOD stamp' are 'OOD blocks'.

    Here we treat the last 80% of accesses are popular and the rest of 20%s are OOD.
    The OOD will be used to calculate the representation of Recall Ratio.
*/
static long OODstamp; // = StampGlobal - (long)(NBLOCK_SSD_CACHE * 0.8)
struct blk_cm_info
{
    int num_OODblks;
    int num_totalblks;
};



/** PAUL**/
static double redefineOpenZones();
static int get_FrozenOpZone_Seq();
static int random_pick (float weight1, float weight2, float obey);
static int restart_cm_alpha();

typedef enum EvictPhrase_t
{
    EP_Clean,
    EP_Dirty,
    EP_Reset
} EvictPhrase_t;
static EvictPhrase_t WhoEvict_Now, WhoEvict_Before; // Used to mark which type (r/w) of blocks should be evict in the [alpha] costmodel. (-1,clean), (1, dirty), (0, unknown)
static int NumEvict_thistime_apprx = 100000;

/** Cost Model(alpha) **/
struct COSTMODEL_Alpha
{
    microsecond_t Lat_SMR_read;
    microsecond_t (*FX_WA) (int blkcnt);

    double (*Cost_Dirty) (struct blk_cm_info * dirty, int num);
    double (*Cost_Clean) (struct blk_cm_info clean);
};
static microsecond_t costmodel_fx_wa(int blkcnt);
static double costmodel_evaDirty_alpha(struct blk_cm_info * dirty, int num);
static double costmodel_evaClean_alpha(struct blk_cm_info clean);
static struct COSTMODEL_Alpha CM_Alpha = {
    .Lat_SMR_read = 14000, //14ms per read
    .FX_WA = costmodel_fx_wa,
    .Cost_Dirty = costmodel_evaDirty_alpha,
    .Cost_Clean = costmodel_evaClean_alpha,
};
static EvictPhrase_t run_cm_alpha();


static volatile unsigned long
getZoneNum(size_t offset)
{
    return offset / ZONESZ;
}

/* Process Function */
int
Init_PUAL()
{
    ZONEBLKSZ = ZONESZ / BLKSZ;

    CycleID = StampGlobal = Cycle_Progress = OODstamp = 0;
    GlobalDespArray = (Dscptr_paul*) malloc(sizeof(Dscptr_paul) * NBLOCK_SSD_CACHE);
    ZoneCtrl_pualArray = (ZoneCtrl_pual*)  malloc(sizeof(ZoneCtrl_pual) * NZONES);

    NonEmptyZoneCnt = OpenZoneCnt = 0;
    ZoneSortArray = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);
    OpenZoneSet = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);
    int i = 0;
    while(i < NBLOCK_SSD_CACHE)
    {
        Dscptr_paul* desp = GlobalDespArray + i;
        desp->serial_id = i;
        desp->ssd_buf_tag.offset = -1;
        desp->next = desp->pre = -1;
        desp->stamp = 0;
        desp->flag = 0;
        desp->zoneId = -1;
        i++;
    }
    i = 0;
    while(i < NZONES)
    {
        ZoneCtrl_pual* ctrl = ZoneCtrl_pualArray + i;
        ctrl->zoneId = i;
        ctrl->pagecnt_dirty = 0;
        ctrl->head = ctrl->tail = -1;
        ctrl->OOD_num = 0;
        ZoneSortArray[i] = 0;
        i++;
    }
    CleanCtrl.pagecnt_clean = 0;
    CleanCtrl.head = CleanCtrl.tail = -1;

    WhoEvict_Now = WhoEvict_Before = EP_Reset;
    return 0;
}

int
LogIn_PAUL(long despId, SSDBufTag tag, unsigned flag)
{
    /* activate the decriptor */
    Dscptr_paul* myDesp = GlobalDespArray + despId;
    unsigned long myZoneId = getZoneNum(tag.offset);
    ZoneCtrl_pual* myZone = ZoneCtrl_pualArray + myZoneId;
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
Hit_PAUL(long despId, unsigned flag)
{
    Dscptr_paul* myDesp = GlobalDespArray + despId;
    ZoneCtrl_pual* myZone = ZoneCtrl_pualArray + getZoneNum(myDesp->ssd_buf_tag.offset);

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

    return 0;
}

static int
start_new_cycle()
{
    CycleID++;
    redefineOpenZones();

    printf("-------------New Cycle!-----------\n");
    printf("Cycle ID [%ld], Non-Empty Zone_Cnt=%d, OpenZones_cnt=%d, CleanBlks=%ld(%0.2lf)\n",CycleID, NonEmptyZoneCnt, OpenZoneCnt,CleanCtrl.pagecnt_clean, (double)CleanCtrl.pagecnt_clean/NBLOCK_SSD_CACHE);

    return 0;
}

/** \brief
 */
int
LogOut_PAUL(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type)
{
    static int CurEvictZoneSeq;
    static int Num_evict_clean_cycle = 0, Num_evict_dirty_cycle = 0;
    int evict_cnt = 0;

    ZoneCtrl_pual* evictZone;

    if(suggest_type == ENUM_B_Clean)
    {
        if(CleanCtrl.pagecnt_clean == 0 || WhoEvict_Now == EP_Dirty) // Consistency judgment
            usr_error("Illegal to evict CLEAN cache.");

        if(WhoEvict_Now == EP_Reset)
        {
            WhoEvict_Now = WhoEvict_Before = EP_Clean;
        }
        goto FLAG_EVICT_CLEAN;
    }
    else if(suggest_type == ENUM_B_Dirty)
    {
        if(STT->incache_n_dirty == 0 || WhoEvict_Now == EP_Clean )   // Consistency judgment
            usr_error("Illegal to evict DIRTY cache.");


        if(WhoEvict_Now == EP_Reset){
            start_new_cycle();
            WhoEvict_Now = WhoEvict_Before = EP_Dirty;
        }
        goto FLAG_EVICT_DIRTYZONE;
    }
    else if(suggest_type == ENUM_B_Any)
    {
        /* Here we use the Cost Model as the default strategy */
        if(WhoEvict_Now == EP_Reset)
        { // unknown. So the costmodel[alpha] runs!
            WhoEvict_Now = WhoEvict_Before = run_cm_alpha();
        }

        if(WhoEvict_Now == EP_Clean)
        { // clean
            goto FLAG_EVICT_CLEAN;
        }
        else if(WhoEvict_Now == EP_Dirty)
        { //dirty
            goto FLAG_EVICT_DIRTYZONE;
        }
    }
    else
        usr_error("PAUL catched an unsupported eviction type.");

FLAG_EVICT_CLEAN:
    while(evict_cnt < EVICT_DITRY_GRAIN && CleanCtrl.pagecnt_clean > 0)
    {
        Dscptr_paul * cleanDesp = GlobalDespArray + CleanCtrl.tail;
        out_despid_array[evict_cnt] = cleanDesp->serial_id;
        unloadfromCleanArray(cleanDesp);
        clearDesp(cleanDesp);

        Num_evict_clean_cycle ++;
        CleanCtrl.pagecnt_clean --;
        evict_cnt ++;
    }

    if(CleanCtrl.pagecnt_clean == 0 || (Num_evict_clean_cycle >= NumEvict_thistime_apprx)){
        Num_evict_clean_cycle = 0;
        WhoEvict_Now = EP_Reset;
    }
    return evict_cnt;

FLAG_EVICT_DIRTYZONE:
    if((CurEvictZoneSeq = get_FrozenOpZone_Seq()) < 0)
        usr_error("FLAG_EVICT_DIRTYZONE error");
    evictZone = ZoneCtrl_pualArray + OpenZoneSet[CurEvictZoneSeq];

    while(evict_cnt < EVICT_DITRY_GRAIN && evictZone->pagecnt_dirty > 0)
    {
        Dscptr_paul* frozenDesp = GlobalDespArray + evictZone->tail;

        unloadfromZone(frozenDesp,evictZone);
        out_despid_array[evict_cnt] = frozenDesp->serial_id;

        Cycle_Progress ++;
        evictZone->pagecnt_dirty--;
        Num_evict_dirty_cycle++;

        clearDesp(frozenDesp);
        evict_cnt++;
    }

    /* If end the dirty eviction */
    if(Cycle_Progress >= Cycle_Length || (CurEvictZoneSeq = get_FrozenOpZone_Seq()) < 0){
        /* End and set the eviction type to *Unknown*. */
        printf(">> Output of last Cycle: clean:%ld, dirty:%ld\n",Num_evict_clean_cycle,Num_evict_dirty_cycle);

        Num_evict_dirty_cycle = 0;
        Cycle_Progress = 0;
        WhoEvict_Now = EP_Reset;
    }

    //printf("pore+V2: batch flush dirty cnt [%d] from zone[%lu]\n", j,evictZone->zoneId);

//    printf("SCORE REPORT: zone id[%d], score[%lu]\n", evictZone->zoneId, evictZone->score);
    return evict_cnt;
}

static EvictPhrase_t run_cm_alpha()
{
    if(STT->incache_n_dirty == 0 || STT->incache_n_clean == 0)
        usr_error("Illegal to run CostModel:alpha");

    struct blk_cm_info blk_cm_info_drt={0,0}, blk_cm_info_cln={0,0};
    double cost_drt = -1, cost_cln = -1;

    /* Get number of dirty OODs. NOTICE! Have to get the dirty first and then the clean, the order cannot be reverted.*/
    cost_drt = redefineOpenZones();

    /* Get number of clean OODs. NOTICE! Have to get the dirty first and then the clean, the order cannot be reverted. */
    blkcnt_t despId_cln = CleanCtrl.tail;
    Dscptr_paul* cleandesp;
    int cnt = 0;
    while(cnt < NumEvict_thistime_apprx && despId_cln >= 0)
    {
        cleandesp = GlobalDespArray + despId_cln;
        if(cleandesp->stamp < OODstamp){
            blk_cm_info_cln.num_OODblks ++;
        }
        despId_cln = cleandesp->pre;
        cnt ++;
    }
    blk_cm_info_cln.num_totalblks = cnt;

    cost_cln = CM_Alpha.Cost_Clean(blk_cm_info_cln);

    printf(">>>[NEWCYCLE] CostModel(Alpha) C:%1f vs. D:%1f<<<\n", cost_cln, cost_drt);
    printf(">>>[NEWCYCLE] Cached Number C:%lu vs. D:%lu<<<\n", STT->incache_n_clean, STT->incache_n_dirty);

    /* Compare. */
    if(cost_drt == -1 && cost_cln == -1)
        usr_error("paul.c: alpha costmodel found the cost og dirty and clean are both -1");

    if(cost_cln < cost_drt){
        printf("~CLEAN\n\n");
        return EP_Clean;
    }
    else{
        printf("~DIRTY\n\n");
        return EP_Dirty;
    }
}


/****************
** Utilities ****
*****************/
/* Utilities for Dirty descriptors Array in each Zone*/

static void
hit(Dscptr_paul* desp, ZoneCtrl_pual* ZoneCtrl_pual)
{
    //ZoneCtrl_pual->heat++;
    //ZoneCtrl_pual->score -= (double) 1 / (1 << desp->heat);
}

static void
add2ArrayHead(Dscptr_paul* desp, ZoneCtrl_pual* ZoneCtrl_pual)
{
    if(ZoneCtrl_pual->head < 0)
    {
        //empty
        ZoneCtrl_pual->head = ZoneCtrl_pual->tail = desp->serial_id;
    }
    else
    {
        //unempty
        Dscptr_paul* headDesp = GlobalDespArray + ZoneCtrl_pual->head;
        desp->pre = -1;
        desp->next = ZoneCtrl_pual->head;
        headDesp->pre = desp->serial_id;
        ZoneCtrl_pual->head = desp->serial_id;
    }
}

static void
unloadfromZone(Dscptr_paul* desp, ZoneCtrl_pual* ZoneCtrl_pual)
{
    if(desp->pre < 0)
    {
        ZoneCtrl_pual->head = desp->next;
    }
    else
    {
        GlobalDespArray[desp->pre].next = desp->next;
    }

    if(desp->next < 0)
    {
        ZoneCtrl_pual->tail = desp->pre;
    }
    else
    {
        GlobalDespArray[desp->next].pre = desp->pre;
    }
    desp->pre = desp->next = -1;
}

static void
move2ArrayHead(Dscptr_paul* desp,ZoneCtrl_pual* ZoneCtrl_pual)
{
    unloadfromZone(desp, ZoneCtrl_pual);
    add2ArrayHead(desp, ZoneCtrl_pual);
}

static void
clearDesp(Dscptr_paul* desp)
{
    desp->ssd_buf_tag.offset = -1;
    desp->next = desp->pre = -1;
    desp->stamp = 0;
    desp->flag &= ~(SSD_BUF_DIRTY | SSD_BUF_VALID);
    desp->zoneId = -1;
}

/* Utilities for Global Clean Descriptors Array */
static void
add2CleanArrayHead(Dscptr_paul* desp)
{
    if(CleanCtrl.head < 0)
    {
        //empty
        CleanCtrl.head = CleanCtrl.tail = desp->serial_id;
    }
    else
    {
        //unempty
        Dscptr_paul* headDesp = GlobalDespArray + CleanCtrl.head;
        desp->pre = -1;
        desp->next = CleanCtrl.head;
        headDesp->pre = desp->serial_id;
        CleanCtrl.head = desp->serial_id;
    }
}

static void
unloadfromCleanArray(Dscptr_paul* desp)
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
move2CleanArrayHead(Dscptr_paul* desp)
{
    unloadfromCleanArray(desp);
    add2CleanArrayHead(desp);
}

/* Decision Method */
/** \brief
 *  Quick-Sort method to sort the zones by score.
    NOTICE!
        If the gap between variable 'start' and 'end'is too long, it will PROBABLY cause call stack OVERFLOW!
        So this function need to modify for better.
 */
static void
qsort_zone(long start, long end)
{
    long		i = start;
    long		j = end;

    long S = ZoneSortArray[start];
    ZoneCtrl_pual* curCtrl = ZoneCtrl_pualArray + S;
    unsigned long score = curCtrl->OOD_num;
    while (i < j)
    {
        while (!(ZoneCtrl_pualArray[ZoneSortArray[j]].OOD_num > score) && i<j)
        {
            j--;
        }
        ZoneSortArray[i] = ZoneSortArray[j];

        while (!(ZoneCtrl_pualArray[ZoneSortArray[i]].OOD_num < score) && i<j)
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

/*
  extract the non-empty zones and record them into the ZoneSortArray
 */
static long
extractNonEmptyZoneId()
{
    int zoneId = 0, cnt = 0;
    while(zoneId < NZONES)
    {
        ZoneCtrl_pual* zone = ZoneCtrl_pualArray + zoneId;
        if(zone->pagecnt_dirty
        > 0)
        {
            ZoneSortArray[cnt] = zoneId;
            cnt++;
        }
        zoneId++;

        if(WhoEvict_Before == 1 && zone->activate_after_n_cycles > 0)
            zone->activate_after_n_cycles --;
    }
    return cnt;
}

static void
pause_and_score()
{
    /*  For simplicity, searching all the zones of SMR,
        actually it's only needed to search the zones which had been cached.
        But it doesn't matter because of only 200~500K meta data of zones in memory for searching, it's not a big number.
    */
    /* Score all zones. */
    ZoneCtrl_pual* izone;
    Dscptr_paul* desp;
    blkcnt_t n = 0;

    OODstamp = StampGlobal - (long)(NBLOCK_SSD_CACHE * 0.8);
    while(n < NonEmptyZoneCnt)
    {
        izone = ZoneCtrl_pualArray + ZoneSortArray[n];
        izone->OOD_num = 0;

        /* score each block of the non-empty zone */

        blkcnt_t despId = izone->tail;
        while(despId >= 0)
        {
            desp = GlobalDespArray + despId;
            if(desp->stamp < OODstamp)
                izone->OOD_num ++;
            despId = desp->pre;
        }
        n++ ;
    }
}


static double redefineOpenZones()
{
    double cost_ret = 0; 
    struct blk_cm_info cm_drt[100];
    NonEmptyZoneCnt = extractNonEmptyZoneId(); // >< #ugly way.
    if(NonEmptyZoneCnt == 0)
        usr_error("There are no zone for open.");
    pause_and_score(); /** ARS (Actually Release Space) */
    qsort_zone(0,NonEmptyZoneCnt-1);

    long max_n_zones = Cycle_Length / (ZONESZ / BLKSZ);
    if(max_n_zones == 0)
        max_n_zones = 1;  // This is for Emulation on small traces, some of their fifo size are lower than a zone size.

    OpenZoneCnt = 0;
    long i = 0, k = 0;
    while(OpenZoneCnt < max_n_zones && i < NonEmptyZoneCnt)
    {
        ZoneCtrl_pual* zone = ZoneCtrl_pualArray + ZoneSortArray[i];

        /* According to the RULE 2, zones which have already be in PB cannot be choosed into this cycle. */
        if(zone->activate_after_n_cycles == 0)
        {
            zone->activate_after_n_cycles = 2;  // Deactivate the zone for the next 2 cycles.
            OpenZoneSet[OpenZoneCnt] = zone->zoneId;
            OpenZoneCnt++;

            cm_drt[k].num_OODblks = zone->OOD_num;
            cm_drt[k].num_totalblks = zone->pagecnt_dirty;
            k++;
        }
        else if(zone->activate_after_n_cycles > 0)
            //info("PAUL FILTERS A REPEAT ZONE.");
        i++;
    }

    cost_ret = CM_Alpha.Cost_Dirty(cm_drt, OpenZoneCnt);
    return cost_ret;
}

static int
get_FrozenOpZone_Seq()
{
    int seq = 0;
    blkcnt_t frozenSeq = -1;
    long frozenStamp = StampGlobal;
    while(seq < OpenZoneCnt)
    {
        ZoneCtrl_pual* ctrl = ZoneCtrl_pualArray + OpenZoneSet[seq];
        if(ctrl->pagecnt_dirty <= 0)
        {
            seq ++;
            continue;
        }

        Dscptr_paul* tail = GlobalDespArray + ctrl->tail;
        if(tail->stamp < frozenStamp)
        {
            frozenStamp = tail->stamp;
            frozenSeq = seq;
        }
        seq ++;
    }

    return frozenSeq;   // If return value <= 0, it means 1. here already has no any dirty block in the selected bands. 2. here has not started the cycle.
}

// static int random_pick(float weight1, float weight2, float obey)
// {
//     //return (weight1 < weight2) ? 1 : 2;
//     // let weight as the standard, set as 1,
//     float inc_times = (weight2 / weight1) - 1;
//     inc_times *= obey;

//     float de_point = 1000 * (1 / (2 + inc_times));
// //    rand_init();
//     int token = rand() % 1000;

//     if(token < de_point)
//         return 1;
//     return 2;
// }


/**************
 * Cost Model *
 * ************/

static microsecond_t costmodel_fx_wa(int blkcnt){
    microsecond_t lat_for_blkcnt = 728*blkcnt + 435833; // F(blkcnt) = RMW + k*blkcnt; <Regression function of actual test results>
    return lat_for_blkcnt;
}

static double costmodel_evaDirty_alpha(struct blk_cm_info * dirty, int num){
    if(num <= 0)
        return -1;
    double evaDirty = 0;
    int ood_num = 0;
    int i = 0;
    while(i < num){
        evaDirty += (double)CM_Alpha.FX_WA(dirty[i].num_totalblks);
        ood_num += dirty[i].num_OODblks;
        i++;
    }
    
    return evaDirty / (ood_num + 1);
}
static double costmodel_evaClean_alpha(struct blk_cm_info clean){
    double evaClean = \
    ( \
        (0 * clean.num_totalblks) + \
        (clean.num_totalblks - clean.num_OODblks) * CM_Alpha.Lat_SMR_read \
    ) / (clean.num_OODblks+1);
    return evaClean;
}
