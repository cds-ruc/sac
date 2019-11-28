#include <stdio.h>
#include <stdlib.h>

#include "ssd-cache.h"
#include "maxcold.h"
#include "ssd_buf_table_for_coldmax_history.h"

void HashTab_InitHistory(size_t size)
{
	ssd_buf_hashtable_history = (SSDBufHashBucket *)malloc(sizeof(SSDBufHashBucket)*size);
	size_t i;
	SSDBufHashBucket *ssd_buf_hash = ssd_buf_hashtable_history;
	for (i = 0; i < size; ssd_buf_hash++, i++){
		ssd_buf_hash->desp_serial_id = -1;
		ssd_buf_hash->hash_key.offset = -1;
		ssd_buf_hash->next_item = NULL;
	}
}

unsigned long HashTab_GetHashCodeHistory(SSDBufferTag *ssd_buf_tag)
{
	unsigned long ssd_buf_hash = (ssd_buf_tag->offset / SSD_BUFFER_SIZE) % NTABLE_SSD_CACHE;
	return ssd_buf_hash;
}

size_t HashTab_LookupHistory(SSDBufferTag *ssd_buf_tag, unsigned long hash_code)
{
	if (DEBUG)
		printf("[INFO] Lookup ssd_buf_tag: %lu\n",ssd_buf_tag->offset);
	SSDBufHashBucket *nowbucket = GetSSDBufHashBucketForColdMaxHistory(hash_code);
	while (nowbucket != NULL) {
		if (isSamebuf(&nowbucket->hash_key, ssd_buf_tag)) {
			return nowbucket->desp_serial_id;
		}
		nowbucket = nowbucket->next_item;
	}

	return -1;
}

long HashTab_InsertHistory(SSDBufferTag *ssd_buf_tag, unsigned long hash_code, long desp_serial_id)
{
	if (DEBUG)
		printf("[INFO] Insert buf_tag: %lu\n",ssd_buf_tag->offset);
	SSDBufHashBucket *nowbucket = GetSSDBufHashBucketForColdMaxHistory(hash_code);
	while (nowbucket->next_item != NULL && nowbucket != NULL) {
		if (isSamebuf(&nowbucket->hash_key, ssd_buf_tag)) {
			return nowbucket->desp_serial_id;
		}
		nowbucket = nowbucket->next_item;
	}
	if (nowbucket != NULL) {
		SSDBufHashBucket *newitem = (SSDBufHashBucket*)malloc(sizeof(SSDBufHashBucket));
		newitem->hash_key = *ssd_buf_tag;
		newitem->desp_serial_id = desp_serial_id;
		newitem->next_item = NULL;
		nowbucket->next_item = newitem;
	}
	else {
		nowbucket->hash_key = *ssd_buf_tag;
		nowbucket->desp_serial_id = desp_serial_id;
		nowbucket->next_item = NULL;
	}

	return -1;
}

long HashTab_DeleteHistory(SSDBufferTag *ssd_buf_tag, unsigned long hash_code)
{
	if (DEBUG)
		printf("[INFO] Delete buf_tag: %lu\n",ssd_buf_tag->offset);
	SSDBufHashBucket *nowbucket = GetSSDBufHashBucketForColdMaxHistory(hash_code);
	long del_id;
	SSDBufHashBucket *delitem;
	nowbucket->next_item;
	while (nowbucket->next_item != NULL && nowbucket != NULL) {
		if (isSamebuf(&nowbucket->next_item->hash_key, ssd_buf_tag)) {
			del_id = nowbucket->next_item->desp_serial_id;
			break;
		}
		nowbucket = nowbucket->next_item;
	}
	//printf("not found2\n");
	if (isSamebuf(&nowbucket->hash_key, ssd_buf_tag)) {
		del_id = nowbucket->desp_serial_id;
	}
	//printf("not found3\n");
	if (nowbucket->next_item != NULL) {
		delitem = nowbucket->next_item;
		nowbucket->next_item = nowbucket->next_item->next_item;
		free(delitem);
		return del_id;
	}
	else {
		delitem = nowbucket->next_item;
		nowbucket->next_item = NULL;
		free(delitem);
		return del_id;
	}

	return -1;
}
