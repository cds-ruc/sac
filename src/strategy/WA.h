#define DEBUG 0
/*----------------------------------Most---------------------------------*/
#include <band_table.h>

extern unsigned long WRITEAMPLIFICATION;

void initSSDBufferForWA();
SSDBufDesp *getWABuffer(SSDBufferTag*);
void hitInMostBuffer();
