#include <u.h>
#include <lib9.h>
#include <inttypes.h>
#include <sys/types.h>
typedef uint32_t u32int;
typedef uint64_t u64int;
#include <libsec.h>

extern vlong _NSEC(void);

char *argv0; // this is needed because is referenced from lib9.

vlong
nsec(void)
{
	return _NSEC();
}

void
arc4random_buf(void *buf, size_t nbytes)
{
	genrandom(buf, nbytes);
}
