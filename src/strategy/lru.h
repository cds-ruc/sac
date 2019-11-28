#ifndef _LRU_GLOBAL_H_
#define _LRU_GLOBAL_H_
#define DEBUG 0

/* ---------------------------lru---------------------------- */

typedef struct
{
	long 		serial_id;			// the corresponding descriptor serial number.
    long        next_lru;               // to link used SSD as LRU
    long        last_lru;               // to link used SSD as LRU
    pthread_mutex_t lock;
} StrategyDesp_LRU_global;

typedef struct
{
    long        first_lru;          // Head of list of LRU
    long        last_lru;           // Tail of list of LRU
    pthread_mutex_t lock;
} StrategyCtrl_LRU_global;

extern int initSSDBufferForLRU();
extern long Unload_LRUBuf();
extern int hitInLRUBuffer(long serial_id);
extern void *insertLRUBuffer(long serial_id);
#endif // _LRU_GLOBAL_H_
