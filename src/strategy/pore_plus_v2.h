#ifndef _PORE_H
#define _PORE_H
#include "pore.h"
#include "cache.h"
#include "global.h"

#endif // _PORE_H

#ifndef _PORE_PLUS_V2_H_
#define _PORE_PLUS_V2_H_



extern int Init_poreplus_v2();
extern int LogIn_poreplus_v2(long despId, SSDBufTag tag, unsigned flag);
extern int Hit_poreplus_v2(long despId, unsigned flag);
extern int LogOut_poreplus_v2(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type);

#endif // _PORE_PLUS_H_
