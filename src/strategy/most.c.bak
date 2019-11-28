#include <stdio.h>
#include <stdlib.h>
#include <global.h>
#include "most.h"

 long GetSMRBandNumFromSSD(unsigned long offset);

BandDescForMost	EvictedBand;

int
initSSDBufferForMost()
{
	initBandTable(NBANDTables, &band_hashtable_for_most);

	SSDBufDespForMost *ssd_buf_hdr_for_most;
	BandDescForMost *band_hdr_for_most;
	ssd_buf_desps_for_most = (SSDBufDespForMost *) malloc(sizeof(SSDBufDespForMost) * NBLOCK_SSD_CACHE);
	long		i;
	ssd_buf_hdr_for_most = ssd_buf_desps_for_most;
	for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_most++, i++) {
		ssd_buf_hdr_for_most->ssd_buf_id = i;
		ssd_buf_hdr_for_most->next_ssd_buf = -1;
	}

	band_descriptors_for_most = (BandDescForMost *) malloc(sizeof(BandDescForMost) * NZONES);
	band_hdr_for_most = band_descriptors_for_most;
	for (i = 0; i < NZONES; band_hdr_for_most++, i++) {
		band_hdr_for_most->band_num = 0;
		band_hdr_for_most->current_pages = 0;
		band_hdr_for_most->first_page = -1;
	}

	ssd_buf_strategy_ctrl_for_most = (SSDBufferStrategyControlForMost *) malloc(sizeof(SSDBufferStrategyControlForMost));
	ssd_buf_strategy_ctrl_for_most->nbands = 0;

	EvictedBand.band_num = 0;
	EvictedBand.first_page = -1;
	EvictedBand.current_pages = 0;
}

int
HitMostBuffer()
{
	return 1;
}

long LogOutDesp_most()
{
	long band_hash = 0;
	if(EvictedBand.first_page < 0){
		bandtableDelete(EvictedBand.band_num, bandtableHashcode(EvictedBand.band_num), &band_hashtable_for_most);

		BandDescForMost	temp;

		EvictedBand = band_descriptors_for_most[0];
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
				bandtableDelete(band_descriptors_for_most[child].band_num, band_hash, &band_hashtable_for_most);
				bandtableInsert(band_descriptors_for_most[child].band_num, band_hash, parent, &band_hashtable_for_most);
				parent = child;
				child = child * 2 + 1;
			}
		}
		band_descriptors_for_most[parent] = temp;
		band_descriptors_for_most[ssd_buf_strategy_ctrl_for_most->nbands - 1].band_num = -1;
		band_descriptors_for_most[ssd_buf_strategy_ctrl_for_most->nbands - 1].current_pages = 0;
		band_descriptors_for_most[ssd_buf_strategy_ctrl_for_most->nbands - 1].first_page = -1;
		ssd_buf_strategy_ctrl_for_most->nbands--;
		band_hash = bandtableHashcode(temp.band_num);
		bandtableDelete(temp.band_num, band_hash, &band_hashtable_for_most);
		bandtableInsert(temp.band_num, band_hash, parent, &band_hashtable_for_most);

	}

	long		band_num = EvictedBand.band_num;
	band_hash = bandtableHashcode(band_num);
	long		band_id = bandtableLookup(band_num, band_hash, band_hashtable_for_most);
	long		first_page = EvictedBand.first_page;

	ssd_buf_desps_for_most[first_page].next_ssd_buf = -1;

	return ssd_buf_desps_for_most[first_page].ssd_buf_id;
}

int LogInMostBuffer(long despId, SSDBufTag tag)
{
	long		band_num = GetSMRBandNumFromSSD(tag.offset);
	unsigned long	band_hash = bandtableHashcode(band_num);
	long		band_id = bandtableLookup(band_num, band_hash, band_hashtable_for_most);

	SSDBufDespForMost *ssd_buf_for_most;
	BandDescForMost *band_hdr_for_most;

	if (band_id >= 0) {
		//printf("hit band %ld\n", band_num);
		SSDBufDespForMost *new_ssd_buf_for_most;
		new_ssd_buf_for_most = &ssd_buf_desps_for_most[despId];
		new_ssd_buf_for_most->next_ssd_buf = band_descriptors_for_most[band_id].first_page;
		band_descriptors_for_most[band_id].first_page = despId;

		band_descriptors_for_most[band_id].current_pages++;
		BandDescForMost	temp;
		long		parent = (band_id - 1) / 2;
		long		child = band_id;
		while (parent >= 0 && band_descriptors_for_most[child].current_pages > band_descriptors_for_most[parent].current_pages) {
			temp = band_descriptors_for_most[child];
			band_descriptors_for_most[child] = band_descriptors_for_most[parent];
			band_hash = bandtableHashcode(band_descriptors_for_most[parent].band_num);
			bandtableDelete(band_descriptors_for_most[parent].band_num, band_hash, &band_hashtable_for_most);
			bandtableInsert(band_descriptors_for_most[parent].band_num, band_hash, child, &band_hashtable_for_most);
			band_descriptors_for_most[parent] = temp;
			band_hash = bandtableHashcode(temp.band_num);
			bandtableDelete(temp.band_num, band_hash, &band_hashtable_for_most);
			bandtableInsert(temp.band_num, band_hash, parent, &band_hashtable_for_most);

			child = parent;
			parent = (child - 1) / 2;
		}
	} else {
		ssd_buf_strategy_ctrl_for_most->nbands++;
		band_descriptors_for_most[ssd_buf_strategy_ctrl_for_most->nbands - 1].band_num = band_num;
		band_descriptors_for_most[ssd_buf_strategy_ctrl_for_most->nbands - 1].current_pages = 1;
		band_descriptors_for_most[ssd_buf_strategy_ctrl_for_most->nbands - 1].first_page = despId;
		bandtableInsert(band_num, band_hash, ssd_buf_strategy_ctrl_for_most->nbands - 1, &band_hashtable_for_most);
		SSDBufDespForMost *new_ssd_buf_for_most;
		new_ssd_buf_for_most = &ssd_buf_desps_for_most[despId];
		new_ssd_buf_for_most->next_ssd_buf = -1;
	}
}

long GetSMRBandNumFromSSD(unsigned long offset)
{
    long BNDSZ = 36*1024*1024;      // bandsize = 36MB  (18MB~36MB)
    long band_size_num = BNDSZ / 1024 / 1024 / 2 + 1;
    long num_each_size = NZONES / band_size_num;
    long        i, size, total_size = 0;
    for (i = 0; i < band_size_num; i++)
    {
        size = BNDSZ / 2 + i * 1024 * 1024;
        if (total_size + size * num_each_size > offset)
            return num_each_size * i + (offset - total_size) / size;
        total_size += size * num_each_size;
    }

    return 0;
}
