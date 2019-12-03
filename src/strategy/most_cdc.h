#ifndef _MOST_CDC_H_
#define _MOST_CDC_H_
#include "../cache.h"
#include "../global.h"

extern int Init_most_cdc();
extern int LogIn_most_cdc(long despId, SSDBufTag tag, unsigned flag);
extern int Hit_most_cdc(long despId, unsigned flag);
extern int LogOut_most_cdc(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type);

#endif // _MOST_CDC_H_
