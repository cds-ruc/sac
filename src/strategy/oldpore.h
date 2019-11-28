#ifndef _OLDPORE_H_
#define _OLDPORE_H_

#include "../cache.h"
#include "../global.h"

extern int Init_oldpore();
extern int LogIn_oldpore(long despId, SSDBufTag tag, unsigned flag);
extern int Hit_oldpore(long despId, unsigned flag);
extern int LogOut_oldpore(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type);

#endif // _PAUL_H_
