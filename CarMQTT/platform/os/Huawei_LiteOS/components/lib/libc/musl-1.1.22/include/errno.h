#ifndef	_ERRNO_H
#define _ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <features.h>

#include <bits/errno.h>

#ifdef __GNUC__
__attribute__((const))
#endif

#ifdef __LITEOS__
extern void set_errno(int);
extern int get_errno(void);
/* internal function returning the address of the thread-specific errno */
extern volatile int* __errno(void);
#endif
int *__errno_location(void);
#ifndef __LITEOS__
#define errno (*__errno_location())
#else
#define errno (*__errno())
#endif

#ifdef _GNU_SOURCE
extern char *program_invocation_short_name, *program_invocation_name;
#endif

#ifdef __cplusplus
}
#endif

#endif

