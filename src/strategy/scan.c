#include <stdio.h>
#include <stdlib.h>
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"
#include "scan.h"

static volatile void* addToSCANHead(SSDBufDespForSCAN *ssd_buf_hdr_for_scan);
static volatile void* deleteFromSCAN(long ssd_buf_id);
static volatile void* moveToSCANHead(SSDBufDespForSCAN *ssd_buf_hdr_for_scan);

/*
 * init buffer hash table, strategy_control, buffer, work_mem
 */
void initSSDBufferForSCAN()
{
	ssd_buf_strategy_ctrl_for_scan = (SSDBufferStrategyControlForSCAN *) malloc(sizeof(SSDBufferStrategyControlForSCAN));
	ssd_buf_strategy_ctrl_for_scan->scan_ptr = -1;
	ssd_buf_strategy_ctrl_for_scan->start= -1;
 // ssd_buf_strategy_ctrl_for_scan->last_scan = -1;

	ssd_buf_desps_for_scan = (SSDBufDespForSCAN *) malloc(sizeof(SSDBufDespForSCAN)*NBLOCK_SSD_CACHE);
	SSDBufDespForSCAN *ssd_buf_hdr_for_scan;
	//ssd_buf_hdr_for_scan is a pointer
	long i;
	ssd_buf_hdr_for_scan = ssd_buf_desps_for_scan;

	for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_scan++, i++) {
		ssd_buf_hdr_for_scan->ssd_buf_id = i;
        ssd_buf_hdr_for_scan->next_scan = -1;
        ssd_buf_hdr_for_scan->last_scan = -1;

	}
	flush_times = 0;
}

static volatile void* addToSCANHead(SSDBufDespForSCAN *ssd_buf_hdr_for_scan)
{
  /*  if (ssd_buf_desp_ctrl->n_usedssd == 0) {
//modi
       ssd_buf_strategy_ctrl_for_scan->scan_ptr = ssd_buf_hdr_for_scan->ssd_buf_id;
        } else {
ssd_buf_hdr_for_scan 指针->
        ssd_buf_hdr_for_scan->next_scan = ssd_buf_desps_for_scan[ssd_buf_strategy_ctrl_for_scan->first_scan].ssd_buf_id;
        ssd_buf_hdr_for_scan->last_scan = -1;
        ssd_buf_desps_for_scan[ssd_buf_strategy_ctrl_for_scan->first_scan].last_scan = ssd_buf_hdr_for_scan->ssd_buf_id;
        ssd_buf_strategy_ctrl_for_scan->first_scan = ssd_buf_hdr_for_scan->ssd_buf_id;
    }
    ssd_buf_desp_ctrl->n_usedssd ++;

    return NULL;
*/
}
void output(){
	SSDBufDespForSCAN *ssd_buf_hdr_for_scan;
	//ssd_buf_hdr_for_scan is a pointer
	long i, j;
	ssd_buf_hdr_for_scan = ssd_buf_desps_for_scan;

	for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_scan++, i++) {
	  printf("ssd_buf_id %ld",ssd_buf_hdr_for_scan->ssd_buf_id);
      	  printf("next_scan %ld",ssd_buf_hdr_for_scan->next_scan);
      	  printf("last_scan %ld\n",ssd_buf_hdr_for_scan->last_scan);
	}
	for(j = 0 ; j < NBLOCK_SSD_CACHE; j++){
	 printf("ssd_buf_tag no.%d tag is %ld\n",j,ssd_buf_desps[j].ssd_buf_tag.offset);
	}
	printf("scanptr %ld\n",ssd_buf_strategy_ctrl_for_scan->scan_ptr);
	printf("start %ld\n",ssd_buf_strategy_ctrl_for_scan->start);


}
void insertByTag(SSDBufferTag *ssd_buf_tag, long ssd_buf_id){
		printf("start %ld\n",ssd_buf_strategy_ctrl_for_scan->start);
	if(ssd_buf_strategy_ctrl_for_scan->start == -1){
		ssd_buf_strategy_ctrl_for_scan->start = ssd_buf_id;
		ssd_buf_strategy_ctrl_for_scan->scan_ptr = ssd_buf_id;
//		printf("insert %ld",ssd_buf_strategy_ctrl_for_scan->start);
//		printf("check that should be -1 %ld\n",ssd_buf_desps_for_scan[ssd_buf_id].next_scan);
	}
	else{

	long movePtr;
	movePtr = ssd_buf_strategy_ctrl_for_scan->start;
	if(ssd_buf_desps[movePtr].ssd_buf_tag.offset > ssd_buf_tag->offset){
		printf("ENTER HERE\n");
		ssd_buf_desps_for_scan[movePtr].last_scan = ssd_buf_id;
		ssd_buf_desps_for_scan[ssd_buf_id].next_scan = movePtr;
		ssd_buf_strategy_ctrl_for_scan -> start = ssd_buf_id;
	}else{

	while(ssd_buf_desps[movePtr].ssd_buf_tag.offset <= ssd_buf_tag->offset){
//		printf("%ld, %ld\n", ssd_buf_desps[movePtr].ssd_buf_tag.offset,ssd_buf_tag.offset );

		if(ssd_buf_desps_for_scan[movePtr].next_scan!=-1){
//printf("movePtr before %ld\n",movePtr);
		long next = ssd_buf_desps_for_scan[movePtr].next_scan;
			if(ssd_buf_desps[next].ssd_buf_tag.offset<= ssd_buf_tag->offset){
				movePtr = next;
				}
			else
				break;
//printf("movePtr %ld\n",movePtr);
		}
		else
			break;
		}
	long  temp ;
	temp = ssd_buf_desps_for_scan[movePtr].next_scan;
	ssd_buf_desps_for_scan[movePtr].next_scan = ssd_buf_id;
	ssd_buf_desps_for_scan[ssd_buf_id].last_scan =	ssd_buf_desps_for_scan[movePtr].ssd_buf_id;
	if(temp!=-1){
		ssd_buf_desps_for_scan[temp].last_scan =  ssd_buf_id;
		ssd_buf_desps_for_scan[ssd_buf_id].next_scan = temp;
	}else{
		ssd_buf_desps_for_scan[ssd_buf_id].next_scan = temp;
	}
	}

}
output();

}

