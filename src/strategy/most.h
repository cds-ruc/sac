#ifndef _MOST_H_
#define _MOST_H_
#include "../cache.h"
#include "../global.h"

extern int Init_most();
extern int LogIn_most(long despId, SSDBufTag tag, unsigned flag);
extern int Hit_most(long despId, unsigned flag);
extern int LogOut_most(long * out_despid_array, int max_n_batch);
#endif // _PORE_H
