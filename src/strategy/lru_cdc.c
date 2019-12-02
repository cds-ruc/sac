#include <stdio.h>
#include <stdlib.h>

#include "../global.h"
#include"../shmlib.h"
#include "lru_private.h"
#include "LRU_CDC.h"


#define EVICT_DITRY_GRAIN 64
#define stamp(desp) \
    desp->stamp = ++ StampGlobal;

/********
 ** SHM**
 ********/

static StrategyCtrl_LRU_private lru_dirty_ctrl, lru_clean_ctrl; //*self_strategy_ctrl,
static StrategyDesp_LRU_private	* strategy_desp;

static volatile void *addToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru, unsigned flag);
static volatile void *deleteFromLRU(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru);
static volatile void *moveToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru, unsigned flag);
static long                 StampGlobal;      /* Current io sequenced number in a period lenth, used to distinct the degree of heat among zones */

#define IsDirty(flag) ((flag & SSD_BUF_DIRTY) != 0)

/*
 * init buffer hash table, Strategy_control, buffer, work_mem
 */
int
initSSDBufferFor_LRU_CDC()
{
    //STT->cacheLimit = Param1;
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

    lru_dirty_ctrl.first_self_lru = lru_clean_ctrl.first_self_lru = -1;
    lru_dirty_ctrl.last_self_lru = lru_clean_ctrl.last_self_lru = -1;
    lru_dirty_ctrl.count = lru_clean_ctrl.count = 0;

    StampGlobal = 0;
    return stat;
}

int
Unload_Buf_LRU_CDC(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type)
{
    long frozen_id;
    int cnt = 0;
    StrategyDesp_LRU_private * victim;
    if(suggest_type == ENUM_B_Any)
    {
        if(lru_dirty_ctrl.last_self_lru < 0 || lru_clean_ctrl.last_self_lru < 0)
        {
            usr_warning("Order to evict any cache block, but one of them has exhausted in advance.");
        }
        if(strategy_desp[lru_dirty_ctrl.last_self_lru].stamp > strategy_desp[lru_clean_ctrl.last_self_lru].stamp)
            goto FLAG_EVICT_CLEAN;
        else
            goto FLAG_EVICT_DIRTY;
    }
    else if (suggest_type == ENUM_B_Dirty)
    {
        if(lru_dirty_ctrl.last_self_lru < 0)
            usr_warning("Order to evict [dirty] cache block, but one of them has exhausted in advance.");

        goto FLAG_EVICT_DIRTY;
    }
    else if (suggest_type == ENUM_B_Clean)
    {
        if(lru_clean_ctrl.last_self_lru < 0)
            usr_warning("Order to evict [clean] cache block, but one of them has exhausted in advance.");
        goto FLAG_EVICT_CLEAN;
    }
    else
    {
        usr_warning("Order to evict [Unknown] type block.");
    }

FLAG_EVICT_CLEAN:
    while(lru_clean_ctrl.last_self_lru >= 0 &&  cnt < EVICT_DITRY_GRAIN){
        victim =  strategy_desp + lru_clean_ctrl.last_self_lru;
        out_despid_array[cnt] = victim->serial_id;
        deleteFromLRU(victim);
        cnt ++ ;
    }
    return cnt;

FLAG_EVICT_DIRTY:
    while(lru_dirty_ctrl.last_self_lru >= 0 &&  cnt < EVICT_DITRY_GRAIN){
        victim =  strategy_desp + lru_dirty_ctrl.last_self_lru;
        out_despid_array[cnt] = victim->serial_id;
        deleteFromLRU(victim);
        cnt ++ ;
    }
    return cnt;
}

int
hitInBuffer_LRU_CDC(long serial_id, unsigned flag)
{
    StrategyDesp_LRU_private* ssd_buf_hdr_for_lru = &strategy_desp[serial_id];
    moveToLRUHead(ssd_buf_hdr_for_lru, flag);
    stamp(ssd_buf_hdr_for_lru);
    return 0;
}

