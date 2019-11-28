#include <stdio.h>
#include <stdlib.h>
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"
#include "lruofband.h"
#include "most.h"
#include "WA.h"
#include "band_table.h"

static volatile void moveFromMostToLRUofBand(long);

void
initSSDBufferForWA()
{
    initSSDBufferForLRUofBand();
    initSSDBufferForMost();
}

static volatile void
moveFromMostToLRUofBand(long band_id_for_most)
{
    BandDescForMost band_hdr_for_most;
    BandDescForMost temp;

    if (band_id_for_most > 0) {
        printf("[ERROR] moveFromMostToLRUofBand():-------move from Most: band_id_for_most(%ld) > 0\n", band_id_for_most);
        exit(-1);
    }

	band_hdr_for_most = band_descriptors_for_most[0];
	temp = band_descriptors_for_most[ssd_buf_strategy_ctrl_for_most->nbands - 1];
	long		parent = 0;
	long		child = parent * 2 + 1;
	while (child < ssd_buf_strategy_ctrl_for_most->nbands) {
		if (child < ssd_buf_strategy_ctrl_for_most->nbands && band_descriptors_for_most[child].current_pages < band_descriptors_for_most[child + 1].current_pages)
			child++;
		if (temp.current_pages >= band_descriptors_for_most[child].current_pages)
			break;
		else {
			band_descriptors_for_most[parent] = band_descriptors_for_most[child];
			long		band_hash = bandtableHashcode(band_descriptors_for_most[child].band_num);
			bandtableDelete(band_descriptors_for_most[child].band_num, band_hash, band_hashtable_for_most);
			bandtableInsert(band_descriptors_for_most[child].band_num, band_hash, parent, band_hashtable_for_most);
			parent = child;
			child = child * 2 + 1;
		}
	}
	band_descriptors_for_most[parent] = temp;
	band_descriptors_for_most[ssd_buf_strategy_ctrl_for_most->nbands - 1].band_num = -1;
	band_descriptors_for_most[ssd_buf_strategy_ctrl_for_most->nbands - 1].current_pages = 0;
	band_descriptors_for_most[ssd_buf_strategy_ctrl_for_most->nbands - 1].first_page = -1;
	ssd_buf_strategy_ctrl_for_most->nbands--;
	long		band_hash = bandtableHashcode(temp.band_num);
	bandtableDelete(temp.band_num, band_hash, band_hashtable_for_most);
	bandtableInsert(temp.band_num, band_hash, parent, band_hashtable_for_most);

	long		band_num = band_hdr_for_most.band_num;
	band_hash = bandtableHashcode(band_num);
	long		band_id;
	long		first_page = band_hdr_for_most.first_page;

	SSDBufDespForLRUofBand  *ssd_buf_hdr_for_lruofband;

    // add band in lruofband
    long		temp_first_freeband = band_control->first_freeband;
    bandtableInsert(band_num, band_hash, temp_first_freeband, band_hashtable_for_lruofband);
    band_control->first_freeband = band_descriptors[temp_first_freeband].next_free_band;
    band_descriptors[temp_first_freeband].next_free_band = -1;
    band_descriptors[temp_first_freeband].current_pages = 1;
    band_descriptors[temp_first_freeband].band_num = band_num;
    band_descriptors[temp_first_freeband].first_page = first_page;
    ssd_buf_desp_for_lruofband[first_page].next_ssd_buf = -1;

    // insert this page into lruofband lru queue
    ssd_buf_hdr_for_lruofband = &ssd_buf_desp_for_lruofband[first_page];
    ssd_buf_hdr_for_lruofband->next_lru = ssd_buf_desp_for_lruofband[ssd_buf_strategy_ctrl_lruofband->first_lru].ssd_buf_id;
    ssd_buf_hdr_for_lruofband->last_lru = -1;
    ssd_buf_desp_for_lruofband[ssd_buf_strategy_ctrl_lruofband->first_lru].last_lru = ssd_buf_hdr_for_lruofband->ssd_buf_id;
    ssd_buf_strategy_ctrl_lruofband->first_lru = ssd_buf_hdr_for_lruofband->ssd_buf_id;

	while (first_page >= 0) {
        // insert this page into lruofband band
	    band_id = bandtableLookup(band_num, band_hash, band_hashtable_for_lruofband);

        long		old_first_page = band_descriptors[band_id].first_page;
        ssd_buf_hdr_for_lruofband = &ssd_buf_desp_for_lruofband[old_first_page];
        SSDBufDespForLRUofBand *new_ssd_buf_hdr_for_lruofband;
        new_ssd_buf_hdr_for_lruofband = &ssd_buf_desp_for_lruofband[first_page];
        new_ssd_buf_hdr_for_lruofband->next_ssd_buf = ssd_buf_hdr_for_lruofband->next_ssd_buf;
        ssd_buf_hdr_for_lruofband->next_ssd_buf = first_page;

        // insert this page into lruofband lru queue
		ssd_buf_hdr_for_lruofband = &ssd_buf_desp_for_lruofband[first_page];
		ssd_buf_hdr_for_lruofband->next_lru = ssd_buf_desp_for_lruofband[ssd_buf_strategy_ctrl_lruofband->first_lru].ssd_buf_id;
		ssd_buf_hdr_for_lruofband->last_lru = -1;
		ssd_buf_desp_for_lruofband[ssd_buf_strategy_ctrl_lruofband->first_lru].last_lru = ssd_buf_hdr_for_lruofband->ssd_buf_id;
		ssd_buf_strategy_ctrl_lruofband->first_lru = ssd_buf_hdr_for_lruofband->ssd_buf_id;

        // get next page in most band
        first_page = ssd_buf_desps_for_most[first_page].next_ssd_buf;
		ssd_buf_desps_for_most[ssd_buf_desp_ctrl->first_freessd].next_ssd_buf = -1;
	}

    // delete this band from most band queue
	band_id = bandtableLookup(band_num, band_hash, band_hashtable_for_most);
    if (band_id >= 0)
    	bandtableDelete(band_num, band_hash, band_hashtable_for_most);
    else {
        printf("[ERROR] moveFromMostToLRUofBand():-------delete band from most hash table: band_num=%ld\n", band_num);
        exit(-1);
    }


}

