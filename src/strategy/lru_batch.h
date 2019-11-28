#ifndef _LRU_BATCH_H_
#define _LRU_BATCH_H_
#define DEBUG 0
/* ---------------------------lru---------------------------- */

typedef struct
{
	long 		serial_id;			// the corresponding descriptor serial number.
    long        next_self_lru;
    long        last_self_lru;
    int   user_id;
    pthread_mutex_t lock;
} StrategyDesp_LRU_batch;

typedef struct
{
    long        first_self_lru;          // Head of list of LRU
    long        last_self_lru;           // Tail of list of LRU
    pthread_mutex_t lock;
} StrategyCtrl_LRU_batch;

extern int initSSDBufferFor_LRU_batch();
extern long Unload_Buf_LRU_batch(long* unloads, int cnt);
extern int hitInBuffer_LRU_batch(long serial_id);
extern void *insertBuffer_LRU_batch(long serial_id);
#endif // _LRU_batch_H_
