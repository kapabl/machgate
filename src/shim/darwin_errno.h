#ifndef MACHGATE_SHIM_DARWIN_ERRNO_H
#define MACHGATE_SHIM_DARWIN_ERRNO_H

__attribute__((visibility("hidden")))
int machgate_darwin_errno_from_linux(int linux_errno);

#endif
