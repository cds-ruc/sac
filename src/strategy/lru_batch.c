#include <stdio.h>
#include <stdlib.h>

#include "../global.h"
#include "lru.h"
#include "../cache.h"
#include "lru_batch.h"
#include "../shmlib.h"
/********
 ** SHM**
 ********/

 struct timeval tv_1,tv_2;

static StrategyCtrl_LRU_batch *self_strategy_ctrl;
static StrategyDesp_LRU_batch	*strategy_desp;

static volatile void *addToLRUHead(StrategyDesp_LRU_batch * ssd_buf_hdr_for_lru);
static volatile void *deleteFromLRU(StrategyDesp_LRU_batch * ssd_buf_hdr_for_lru);
static volatile void *moveToLRUHead(StrategyDesp_LRU_batch * ssd_buf_hdr_for_lru);
static void sort(long *a, int left, int right);
/*
 * init buffer hash table, Strategy_control, buffer, work_mem
 */
int
initSSDBufferFor_LRU_batch()
{
    STT->cacheLimit = Param1;
    int stat = SHM_lock_n_check("LOCK_SSDBUF_STRATEGY_LRU");
    if(stat == 0)
    {
        strategy_desp = (StrategyDesp_LRU_batch *)SHM_alloc(SHM_SSDBUF_STRATEGY_DESP, sizeof(StrategyDesp_LRU_batch) * NBLOCK_SSD_CACHE);

        StrategyDesp_LRU_batch *ssd_buf_hdr_for_lru = strategy_desp;
        long i;
        for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_lru++, i++)
        {
            ssd_buf_hdr_for_lru->serial_id = i;
            ssd_buf_hdr_for_lru->next_self_lru = -1;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            SHM_mutex_init(&
                           ssd_buf_hdr_for_lru->lock);
        }
    }
    else
    {
        strategy_desp = (StrategyDesp_LRU_batch *)SHM_get(SHM_SSDBUF_STRATEGY_DESP, sizeof(StrategyDesp_LRU_batch) * NBLOCK_SSD_CACHE);
    }
    SHM_unlock("LOCK_SSDBUF_STRATEGY_LRU");

    self_strategy_ctrl = (StrategyCtrl_LRU_batch *)malloc(sizeof(StrategyCtrl_LRU_batch));
    self_strategy_ctrl->first_self_lru = -1;
    self_strategy_ctrl->last_self_lru = -1;
    return stat;
}

long
Unload_Buf_LRU_batch(long* unloads, int cnt)
{
    long frozen_id ;
    int i=0;
    while(i<cnt)
    {
        frozen_id = self_strategy_ctrl->last_self_lru;
        deleteFromLRU(&strategy_desp[frozen_id]);
        unloads[i] = frozen_id;
        i++;
    }
    //_TimerLap(&tv_1);
    //sort(unloads,0,cnt-1);
    //_TimerLap(&tv_2);
    //microsecond_t msec_sort = TimerInterval_MICRO(&tv_1,&tv_2);
    //printf("Sort use time: %lu(usec)\n",msec_sort);

    return frozen_id;
}


int
hitInBuffer_LRU_batch(long serial_id)
{
    StrategyDesp_LRU_batch* ssd_buf_hdr_for_lru = &strategy_desp[serial_id];
    moveToLRUHead(ssd_buf_hdr_for_lru);
    return 0;
}

void*
insertBuffer_LRU_batch(long serial_id)
{
    strategy_desp[serial_id].user_id = UserId;
    addToLRUHead(&strategy_desp[serial_id]);

    return 0;
}

static volatile void *
addToLRUHead(StrategyDesp_LRU_batch* ssd_buf_hdr_for_lru)
{
    //deal with self LRU queue
    if(self_strategy_ctrl->last_self_lru < 0)
    {
        self_strategy_ctrl->first_self_lru = ssd_buf_hdr_for_lru->serial_id;
        self_strategy_ctrl->last_self_lru = ssd_buf_hdr_for_lru->serial_id;
    }
    else
    {
        ssd_buf_hdr_for_lru->next_self_lru = strategy_desp[self_strategy_ctrl->first_self_lru].serial_id;
        ssd_buf_hdr_for_lru->last_self_lru = -1;
        strategy_desp[self_strategy_ctrl->first_self_lru].last_self_lru = ssd_buf_hdr_for_lru->serial_id;
        self_strategy_ctrl->first_self_lru =  ssd_buf_hdr_for_lru->serial_id;
    }
    return NULL;
}

static volatile void *
deleteFromLRU(StrategyDesp_LRU_batch * ssd_buf_hdr_for_lru)
{
    //deal with self queue
    if(ssd_buf_hdr_for_lru->last_self_lru>=0)
    {
        strategy_desp[ssd_buf_hdr_for_lru->last_self_lru].next_self_lru = ssd_buf_hdr_for_lru->next_self_lru;
    }
    else
    {
        self_strategy_ctrl->first_self_lru = ssd_buf_hdr_for_lru->next_self_lru;
    }

    if(ssd_buf_hdr_for_lru->next_self_lru>=0)
    {
        strategy_desp[ssd_buf_hdr_for_lru->next_self_lru].last_self_lru = ssd_buf_hdr_for_lru->last_self_lru;
    }
    else
    {
        self_strategy_ctrl->last_self_lru = ssd_buf_hdr_for_lru->last_self_lru;
    }

    ssd_buf_hdr_for_lru->last_self_lru = ssd_buf_hdr_for_lru->next_self_lru = -1;

    return NULL;
}

static volatile void *
moveToLRUHead(StrategyDesp_LRU_batch * ssd_buf_hdr_for_lru)
{
    deleteFromLRU(ssd_buf_hdr_for_lru);
    addToLRUHead(ssd_buf_hdr_for_lru);
    return NULL;
}

static void sort(long *a, int left, int right)
{
    if(left >= right)/*如果左边索引大于或者等于右边的索引就代表已经整理完成一个组了*/
    {
        return ;
    }
    int i = left;
    int j = right;
    int key = a[left];

    while(i < j)                               /*控制在当组内寻找一遍*/
    {
        while(i < j && key <= a[j])
        /*而寻找结束的条件就是，1，找到一个小于或者大于key的数（大于或小于取决于你想升
        序还是降序）2，没有符合条件1的，并且i与j的大小没有反转*/
        {
            j--;/*向前寻找*/
        }

        a[i] = a[j];
        /*找到一个这样的数后就把它赋给前面的被拿走的i的值（如果第一次循环且key是
        a[left]，那么就是给key）*/

        while(i < j && key >= a[i])
        /*这是i在当组内向前寻找，同上，不过注意与key的大小关系停止循环和上面相反，
        因为排序思想是把数往两边扔，所以左右两边的数大小与key的关系相反*/
        {
            i++;
        }

        a[j] = a[i];
    }

    a[i] = key;/*当在当组内找完一遍以后就把中间数key回归*/
    sort(a, left, i - 1);/*最后用同样的方式对分出来的左边的小组进行同上的做法*/
    sort(a, i + 1, right);/*用同样的方式对分出来的右边的小组进行同上的做法*/
                       /*当然最后可能会出现很多分左右，直到每一组的i = j 为止*/
}