static volatile void* deleteFromSCAN(long ssd_buf_id)
{
	long last;
	long next;
	last = ssd_buf_desps_for_scan[ssd_buf_id].last_scan;
	next = ssd_buf_desps_for_scan[ssd_buf_id].next_scan;

	ssd_buf_desps_for_scan[last].next_scan = ssd_buf_desps_for_scan[ssd_buf_id].next_scan;
	ssd_buf_desps_for_scan[next].last_scan = ssd_buf_desps_for_scan[ssd_buf_id].last_scan;
	ssd_buf_desps_for_scan[ssd_buf_id].last_scan = -1;
	ssd_buf_desps_for_scan[ssd_buf_id].next_scan = -1;

/*    if (ssd_buf_hdr_for_scan->last_scan >= 0) {
        ssd_buf_desps_for_scan[ssd_buf_hdr_for_scan->last_scan].next_scan=ssd_buf_hdr_for_scan->next_scan;
    } else {
        ssd_buf_strategy_ctrl_for_scan->first_scan = ssd_buf_hdr_for_scan->next_scan;
    }
    if (ssd_buf_hdr_for_scan->next_scan >= 0 ) {
       ssd_buf_desps_for_scan[ssd_buf_hdr_for_scan->next_scan].last_scan=ssd_buf_hdr_for_scan->last_scan;
    } else {
        ssd_buf_strategy_ctrl_for_scan->last_scan = ssd_buf_hdr_for_scan->last_scan;
    }
    ssd_buf_desp_ctrl->n_usedssd --;

    return NULL;
*/
}

static volatile void* moveToSCANHead(SSDBufDespForSCAN *ssd_buf_hdr_for_scan)
{
  /*  deleteFromSCAN(ssd_buf_hdr_for_scan);
    addToSCANHead(ssd_buf_hdr_for_scan);

    return NULL;
*/
}

SSDBufDesp *getSCANBuffer(SSDBufferTag *ssd_buf_tag)
{
	SSDBufDesp *ssd_buf_hdr;
	SSDBufDespForSCAN *ssd_buf_hdr_for_scan;

	if(ssd_buf_desp_ctrl->first_freessd <0){
		flush_times++;
		ssd_buf_hdr = &ssd_buf_desps[ssd_buf_strategy_ctrl_for_scan->scan_ptr];
		if(ssd_buf_hdr->ssd_buf_id == ssd_buf_strategy_ctrl_for_scan->start){
			ssd_buf_strategy_ctrl_for_scan->start = ssd_buf_desps_for_scan[ssd_buf_strategy_ctrl_for_scan->scan_ptr].next_scan;
		}
/*if the next is -1*/
		if(ssd_buf_desps_for_scan[ssd_buf_strategy_ctrl_for_scan->scan_ptr].next_scan != -1){
			ssd_buf_strategy_ctrl_for_scan->scan_ptr = ssd_buf_desps_for_scan[ssd_buf_strategy_ctrl_for_scan->scan_ptr].next_scan;
		}else{
	//		ssd_buf_desps_for_scan[ssd_buf_desps_for_scan[ssd_buf_strategy_ctrl_for_scan->scan_ptr].
			ssd_buf_strategy_ctrl_for_scan->scan_ptr = ssd_buf_strategy_ctrl_for_scan->start;
		}

 		unsigned char   old_flag = ssd_buf_hdr->ssd_buf_flag;
        	SSDBufferTag    old_tag = ssd_buf_hdr->ssd_buf_tag;
        	if (DEBUG)
                	printf("[INFO] allocSSDBuf(): old_flag&SSD_BUF_DIRTY=%d\n", old_flag & SSD_BUF_DIRTY);
        	if ((old_flag & SSD_BUF_DIRTY) != 0) {
                	flushSSDBuffer(ssd_buf_hdr);
       		}
        	if ((old_flag & SSD_BUF_VALID) != 0) {
                	unsigned long   old_hash = HashTab_GetHashCode(&old_tag);
                	HashTab_Delete(&old_tag, old_hash);
        	}
		deleteFromSCAN(ssd_buf_hdr->ssd_buf_id);
		ssd_buf_hdr->next_freessd = ssd_buf_desp_ctrl->first_freessd;
		ssd_buf_desp_ctrl->first_freessd = ssd_buf_hdr->ssd_buf_id;

	}


	if (ssd_buf_desp_ctrl->first_freessd >=0 ) {
	//	printf("Enter freessd\n");
		ssd_buf_hdr = &ssd_buf_desps[ssd_buf_desp_ctrl->first_freessd];
		ssd_buf_hdr_for_scan = &ssd_buf_desps_for_scan[ssd_buf_desp_ctrl->first_freessd];
		ssd_buf_desp_ctrl->first_freessd = ssd_buf_hdr->next_freessd;
		ssd_buf_hdr->next_freessd = -1;
		// request has known tag
		insertByTag(ssd_buf_tag,ssd_buf_hdr->ssd_buf_id);
	        return ssd_buf_hdr;
    }
    return ssd_buf_hdr;
}

void *hitInSCANBuffer(SSDBufDesp *ssd_buf_hdr)
{
 //   moveToSCANHead(&ssd_buf_desps_for_scan[ssd_buf_hdr->ssd_buf_id]);

    return NULL;
}
