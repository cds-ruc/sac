#ifndef _LRU_CDC_H_
#define _LRU_CDC_H_
#define DEBUG 0

#include "lru_private.h"
#include "../cache.h"

extern int initSSDBufferFor_LRU_CDC();
//extern int Unload_Buf_LRU_CDC(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type);
extern int hitInBuffer_LRU_CDC(long serial_id, unsigned flag);
extern int insertBuffer_LRU_CDC(long serial_id, unsigned flag);
extern int Unload_Buf_LRU_CDC(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type);

#endif // _LRU_PRIVATE_H_

