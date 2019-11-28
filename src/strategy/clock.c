#include <stdio.h>
#include <stdlib.h>
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"
#include "clock.h"

/*
 * init strategy_control for clock
 */
void
initSSDBufferForClock()
{

	ssd_buf_strategy_ctrl_for_clock = (SSDBufferStrategyControlForClock *) malloc(sizeof(SSDBufferStrategyControlForClock));
	ssd_buf_strategy_ctrl_for_clock->next_victimssd = 0;

	ssd_buf_desps_for_clock = (SSDBufDespForClock *) malloc(sizeof(SSDBufDespForClock) * NBLOCK_SSD_CACHE);
	SSDBufDespForClock *ssd_buf_hdr_for_clock;
	long		i;
	ssd_buf_hdr_for_clock = ssd_buf_desps_for_clock;
	for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_clock++, i++) {
		ssd_buf_hdr_for_clock->ssd_buf_id = i;
		ssd_buf_hdr_for_clock->usage_count = 0;
	}
	flush_times = 0;
}

SSDBufDesp  *
getCLOCKBuffer()
{
	SSDBufDespForClock *ssd_buf_hdr_for_clock;
	SSDBufDesp  *ssd_buf_hdr;

	if (ssd_buf_desp_ctrl->first_freessd >= 0) {
		ssd_buf_hdr = &ssd_buf_desps[ssd_buf_desp_ctrl->first_freessd];
		ssd_buf_desp_ctrl->first_freessd = ssd_buf_hdr->next_freessd;
		ssd_buf_hdr->next_freessd = -1;
		ssd_buf_desp_ctrl->n_usedssd++;
		return ssd_buf_hdr;
	}
	flush_times++;
	for (;;) {
		ssd_buf_hdr_for_clock = &ssd_buf_desps_for_clock[ssd_buf_strategy_ctrl_for_clock->next_victimssd];
		ssd_buf_hdr = &ssd_buf_desps[ssd_buf_strategy_ctrl_for_clock->next_victimssd];
		ssd_buf_strategy_ctrl_for_clock->next_victimssd++;
		if (ssd_buf_strategy_ctrl_for_clock->next_victimssd >= NBLOCK_SSD_CACHE) {
			ssd_buf_strategy_ctrl_for_clock->next_victimssd = 0;
		}
		if (ssd_buf_hdr_for_clock->usage_count > 0) {
			ssd_buf_hdr_for_clock->usage_count--;
		} else {
			unsigned char	old_flag = ssd_buf_hdr->ssd_buf_flag;
			SSDBufferTag	old_tag = ssd_buf_hdr->ssd_buf_tag;
			if (DEBUG)
				printf("[INFO] allocSSDBuf(): old_flag&SSD_BUF_DIRTY=%d\n", old_flag & SSD_BUF_DIRTY);
			if ((old_flag & SSD_BUF_DIRTY) != 0) {
				flushSSDBuffer(ssd_buf_hdr);
			}
			if ((old_flag & SSD_BUF_VALID) != 0) {
				unsigned long	old_hash = HashTab_GetHashCode(&old_tag);
				HashTab_Delete(&old_tag, old_hash);
			}
			return ssd_buf_hdr;
		}
	}

	return NULL;
}


void           *
hitInCLOCKBuffer(SSDBufDesp * ssd_buf_hdr)
{
	SSDBufDespForClock *ssd_buf_hdr_for_clock;
	ssd_buf_hdr_for_clock = &ssd_buf_desps_for_clock[ssd_buf_hdr->ssd_buf_id];
	ssd_buf_hdr_for_clock->usage_count++;

	return NULL;
}
