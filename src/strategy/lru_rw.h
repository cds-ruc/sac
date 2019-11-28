#ifndef _LRU_RW_H_
#define _LRU_RW_H_
#define DEBUG 0

#include "lru_private.h"
#include "../cache.h"

extern int initSSDBufferFor_LRU_rw();
//extern int Unload_Buf_LRU_rw(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type);
extern int hitInBuffer_LRU_rw(long serial_id, unsigned flag);
extern int insertBuffer_LRU_rw(long serial_id, unsigned flag);
extern int Unload_Buf_LRU_rw(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type);

#endif // _LRU_PRIVATE_H_

