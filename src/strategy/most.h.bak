#define DEBUG 0
/*----------------------------------Most---------------------------------*/
#include "band_table.h"
#include "cache.h"

typedef struct
{
	long ssd_buf_id;//ssd buffer location in shared buffer
	long next_ssd_buf;
} SSDBufDespForMost;

typedef struct
{
	unsigned long band_num;
	unsigned long current_pages;
	unsigned long first_page;
} BandDescForMost;

typedef struct
{
    long        nbands;          // # of cached bands
} SSDBufferStrategyControlForMost;


SSDBufDespForMost *ssd_buf_desps_for_most;
BandDescForMost *band_descriptors_for_most;
SSDBufferStrategyControlForMost *ssd_buf_strategy_ctrl_for_most;
BandHashBucket *band_hashtable_for_most;

extern int initSSDBufferForMost();
extern int HitMostBuffer();
extern long LogOutDesp_most();
extern int LogInMostBuffer();
