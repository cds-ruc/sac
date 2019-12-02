#include <stdio.h>
#include <stdlib.h>

#include "inner_ssd_buf_table.h"
#include "simulator_v2.h"
#include "../cache.h"

static SSDHashBucket* HashTable;
static SSDHashBucket* HashItemPool,* FreeItem_hdr;
static SSDHashBucket* memPop();
static void memPush(SSDHashBucket* item);

#define GetSSDHashBucket(hash_code) ((SSDHashBucket *) (HashTable + (unsigned long) (hash_code)))

#define IsSame(T1, T2) ((T1.offset == T2.offset) ? 1 : 0)




void initSSDTable(size_t size)
{
	HashTable = (SSDHashBucket*)malloc(sizeof(SSDHashBucket)*size);
	HashItemPool = (SSDHashBucket*)malloc(sizeof(SSDHashBucket)*size);
	FreeItem_hdr = HashItemPool;

	size_t i;
	SSDHashBucket* bucket = HashTable;
	SSDHashBucket* freeitem = FreeItem_hdr;
	for (i = 0; i < size; bucket++, freeitem++, i++){
		freeitem->despId = bucket->despId = -1;
		freeitem->hash_key.offset = bucket->hash_key.offset = -1;
		bucket->next_item = NULL;
		freeitem->next_item = freeitem + 1;
	}
	FreeItem_hdr[size - 1].next_item = NULL;
}

unsigned long ssdtableHashcode(DespTag tag)
{
	unsigned long ssd_hash = (tag.offset / BLKSZ) % NBLOCK_SMR_PB;
	return ssd_hash;
}

long ssdtableLookup(DespTag tag, unsigned long hash_code)
{
	if (DEBUG)
		printf("[INFO] Lookup tag: %lu\n",tag.offset);
	SSDHashBucket *nowbucket = GetSSDHashBucket(hash_code);
	while (nowbucket != NULL) {
	//	printf("nowbucket->buf_id = %u %u %u\n", nowbucket->hash_key.rel.database, nowbucket->hash_key.rel.relation, nowbucket->hash_key.block_num);
		if (IsSame(nowbucket->hash_key, tag)) {
	//		printf("find\n");
			return nowbucket->despId;
		}
		nowbucket = nowbucket->next_item;
	}
//	printf("no find\n");

	return -1;
}

long ssdtableInsert(DespTag tag, unsigned long hash_code, long despId)
{
	if (DEBUG)
		printf("[INFO] Insert tag: %lu, hash_code=%lu\n",tag.offset, hash_code);
	SSDHashBucket *nowbucket = GetSSDHashBucket(hash_code);
	while (nowbucket->next_item != NULL) {
		nowbucket = nowbucket->next_item;
	}

    SSDHashBucket* newitem = memPop();
    newitem->hash_key = tag;
    newitem->despId = despId;
    newitem->next_item = NULL;
    nowbucket->next_item = newitem;

	return 0;
}

long ssdtableDelete(DespTag tag, unsigned long hash_code)
{
	if (DEBUG)
		printf("[INFO] Delete tag: %lu, hash_code=%lu\n",tag.offset, hash_code);
	SSDHashBucket *nowbucket = GetSSDHashBucket(hash_code);
	long del_id;
	SSDHashBucket *delitem;

	while (nowbucket->next_item != NULL) {
		if (IsSame(nowbucket->next_item->hash_key, tag)) {
            delitem = nowbucket->next_item;
            del_id = delitem->despId;
            nowbucket->next_item = delitem->next_item;
            memPush(delitem);
            return del_id;
		}
		nowbucket = nowbucket->next_item;
	}

	return -1;
}

long ssdtableUpdate(DespTag tag, unsigned long hash_code, long despId)
{
	if (DEBUG)
		printf("[INFO] Insert tag: %lu, hash_code=%lu\n",tag.offset, hash_code);
	SSDHashBucket* nowbucket = GetSSDHashBucket(hash_code);
	SSDHashBucket* lastbucket = nowbucket;
	while (nowbucket != NULL) {
        lastbucket = nowbucket;
        if (IsSame(nowbucket->hash_key,tag)) {
            long oldId = nowbucket->despId;
            nowbucket->despId = despId;
            return oldId;
		}
		nowbucket = nowbucket->next_item;
	}

	// if not exist in table, insert one.

    SSDHashBucket* newitem = memPop();
    newitem->hash_key = tag;
    newitem->despId = despId;
    newitem->next_item = NULL;
    lastbucket->next_item = newitem;
	return -1;
}

static SSDHashBucket* memPop(){
	if(FreeItem_hdr == NULL)
	{
		printf("SIMU: fifo hashtale poll overflow!\n");
		exit(-1);
	}
	SSDHashBucket* item = FreeItem_hdr;
	FreeItem_hdr = FreeItem_hdr->next_item;
	return item;
}
static void memPush(SSDHashBucket* item){
	item->next_item = FreeItem_hdr;
	FreeItem_hdr = item;
}
