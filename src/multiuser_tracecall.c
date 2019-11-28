#include <sys/mman.h>

#include "cache.h"
int multiuser_access()
{
    return 0;
}

//write_block(off_t offset,int blkcnt, char *ssd_buffer)
//{
//    void           *ssd_buf_block;
//    bool		found;
//    int		returnCode = 0;
//
//    static SSDBufTag ssd_buf_tag;
//    static SSDBufDesp *ssd_buf_hdr;
//
//    ssd_buf_tag.offset = offset;
//    if (DEBUG)
//        printf("[INFO] write():-------offset=%lu\n", offset);
//    if (EvictStrategy == CMR)
//    {
//        flush_bands++;
//        returnCode = pwrite(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, offset);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] write_block():-------write to smr: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
//            exit(-1);
//        }
//        //returnCode = fsync(smr_fd);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] write_block():----------fsync\n");
//            exit(-1);
//        }
//
//    }
//    else if (EvictStrategy == SMR)
//    {
//        returnCode = smrwrite(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_tag.offset);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] write_block():---------SMR\n");
//            exit(-1);
//        }
//    }
//    else    //LRU ...
//    {
//        ssd_buf_hdr = SSDBufferAlloc(ssd_buf_tag, &found);
//        flush_ssd_blocks++;
//        if (flush_ssd_blocks % 10000 == 0)
//            printf("hit num:%lu   flush_ssd_blocks:%lu flush_times:%lu flush_fifo_blocks:%lu  flusd_bands:%lu\n ", hit_num, flush_ssd_blocks, flush_times, flush_fifo_blocks, flush_bands);
//        returnCode = pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] write():-------write to ssd: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
//            exit(-1);
//        }
//        //returnCode = fsync(ssd_fd);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] write_block():----------fsync\n");
//            exit(-1);
//        }
//
//        ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID | SSD_BUF_DIRTY;
//    }
//}
