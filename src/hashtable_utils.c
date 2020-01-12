#include <stdio.h>
#include <stdlib.h>
#include "shmlib.h"
#include "cache.h"
#include "hashtable_utils.h"
#include "global.h"
#include "mcheck.h"

#define GetSSDBufHashBucket(hash_code) ((SSDBufHashBucket *) (ssd_buf_hashtable + (unsigned) (hash_code)))
#define isSameTag(tag1,tag2) (tag1.offset == tag2.offset)
extern void _LOCK(pthread_mutex_t* lock);
extern void _UNLOCK(pthread_mutex_t* lock);

SSDBufHashBucket* ssd_buf_hashtable;

static SSDBufHashBucket* hashitem_freelist;
static SSDBufHashBucket* topfree_ptr;
static SSDBufHashBucket* buckect_alloc();

static long insertCnt,deleteCnt;

static void freebucket(SSDBufHashBucket* bucket);
int HashTab_Init()
{
    insertCnt = deleteCnt = 0;
    ssd_buf_hashtable = (SSDBufHashBucket*)malloc(sizeof(SSDBufHashBucket)*NTABLE_SSD_CACHE);
    hashitem_freelist = (SSDBufHashBucket*)malloc(sizeof(SSDBufHashBucket)*NTABLE_SSD_CACHE);
    topfree_ptr = hashitem_freelist;

    if(ssd_buf_hashtable == NULL || hashitem_freelist == NULL)
        return -1;

    SSDBufHashBucket* bucket = ssd_buf_hashtable;
    SSDBufHashBucket* freebucket = hashitem_freelist;
    blksize_t i = 0;
    for(i = 0; i < NBLOCK_SSD_CACHE; bucket++, freebucket++, i++)
    {
        bucket->desp_serial_id = freebucket->desp_serial_id = -1;
        bucket->hash_key.offset = freebucket->hash_key.offset = -1;
        bucket->next_item = NULL;
        freebucket->next_item = freebucket + 1;
    }
    hashitem_freelist[NBLOCK_SSD_CACHE - 1].next_item = NULL;
    return 0;
}

unsigned long HashTab_GetHashCode(SSDBufTag ssd_buf_tag)
{
    unsigned long hashcode = (ssd_buf_tag.offset / BLKSZ) % NTABLE_SSD_CACHE;
    return hashcode;
}

long HashTab_Lookup(SSDBufTag ssd_buf_tag, unsigned long hash_code)
{
    if (DEBUG)
        printf("[INFO] Lookup ssd_buf_tag: %lu\n",ssd_buf_tag.offset);
    SSDBufHashBucket *nowbucket = GetSSDBufHashBucket(hash_code);
    while (nowbucket != NULL)
    {
        if (isSameTag(nowbucket->hash_key, ssd_buf_tag))
        {
            return nowbucket->desp_serial_id;
        }
        nowbucket = nowbucket->next_item;
    }

    return -1;
}

long HashTab_Insert(SSDBufTag ssd_buf_tag, unsigned long hash_code, long desp_serial_id)
{
    if (DEBUG)
        printf("[INFO] Insert buf_tag: %lu\n",ssd_buf_tag.offset);

    insertCnt++;
    //printf("hashitem alloc times:%d\n",insertCnt);

    SSDBufHashBucket *nowbucket = GetSSDBufHashBucket(hash_code);
    if(nowbucket == NULL)
    {
        printf("[ERROR] Insert HashBucket: Cannot get HashBucket.\n");
        exit(1);
    }
    while (nowbucket->next_item != NULL)
    {
        nowbucket = nowbucket->next_item;
    }

    SSDBufHashBucket* newitem;
    if((newitem  = buckect_alloc()) == NULL)
    {
        printf("hash bucket alloc failure\n");
        exit(-1);
    }
    newitem->hash_key = ssd_buf_tag;
    newitem->desp_serial_id = desp_serial_id;
    newitem->next_item = NULL;

    nowbucket->next_item = newitem;
    return 0;
}

long HashTab_Delete(SSDBufTag ssd_buf_tag, unsigned long hash_code)
{
    if (DEBUG)
        printf("[INFO] Delete buf_tag: %lu\n",ssd_buf_tag.offset);

    deleteCnt++;
    //printf("hashitem free times:%d\n",deleteCnt++);

    long del_id;
    SSDBufHashBucket *delitem;
    SSDBufHashBucket *nowbucket = GetSSDBufHashBucket(hash_code);

    while (nowbucket->next_item != NULL)
    {
        if (isSameTag(nowbucket->next_item->hash_key, ssd_buf_tag))
        {
            delitem = nowbucket->next_item;
            del_id = delitem->desp_serial_id;
            nowbucket->next_item = delitem->next_item;
            freebucket(delitem);
            return del_id;
        }
        nowbucket = nowbucket->next_item;
    }
    return -1;
}

static SSDBufHashBucket* buckect_alloc()
{
    if(topfree_ptr == NULL)
        return NULL;
    SSDBufHashBucket* freebucket = topfree_ptr;
    topfree_ptr = topfree_ptr->next_item;
    return freebucket;
}

static void freebucket(SSDBufHashBucket* bucket)
{
    bucket->next_item = topfree_ptr;
    topfree_ptr = bucket;
}
