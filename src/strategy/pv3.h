#ifndef _PV3_H_
#define _PV3_H_

#include "../cache.h"
#include "../global.h"

typedef struct Dscptr_pv3
{
    long            serial_id;
    SSDBufTag       ssd_buf_tag;
    unsigned 	    flag;
    long            pre,next;
    unsigned long   heat;
    long     	    stamp;
    unsigned long   zoneId;
}Dscptr_pv3;

//typedef struct StrategyCtrl_pore
//{
//    long            freeId;     // Head of list of free ssds
//    long            n_used;
//}StrategyCtrl_pore;

typedef struct ZoneCtrl_pv3
{
    unsigned long   zoneId;
    long            heat;
    long            pagecnt_dirty;
    long            pagecnt_clean;
    long            head,tail;
    int             activate_after_n_cycles;
    unsigned long score;

}ZoneCtrl_pv3;

extern int Init_PUAL();
extern int LogIn_PAUL(long despId, SSDBufTag tag, unsigned flag);
extern int Hit_PAUL(long despId, unsigned flag);
extern int LogOut_PAUL(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type);

#endif // _PV3_H_
