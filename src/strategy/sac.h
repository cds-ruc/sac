#ifndef _SAC_H_
#define _SAC_H_

#include "../cache.h"
#include "../global.h"

typedef struct Dscptr_sac
{
    long            serial_id;
    SSDBufTag       ssd_buf_tag;
    unsigned 	    flag;
    long            pre,next;
    long     	    stamp;
    unsigned long   zoneId;
}Dscptr_sac;

//typedef struct StrategyCtrl_pore
//{
//    long            freeId;     // Head of list of free ssds
//    long            n_used;
//}StrategyCtrl_pore;

typedef struct ZoneCtrl_pual
{
    unsigned int   zoneId;
    int            pagecnt_dirty;
    //int            pagecnt_clean;
    int            head,tail;
    int            OOD_num;
    int            activate_after_n_cycles;
}ZoneCtrl_pual;

extern int Init_SAC();
extern int LogIn_SAC(long despId, SSDBufTag tag, unsigned flag);
extern int Hit_SAC(long despId, unsigned flag);
extern int LogOut_SAC(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type);

#endif // _SAC_H_
