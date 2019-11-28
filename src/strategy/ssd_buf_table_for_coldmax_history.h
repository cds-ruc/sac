#ifndef SSDBUFTABLE_H
#define SSDBUFTABLE_H

#define size_t long
#define GetSSDBufHashBucketForColdMaxHistory(hash_code) ((SSDBufHashBucket *) (ssd_buf_hashtable_history + (unsigned) (hash_code)))

#define SHM_SSDBUF__CTL_LRU  "SHM_SSDBUF_STRATEGY_CTL_LRU"

extern void HashTab_InitHistory(size_t size);
extern unsigned long HashTab_GetHashCodeHistory(SSDBufferTag *ssd_buf_tag);
extern long HashTab_LookupHistory(SSDBufferTag *ssd_buf_tag, unsigned long hash_code);
extern long HashTab_InsertHistory(SSDBufferTag *ssd_buf_tag, unsigned long hash_code, size_t ssd_buf_id);
extern long HashTab_DeleteHistory(SSDBufferTag *ssd_buf_tag, unsigned long hash_code);
#endif   /* SSDBUFTABLE_H */
