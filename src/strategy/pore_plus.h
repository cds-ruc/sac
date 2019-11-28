#ifndef _PORE_H
#define _PORE_H
#include "pore.h"
#include "cache.h"
#include "global.h"

#endif // _PORE_H

#ifndef _PORE_PLUS_H_
#define _PORE_PLUS_H_



extern int InitPORE_plus();
extern int LogInPoreBuffer_plus(long despId, SSDBufTag tag, unsigned flag);
extern int HitPoreBuffer_plus(long despId, unsigned flag);
extern long LogOutDesp_pore_plus();


#endif // _PORE_PLUS_H_
