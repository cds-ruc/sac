#ifndef _LRU_PRIVATE_H_
#define _LRU_PRIVATE_H_
#define DEBUG 0
/* ---------------------------lru---------------------------- */

typedef struct
{
	long 		serial_id;			// the corresponding descriptor serial number.
    long        next_self_lru;
    long        last_self_lru;
    int         user_id;
    pthread_mutex_t lock;
    long     	    stamp;
} StrategyDesp_LRU_private;

typedef struct
{
    long        first_self_lru;          // Head of list of LRU
    long        last_self_lru;           // Tail of list of LRU
    pthread_mutex_t lock;
    blkcnt_t    count;
} StrategyCtrl_LRU_private;

extern int initSSDBufferFor_LRU_private();
extern int Unload_Buf_LRU_private(long * out_despid_array, int max_n_batch);
extern int hitInBuffer_LRU_private(long serial_id);
extern int insertBuffer_LRU_private(long serial_id);
#endif // _LRU_PRIVATE_H_
