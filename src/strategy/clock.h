#define DEBUG 0
/* ---------------------------clock---------------------------- */

typedef struct
{
	long 		ssd_buf_id;				// ssd buffer location in shared buffer
	unsigned long	usage_count;
} SSDBufDespForClock;

typedef struct
{
	long		next_victimssd;		// For CLOCK
} SSDBufferStrategyControlForClock;

SSDBufDespForClock	*ssd_buf_desps_for_clock;
SSDBufferStrategyControlForClock *ssd_buf_strategy_ctrl_for_clock;

extern unsigned long flush_times;

extern void initSSDBufferForClock();
extern SSDBufDesp *getCLOCKBuffer();
extern void *hitInCLOCKBuffer(SSDBufDesp *);
