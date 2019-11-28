#ifndef SMR_SSD_CACHE_SCAN_H
#define SMR_SSD_CACHE_SCAN_H

#define DEBUG 0
/* ---------------------------scan---------------------------- */
#include <band_table.h>

typedef struct
{
	long 		ssd_buf_id;				// ssd buffer location in shared buffer
    long        next_scan;               // to link used ssd as SCAN
    long        last_scan;               // to link used ssd as SCAN
} SSDBufDespForSCAN;

typedef struct
{
	long start;
    long        scan_ptr;
} SSDBufferStrategyControlForSCAN;

SSDBufDespForSCAN	*ssd_buf_desps_for_scan;
SSDBufferStrategyControlForSCAN *ssd_buf_strategy_ctrl_for_scan;
BandHashBucket *band_hashtable;

extern unsigned long flush_times;
extern void initSSDBufferForSCAN();
extern SSDBufDesp *getSCANBuffer();
extern void *hitInSCANBuffer(SSDBufDesp *);
extern void insertByTag();
#endif
