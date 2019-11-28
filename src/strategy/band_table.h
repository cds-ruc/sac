#ifndef BANDTABLE_H
#define BANDTABLE_H

#define DEBUG 0
/*-----------------------------------band----------------------------*/
#define bool unsigned char
#define size_t long
typedef struct BandHashBucket
{
	long band_num;          // actual band # in SMR disk
	long band_id;           // location in SSD cache line
	struct BandHashBucket *next_item;
} BandHashBucket;

#define GetBandHashBucket(hash_code, band_hashtable) ((BandHashBucket *)(band_hashtable +(unsigned)(hash_code)))

extern unsigned long NBANDTables;

extern void initBandTable(size_t size, BandHashBucket **band_hashtable);
extern unsigned long bandtableHashcode(long band_num);
extern long bandtableLookup(long band_num,unsigned long hash_code, BandHashBucket * band_hashtable);
extern long bandtableInsert(long band_num,unsigned long hash_code,long band_id, BandHashBucket ** band_hashtable);
extern long bandtableDelete(long band_num,unsigned long hasd_code, BandHashBucket ** band_hashtable);
#endif    /*  BANDTABLE_H*/
