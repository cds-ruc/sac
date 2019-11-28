#include <stdio.h>
#include <stdlib.h>
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"
#include "lruofband.h"
#include "band_table.h"

static volatile void *addToLRUofBandHead(SSDBufDespForLRUofBand * ssd_buf_hdr_for_lruofband);
static volatile void *deleteFromLRUofBand(SSDBufDespForLRUofBand * ssd_buf_hdr_for_lruofband);
static volatile void *moveToLRUofBandHead(SSDBufDespForLRUofBand * ssd_buf_hdr_for_lruofband);
static volatile void *addToBand(SSDBufferTag *ssd_buf_tag, long freessd);
static volatile void *getSSDBufferofBand(SSDBufferTag ssd_buf_tag);

void
initSSDBufferForLRUofBand()
{
	initBandTable(NBANDTables, &band_hashtable_for_lruofband);

	ssd_buf_strategy_ctrl_lruofband = (SSDBufferStrategyControlForLRUofBand *) malloc(sizeof(SSDBufferStrategyControlForLRUofBand));
	ssd_buf_strategy_ctrl_lruofband->first_lru = -1;
	ssd_buf_strategy_ctrl_lruofband->last_lru = -1;

	SSDBufDespForLRUofBand *ssd_buf_hdr_for_lruofband;
	ssd_buf_desp_for_lruofband = (SSDBufDespForLRUofBand *) malloc(sizeof(SSDBufDespForLRUofBand) * NBLOCK_SSD_CACHE);
	long		i;
	ssd_buf_hdr_for_lruofband = ssd_buf_desp_for_lruofband;
	for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_lruofband++, i++) {
		ssd_buf_hdr_for_lruofband->ssd_buf_id = i;
		ssd_buf_hdr_for_lruofband->next_lru = -1;
		ssd_buf_hdr_for_lruofband->last_lru = -1;
		ssd_buf_hdr_for_lruofband->next_ssd_buf = -1;
	}

	flush_times = 0;

	band_descriptors = (BandDesc *) malloc(sizeof(BandDesc) * NBLOCK_SSD_CACHE);
	BandDesc       *temp_band_desc;
	temp_band_desc = band_descriptors;

	for (i = 0; i < NBLOCK_SSD_CACHE; temp_band_desc++, i++) {
		band_descriptors[i].band_num = -1;
        band_descriptors[i].current_pages = 0;
		band_descriptors[i].first_page = -1;
		band_descriptors[i].next_free_band = i + 1;
	}
	band_descriptors[i].next_free_band = -1;
	band_control = (BandControl *) malloc(sizeof(BandControl));
	band_control->first_freeband = 0;
	band_control->last_freeband = NBLOCK_SSD_CACHE - 1;
	band_control->n_usedband = 0;

}

static volatile void *
addToLRUofBandHead(SSDBufDespForLRUofBand * ssd_buf_hdr_for_lruofband)
{
	if (ssd_buf_desp_ctrl->n_usedssd == 0) {
		ssd_buf_strategy_ctrl_lruofband->first_lru = ssd_buf_hdr_for_lruofband->ssd_buf_id;
		ssd_buf_strategy_ctrl_lruofband->last_lru = ssd_buf_hdr_for_lruofband->ssd_buf_id;
	} else {
		ssd_buf_hdr_for_lruofband->next_lru = ssd_buf_desp_for_lruofband[ssd_buf_strategy_ctrl_lruofband->first_lru].ssd_buf_id;
		ssd_buf_hdr_for_lruofband->last_lru = -1;
		ssd_buf_desp_for_lruofband[ssd_buf_strategy_ctrl_lruofband->first_lru].last_lru = ssd_buf_hdr_for_lruofband->ssd_buf_id;
		ssd_buf_strategy_ctrl_lruofband->first_lru = ssd_buf_hdr_for_lruofband->ssd_buf_id;
	}
	return NULL;
}

static volatile void *
deleteFromLRUofBand(SSDBufDespForLRUofBand * ssd_buf_hdr_for_lruofband)
{

	if (ssd_buf_hdr_for_lruofband->last_lru >= 0) {
		ssd_buf_desp_for_lruofband[ssd_buf_hdr_for_lruofband->last_lru].next_lru = ssd_buf_hdr_for_lruofband->next_lru;
	} else {
		ssd_buf_strategy_ctrl_lruofband->first_lru = ssd_buf_hdr_for_lruofband->next_lru;
	}
	if (ssd_buf_hdr_for_lruofband->next_lru >= 0) {
		ssd_buf_desp_for_lruofband[ssd_buf_hdr_for_lruofband->next_lru].last_lru = ssd_buf_hdr_for_lruofband->last_lru;
	} else {
		ssd_buf_strategy_ctrl_lruofband->last_lru = ssd_buf_hdr_for_lruofband->last_lru;
	}

	return NULL;
}