int
insertBuffer_LRU_CDC(long serial_id, unsigned flag)
{
    //strategy_desp[serial_id].user_id = UserId;
    StrategyDesp_LRU_private * desp = strategy_desp + serial_id;
    addToLRUHead(desp, flag);
    stamp(desp);
    return 0;
}

static volatile void *
addToLRUHead(StrategyDesp_LRU_private* ssd_buf_hdr_for_lru, unsigned flag)
{
    //deal with self LRU queue
    if (IsDirty(flag))
    {
        if(lru_dirty_ctrl.first_self_lru < 0)
        {   // empty
            lru_dirty_ctrl.first_self_lru = ssd_buf_hdr_for_lru->serial_id;
            lru_dirty_ctrl.last_self_lru = ssd_buf_hdr_for_lru->serial_id;
        }
        else
        {
            ssd_buf_hdr_for_lru->next_self_lru = lru_dirty_ctrl.first_self_lru;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            strategy_desp[lru_dirty_ctrl.first_self_lru].last_self_lru = ssd_buf_hdr_for_lru->serial_id;
            lru_dirty_ctrl.first_self_lru =  ssd_buf_hdr_for_lru->serial_id;
        }
    }
    else
    {
        if(lru_clean_ctrl.first_self_lru < 0)
        {   // empty
            lru_clean_ctrl.first_self_lru = ssd_buf_hdr_for_lru->serial_id;
            lru_clean_ctrl.last_self_lru = ssd_buf_hdr_for_lru->serial_id;
        }
        else
        {
            ssd_buf_hdr_for_lru->next_self_lru = lru_clean_ctrl.first_self_lru;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            strategy_desp[lru_clean_ctrl.first_self_lru].last_self_lru = ssd_buf_hdr_for_lru->serial_id;
            lru_clean_ctrl.first_self_lru =  ssd_buf_hdr_for_lru->serial_id;
        }

    }

    return NULL;
}

static volatile void *
deleteFromLRU(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru)
{
    //deal with self queue
    if(ssd_buf_hdr_for_lru->last_self_lru >= 0)
    {
        strategy_desp[ssd_buf_hdr_for_lru->last_self_lru].next_self_lru = ssd_buf_hdr_for_lru->next_self_lru;
    }
    else
    {
        if(lru_clean_ctrl.first_self_lru == ssd_buf_hdr_for_lru->serial_id)
            lru_clean_ctrl.first_self_lru = ssd_buf_hdr_for_lru->next_self_lru;
        else if (lru_dirty_ctrl.first_self_lru == ssd_buf_hdr_for_lru->serial_id)
            lru_dirty_ctrl.first_self_lru = ssd_buf_hdr_for_lru->next_self_lru;
        else
            exit(-1);
    }

    if(ssd_buf_hdr_for_lru->next_self_lru >= 0)
    {
        strategy_desp[ssd_buf_hdr_for_lru->next_self_lru].last_self_lru = ssd_buf_hdr_for_lru->last_self_lru;
    }
    else
    {
        if(lru_clean_ctrl.last_self_lru == ssd_buf_hdr_for_lru->serial_id)
            lru_clean_ctrl.last_self_lru = ssd_buf_hdr_for_lru->last_self_lru;
        else if (lru_dirty_ctrl.last_self_lru == ssd_buf_hdr_for_lru->serial_id)
            lru_dirty_ctrl.last_self_lru = ssd_buf_hdr_for_lru->last_self_lru;
        else
            exit(-1);
    }

    ssd_buf_hdr_for_lru->last_self_lru = ssd_buf_hdr_for_lru->next_self_lru = -1;

    return NULL;
}

static volatile void *
moveToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru, unsigned flag)
{
    deleteFromLRU(ssd_buf_hdr_for_lru);
    addToLRUHead(ssd_buf_hdr_for_lru, flag);
    return NULL;
}


