#include "darwin_errno.h"

#include <errno.h>

int machgate_darwin_errno_from_linux(int linux_errno)
{
	switch (linux_errno) {
	case EAGAIN:
		return 35;
	case EDEADLK:
		return 11;
	case ENOSYS:
		return 78;
	case ETIMEDOUT:
		return 60;
#if ENOTSUP != EOPNOTSUPP
	case ENOTSUP:
#endif
	case EOPNOTSUPP:
		return 45;
	default:
		return linux_errno;
	}
}