SSDBufDesp  *
getWABuffer(SSDBufferTag *ssd_buf_tag)
{
	long		band_num = GetSMRBandNumFromSSD(ssd_buf_tag->offset);
	unsigned long	band_hash = bandtableHashcode(band_num);
	long		band_id_for_lruofband = bandtableLookup(band_num, band_hash, band_hashtable_for_lruofband);
    SSDBufDesp *ssd_buf_hdr;

    if (band_id_for_lruofband >= 0)
        ssd_buf_hdr = getLRUofBandBuffer(ssd_buf_tag);
    else {
        ssd_buf_hdr = getMostBuffer(ssd_buf_tag);
        long band_id_for_most = bandtableLookup(band_num, band_hash, band_hashtable_for_most);
        if (band_descriptors[band_id_for_most].current_pages * WRITEAMPLIFICATION >= BNDSZ/BLCKSZ) {
            moveFromMostToLRUofBand(band_id_for_most);
        }
    }

    return ssd_buf_hdr;
}

void
hitInWABuffer(SSDBufDesp * ssd_buf_hdr)
{
	long		band_num = GetSMRBandNumFromSSD(ssd_buf_hdr->ssd_buf_tag.offset);
	unsigned long	band_hash = bandtableHashcode(band_num);
	long		band_id = bandtableLookup(band_num, band_hash, band_hashtable_for_lruofband);

    if (band_id >= 0)
        hitInLRUofBandBuffer(ssd_buf_hdr);
    else
        hitInMostBuffer();
}
