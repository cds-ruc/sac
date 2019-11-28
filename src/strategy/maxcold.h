#define DEBUG 0
/* ---------------------------maxcold---------------------------- */

typedef struct
{
	SSDBufferTag 	ssd_buf_tag;
	long 		ssd_buf_id;				// ssd buffer location in shared buffer
    long        next_lru;               // to link used ssd as LRU
    long        last_lru;               // to link used ssd as LRU
    long        next_freessd;           // to link free ssd
    unsigned long   hit_times;
} SSDBufDespForMaxColdHistory;

typedef struct
{
	long 		ssd_buf_id;				// ssd buffer location in shared buffer
    long        next_lru;               // to link used ssd as LRU
    long        last_lru;               // to link used ssd as LRU
} SSDBufDespForMaxColdNow;

typedef struct
{
    long        first_lru;          // Head of list of LRU
    long        last_lru;           // Tail of list of LRU
    long        first_freessd;     // Head of list of free ssds
    long        last_freessd;      // Tail of list of free ssds
    long        n_usedssds;
} SSDBufferStrategyControlForMaxColdHistory;

typedef struct
{
    long        first_lru;          // Head of list of LRU
    long        last_lru;           // Tail of list of LRU
    long        n_usedssds;
} SSDBufferStrategyControlForMaxColdNow;

typedef struct
{
    unsigned long band_num;
    long current_hits;
    long current_pages;
    long current_cold_pages;
    long to_sort;
} BandDescForMaxColdHistory;

typedef struct
{
    unsigned char ischosen;
} BandDescForMaxColdNow;

extern size_t   ZONESZ;
extern unsigned long NBANDTables;
extern unsigned long NSMRBands;
extern unsigned long PERIODTIMES;
extern unsigned long NCOLDBAND;
extern unsigned long run_times;
extern unsigned long flush_times;

SSDBufHashBucket	*ssd_buf_hashtable_history;

SSDBufDespForMaxColdHistory *ssd_buf_desps_for_maxcold_history;
SSDBufDespForMaxColdNow *ssd_buf_desps_for_maxcold_now;
SSDBufferStrategyControlForMaxColdHistory *ssd_buf_strategy_ctrl_for_maxcold_history;
SSDBufferStrategyControlForMaxColdNow *ssd_buf_strategy_ctrl_for_maxcold_now;
BandDescForMaxColdHistory *band_descriptors_for_maxcold_history;
BandDescForMaxColdNow *band_descriptors_for_maxcold_now;

extern void initSSDBufferForMaxCold();
extern SSDBufDesp *getMaxColdBuffer(SSDBufferTag*, SSDEvictionStrategy);
extern void *hitInMaxColdBuffer(SSDBufDesp *);
