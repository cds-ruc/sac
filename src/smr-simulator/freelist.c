#include "freelist.h"
#include <unistd.h>

freelist_t freelist_create(long num)
{
    if(num <= 0)
        return NULL;

    freelist_t fl;
    fl.flist = fl.firstFree = (freeItem *)malloc(freeItem, sizeof(freeItem) * num);

    long i = 0;
    freeItem* item;
    while(1)
    {
        item = fl.firstFree + i;
        item->id = i;
        item->next_free = item + 1;
        item->pre_free = item - 1;

        if(++i == num)
        {
            item->next_free = NULL;
            fl.firstFree->pre_free = NULL;
            break;
        }
    }

    return fl;
}

long freelist_getfree(freelist_t fl)
{
    if(fl.firstFree == NULL)
        return -1;
    return fl.firstFree->id;
}

long freelist_free(freelist_t fl, unsigned long id)
{
    freeItem* item = fl.flist + id;
    /* find free item backward. */ // FUTURE WORK
    while(1){

    }

}

unsigned long freelist_unfree(freelist_t fl, unsigned long id)
{
    freeItem* item = fl.flist + id;

}
