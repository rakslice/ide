

#ifndef _AIX_SVR4_SHIMS_H_
#define _AIX_SVR4_SHIMS_H_

#include <sys/types.h>


#define getminor minor

#define cmn_err(code, fmt) (printf(fmt))
#define cmn_err(code, fmt, a) (printf(fmt, a))
#define cmn_err(code, fmt, a, b) (printf(fmt, a, b))

#define dbg_printf(fmt) (printf(fmt))

#define dbg_putchar(c) (putchar(c))
#define dbg_getchar() (getchar())

void kmem_free(caddr_t mem, size_t size);

//#define kem_free(mem, size) kmem_free_impl((caddr_t)(mem), (size))

caddr_t kmem_zalloc(u_int flags, size_t size);

char * strcpy (char * s1, char * s2);
time_t drv_usectohz(time_t microseconds);
void drv_usecwait(time_t microseconds);

void loopoutsw(int port, unsigned short * addr, int count);
void loopinsw(int port, unsigned short * addr, int count);
#define loutw(port, addr, count) loopoutsw(port, addr, count)
#define linw(port, addr, count) loopinsw(port, addr, count)

typedef int addr_t;
int valid_usr_range(addr_t addr, size_t bytes);

#endif /* _AIX_SVR4_SHIMS_H_ */