#include <stdio.h>
#include <stdlib.h>

#include "../global.h"
#include "../cache.h"
#include "lru.h"
#include "lru_private.h"
#include "../shmlib.h"

#define EVICT_DITRY_GRAIN 64

/********
 ** SHM**
 ********/

static StrategyCtrl_LRU_private *self_strategy_ctrl;
static StrategyDesp_LRU_private	*strategy_desp;

static volatile void *addToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru);
static volatile void *deleteFromLRU(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru);
static volatile void *moveToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru);

/*
 * init buffer hash table, Strategy_control, buffer, work_mem
 */
int
initSSDBufferFor_LRU_private()
{
    int stat = multi_SHM_lock_n_check("LOCK_SSDBUF_STRATEGY_LRU");
    if(stat == 0)
    {
        strategy_desp = (StrategyDesp_LRU_private *)multi_SHM_alloc(SHM_SSDBUF_STRATEGY_DESP, sizeof(StrategyDesp_LRU_private) * NBLOCK_SSD_CACHE);

        StrategyDesp_LRU_private *ssd_buf_hdr_for_lru = strategy_desp;
        long i;
        for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_lru++, i++)
        {
            ssd_buf_hdr_for_lru->serial_id = i;
            ssd_buf_hdr_for_lru->next_self_lru = -1;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            multi_SHM_mutex_init(&ssd_buf_hdr_for_lru->lock);
        }
    }
    else
    {
        strategy_desp = (StrategyDesp_LRU_private *)multi_SHM_get(SHM_SSDBUF_STRATEGY_DESP, sizeof(StrategyDesp_LRU_private) * NBLOCK_SSD_CACHE);
    }
    multi_SHM_unlock("LOCK_SSDBUF_STRATEGY_LRU");

    self_strategy_ctrl = (StrategyCtrl_LRU_private *)malloc(sizeof(StrategyCtrl_LRU_private));
    self_strategy_ctrl->first_self_lru = -1;
    self_strategy_ctrl->last_self_lru = -1;

    return stat;
}

int
Unload_Buf_LRU_private(long * out_despid_array, int max_n_batch)
{
    int cnt = 0;
    while(cnt < EVICT_DITRY_GRAIN && cnt < max_n_batch)
    {
        long frozen_id = self_strategy_ctrl->last_self_lru;
        deleteFromLRU(&strategy_desp[frozen_id]);
        out_despid_array[cnt] = frozen_id;
        cnt ++ ;
    }

    return cnt;
}

int
hitInBuffer_LRU_private(long serial_id)
{
    StrategyDesp_LRU_private* ssd_buf_hdr_for_lru = &strategy_desp[serial_id];
    moveToLRUHead(ssd_buf_hdr_for_lru);
    return 0;
}

int
insertBuffer_LRU_private(long serial_id)
{
//    strategy_desp[serial_id].user_id = UserId;
    addToLRUHead(&strategy_desp[serial_id]);

    return 0;
}

static volatile void *
addToLRUHead(StrategyDesp_LRU_private* ssd_buf_hdr_for_lru)
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
deleteFromLRU(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru)
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
moveToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru)
{
    deleteFromLRU(ssd_buf_hdr_for_lru);
    addToLRUHead(ssd_buf_hdr_for_lru);
    return NULL;
}