static volatile void *
addToBand(SSDBufferTag *ssd_buf_tag, long first_freessd)
{
	long		band_num = GetSMRBandNumFromSSD(ssd_buf_tag->offset);
	unsigned long	band_hash = bandtableHashcode(band_num);
	long		band_id = bandtableLookup(band_num, band_hash, band_hashtable_for_lruofband);

	SSDBufDespForLRUofBand *ssd_buf_for_lruofband;
	if (band_id >= 0) {
		long		first_page = band_descriptors[band_id].first_page;
		band_descriptors[band_id].current_pages ++;
		ssd_buf_for_lruofband = &ssd_buf_desp_for_lruofband[first_page];
		SSDBufDespForLRUofBand *new_ssd_buf_for_lruofband;
		new_ssd_buf_for_lruofband = &ssd_buf_desp_for_lruofband[first_freessd];

		new_ssd_buf_for_lruofband->next_ssd_buf = ssd_buf_for_lruofband->next_ssd_buf;
		ssd_buf_for_lruofband->next_ssd_buf = first_freessd;
	} else {
		long		temp_first_freeband = band_control->first_freeband;
		bandtableInsert(band_num, band_hash, temp_first_freeband, &band_hashtable_for_lruofband);
		band_control->first_freeband = band_descriptors[temp_first_freeband].next_free_band;
		band_descriptors[temp_first_freeband].next_free_band = -1;
		band_descriptors[temp_first_freeband].current_pages = 1;
		band_descriptors[temp_first_freeband].band_num = band_num;
		band_descriptors[temp_first_freeband].first_page = first_freessd;
		ssd_buf_desp_for_lruofband[first_freessd].next_ssd_buf = -1;
	}
	return NULL;
}

static volatile void *
getSSDBufferofBand(SSDBufferTag ssd_buf_tag)
{
	long		band_num = GetSMRBandNumFromSSD(ssd_buf_tag.offset);
	unsigned long	band_hash = bandtableHashcode(band_num);
	long		band_id = bandtableLookup(band_num, band_hash, band_hashtable_for_lruofband);
	long		first_page = band_descriptors[band_id].first_page;

	SSDBufDesp  *ssd_buf_hdr;
	SSDBufDespForLRUofBand *ssd_buf_for_lruofband;
	SSDBufferTag	old_tag;
	unsigned char	old_flag;
	unsigned long	old_hash;

	while (first_page >= 0) {
		ssd_buf_for_lruofband = &ssd_buf_desp_for_lruofband[first_page];
		ssd_buf_hdr = &ssd_buf_desps[first_page];

		ssd_buf_hdr->next_freessd = ssd_buf_desp_ctrl->first_freessd;
		ssd_buf_desp_ctrl->first_freessd = first_page;
		first_page = ssd_buf_for_lruofband->next_ssd_buf;

		deleteFromLRUofBand(ssd_buf_for_lruofband);

		old_flag = ssd_buf_hdr->ssd_buf_flag;
		old_tag = ssd_buf_hdr->ssd_buf_tag;
		if ((old_flag & SSD_BUF_DIRTY) != 0) {
			flushSSDBuffer(ssd_buf_hdr);
		}
		if ((old_flag & SSD_BUF_VALID) != 0) {
			old_hash = HashTab_GetHashCode(&old_tag);
			HashTab_Delete(&old_tag, old_hash);
		}
		ssd_buf_desp_ctrl->n_usedssd--;
	}
	bandtableDelete(band_num, band_hash, &band_hashtable_for_lruofband);
	band_descriptors[band_id].next_free_band = band_control->first_freeband;
	band_descriptors[band_id].current_pages = 0;
	band_control->first_freeband = band_id;
}

SSDBufDesp  *
getLRUofBandBuffer(SSDBufferTag *ssd_buf_tag)
{
	SSDBufDesp  *ssd_buffer_hdr;
	SSDBufDespForLRUofBand *ssd_buf_hdr_for_lruofband;
	if (ssd_buf_desp_ctrl->first_freessd < 0) {
		flush_times++;
		ssd_buffer_hdr = &ssd_buf_desps[ssd_buf_strategy_ctrl_lruofband->last_lru];
		getSSDBufferofBand(ssd_buffer_hdr->ssd_buf_tag);
	}
	ssd_buffer_hdr = &ssd_buf_desps[ssd_buf_desp_ctrl->first_freessd];
	ssd_buf_hdr_for_lruofband = &ssd_buf_desp_for_lruofband[ssd_buf_desp_ctrl->first_freessd];
	ssd_buffer_hdr->ssd_buf_tag.offset = ssd_buf_tag->offset;
	addToBand(ssd_buf_tag, ssd_buf_desp_ctrl->first_freessd);

	ssd_buf_desp_ctrl->first_freessd = ssd_buffer_hdr->next_freessd;
	ssd_buffer_hdr->next_freessd = -1;
	addToLRUofBandHead(ssd_buf_hdr_for_lruofband);
	ssd_buf_desp_ctrl->n_usedssd++;
	return ssd_buffer_hdr;
}

static volatile void *
moveToLRUofBandHead(SSDBufDespForLRUofBand * ssd_buf_hdr_for_lruofband)
{
	deleteFromLRUofBand(ssd_buf_hdr_for_lruofband);
	addToLRUofBandHead(ssd_buf_hdr_for_lruofband);

	return NULL;
}

void
hitInLRUofBandBuffer(SSDBufDesp * ssd_buf_hdr)
{
	moveToLRUofBandHead(&ssd_buf_desp_for_lruofband[ssd_buf_hdr->ssd_buf_id]);
}
