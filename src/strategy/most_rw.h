#ifndef _MOST_RW_H_
#define _MOST_RW_H_
#include "pore.h"
#include "../cache.h"
#include "../global.h"

extern int Init_most_rw();
extern int LogIn_most_rw(long despId, SSDBufTag tag, unsigned flag);
extern int Hit_most_rw(long despId, unsigned flag);
extern int LogOut_most_rw(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type);
#endif // _MOST_RW_H_
