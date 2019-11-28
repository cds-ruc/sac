#include <stdio.h>
#include <stdlib.h>

#include "../global.h"
#include "lru.h"
#include "../shmlib.h"
#include "../cache.h"
/********
 ** SHM**
 ********/
static StrategyCtrl_LRU_global *strategy_ctrl;
static StrategyDesp_LRU_global	*strategy_desp;

static volatile void *addToLRUHead(StrategyDesp_LRU_global * ssd_buf_hdr_for_lru);
static volatile void *deleteFromLRU(StrategyDesp_LRU_global * ssd_buf_hdr_for_lru);
static volatile void *moveToLRUHead(StrategyDesp_LRU_global * ssd_buf_hdr_for_lru);
static int hasBeenDeleted(StrategyDesp_LRU_global* ssd_buf_hdr_for_lru);
/*
 * init buffer hash table, Strategy_control, buffer, work_mem
 */
int
initSSDBufferForLRU()
{
    int stat = SHM_lock_n_check("LOCK_SSDBUF_STRATEGY_LRU");
    if(stat == 0)
    {
        strategy_ctrl =(StrategyCtrl_LRU_global *)SHM_alloc(SHM_SSDBUF_STRATEGY_CTRL,sizeof(StrategyCtrl_LRU_global));
        strategy_desp = (StrategyDesp_LRU_global *)SHM_alloc(SHM_SSDBUF_STRATEGY_DESP, sizeof(StrategyDesp_LRU_global) * NBLOCK_SSD_CACHE);

        strategy_ctrl->first_lru = -1;
        strategy_ctrl->last_lru = -1;
        SHM_mutex_init(&strategy_ctrl->lock);

        StrategyDesp_LRU_global *ssd_buf_hdr_for_lru = strategy_desp;
        long i;
        for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_lru++, i++)
        {
            ssd_buf_hdr_for_lru->serial_id = i;
            ssd_buf_hdr_for_lru->next_lru = -1;
            ssd_buf_hdr_for_lru->last_lru = -1;
            SHM_mutex_init(&
                           ssd_buf_hdr_for_lru->lock);
        }
    }
    else
    {
        strategy_ctrl =(StrategyCtrl_LRU_global *)SHM_get(SHM_SSDBUF_STRATEGY_CTRL,sizeof(StrategyCtrl_LRU_global));
        strategy_desp = (StrategyDesp_LRU_global *)SHM_get(SHM_SSDBUF_STRATEGY_DESP, sizeof(StrategyDesp_LRU_global) * NBLOCK_SSD_CACHE);

    }
    SHM_unlock("LOCK_SSDBUF_STRATEGY_LRU");
    return stat;
}

long
Unload_LRUBuf()
{
    _LOCK(&strategy_ctrl->lock);

    long frozen_id = strategy_ctrl->last_lru;
    deleteFromLRU(&strategy_desp[frozen_id]);

    _UNLOCK(&strategy_ctrl->lock);
    return frozen_id;
}

int
hitInLRUBuffer(long serial_id)
{
    _LOCK(&strategy_ctrl->lock);

    StrategyDesp_LRU_global* ssd_buf_hdr_for_lru = &strategy_desp[serial_id];
    if(hasBeenDeleted(ssd_buf_hdr_for_lru))
    {
        _UNLOCK(&strategy_ctrl->lock);
        return -1;
    }
    moveToLRUHead(ssd_buf_hdr_for_lru);
    _UNLOCK(&strategy_ctrl->lock);

    return 0;
}

void*
insertLRUBuffer(long serial_id)
{
    _LOCK(&strategy_ctrl->lock);

    addToLRUHead(&strategy_desp[serial_id]);
    _UNLOCK(&strategy_ctrl->lock);
    return 0;
}


static volatile void *
addToLRUHead(StrategyDesp_LRU_global* ssd_buf_hdr_for_lru)
{
    if (strategy_ctrl->last_lru < 0)
    {
        strategy_ctrl->first_lru = ssd_buf_hdr_for_lru->serial_id;
        strategy_ctrl->last_lru = ssd_buf_hdr_for_lru->serial_id;
    }
    else
    {
        ssd_buf_hdr_for_lru->next_lru = strategy_desp[strategy_ctrl->first_lru].serial_id;
        ssd_buf_hdr_for_lru->last_lru = -1;
        strategy_desp[strategy_ctrl->first_lru].last_lru = ssd_buf_hdr_for_lru->serial_id;
        strategy_ctrl->first_lru = ssd_buf_hdr_for_lru->serial_id;
    }
    return NULL;
}

static volatile void *
deleteFromLRU(StrategyDesp_LRU_global * ssd_buf_hdr_for_lru)
{
    if (ssd_buf_hdr_for_lru->last_lru >= 0)
    {
        strategy_desp[ssd_buf_hdr_for_lru->last_lru].next_lru = ssd_buf_hdr_for_lru->next_lru;
    }
    else
    {
        strategy_ctrl->first_lru = ssd_buf_hdr_for_lru->next_lru;
    }
    if (ssd_buf_hdr_for_lru->next_lru >= 0)
    {
        strategy_desp[ssd_buf_hdr_for_lru->next_lru].last_lru = ssd_buf_hdr_for_lru->last_lru;
    }
    else
    {
        strategy_ctrl->last_lru = ssd_buf_hdr_for_lru->last_lru;
    }

    ssd_buf_hdr_for_lru->last_lru = ssd_buf_hdr_for_lru->next_lru = -1;
    return NULL;
}

static volatile void *
moveToLRUHead(StrategyDesp_LRU_global * ssd_buf_hdr_for_lru)
{
    deleteFromLRU(ssd_buf_hdr_for_lru);
    addToLRUHead(ssd_buf_hdr_for_lru);
    return NULL;
}

static int
hasBeenDeleted(StrategyDesp_LRU_global* ssd_buf_hdr_for_lru)
{
    if(ssd_buf_hdr_for_lru->last_lru < 0 && ssd_buf_hdr_for_lru->next_lru < 0)
        return 1;
    else
        return 0;
}
