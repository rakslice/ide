#include "aix_svr4_shims.h"

#include <sys/vmalloc.h>


caddr_t kmem_zalloc(size_t size, u_int flags) {
    caddr_t mem = kmemalloc(size, flags);
    if (mem) {
        bzero((char *)mem, size);
    }

    return mem;
}

/*
void biodone (struct buf *bp) {
    iodone(bp);
}
*/

void kmem_free(caddr_t mem, size_t size) {
    mfree(mem);
}

extern time_t hz;

time_t drv_usectohz(time_t microseconds) {
    // hz is in ticks per second, we want the number of ticks
    time_t result = hz * microseconds / 1000000;
    if (result == 0) result = 1;
    //printf("hz is %d wait ms %d ticks %d\n", hz, microseconds, result);
    return result;
}

#include <sys/time.h>

#define ITERATIONS_PER_USEC 80

void hddelayloop(int usec) {
	int i;
	for (i = ITERATIONS_PER_USEC * usec; i > 0; i--);
}

void drv_usecwait(time_t microseconds) {
    hddelayloop(microseconds);
}

char * strcpy(char * dst, char * src) {
    char * out = dst;
    while (*dst++ = *src++);
    return out;
}

int valid_usr_range(addr_t addr, size_t bytes) {
    return TRUE;
}

void cmn_err(int lvl, char *fmt, int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
	printf(fmt, a, b, c, d, e, f, g, h, i, j);
}
