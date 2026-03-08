

#ifndef _AIX_SVR4_SHIMS_H_
#define _AIX_SVR4_SHIMS_H_

#include <sys/types.h>


#define getminor minor

void cmn_err(int lvl, char *fmt, ...);
#define CE_CONT 0
#define CE_NOTE 1
#define CE_WARN 2
#define CE_PANIC 3

#define dbg_printf(fmt) (printf(fmt))

#define dbg_putchar(c) (putchar(c))
#define dbg_getchar() (getchar())

void kmem_free(caddr_t mem, size_t size);

//#define kem_free(mem, size) kmem_free_impl((caddr_t)(mem), (size))

/* svr style*/
#define biodone(bp) iodone(bp)
#define kmem_alloc(size, flags) kmemalloc(size, flags)

caddr_t kmem_zalloc(size_t size, u_int flags);

char * strcpy (char * s1, char * s2);
time_t drv_usectohz(time_t microseconds);
void drv_usecwait(time_t microseconds);

void loopoutsw(int port, unsigned short * addr, int count);
void loopinsw(int port, unsigned short * addr, int count);
#define loutw(port, addr, count) loopoutsw(port, addr, count)
#define linw(port, addr, count) loopinsw(port, addr, count)

typedef int addr_t;
int valid_usr_range(addr_t addr, size_t bytes);

typedef struct uio uio_t;

#endif /* _AIX_SVR4_SHIMS_H_ */