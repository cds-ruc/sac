#define DEBUG 0
/*-------------------------lruofband-------------------------------*/
#include <band_table.h>

// extern unsigned long NSMRBands;
typedef struct {
	long		ssd_buf_id;
	              //ssd buffer location in shared buffer
	long		next_lru;
	              //to link used ssd as LRU
	long		last_lru;
	              //to link used ssd as LRU
	long		next_ssd_buf;
}		SSDBufDespForLRUofBand;

typedef struct {
	long		first_lru;
	              //Head of list of LRU
	long		last_lru;
	              //Tail of list of LRU
}		SSDBufferStrategyControlForLRUofBand;

typedef struct {
	long		band_num;
    long        current_pages;
	long		first_page;
	long		next_free_band;
}		BandDesc;

typedef struct {
	long		first_freeband;
	long		last_freeband;
	long		n_usedband;
}		BandControl;

extern unsigned long NBANDTables;
extern unsigned long flush_times;

SSDBufDespForLRUofBand *ssd_buf_desp_for_lruofband;
BandDesc       *band_descriptors;
SSDBufferStrategyControlForLRUofBand *ssd_buf_strategy_ctrl_lruofband;
BandControl    *band_control;
BandHashBucket *band_hashtable_for_lruofband;

void		initSSDBufferForLRUofBand();
SSDBufDesp  *getLRUofBandBuffer(SSDBufTag*);
void           *hinInLRUofBandBuffer(SSDBufDesp *);
