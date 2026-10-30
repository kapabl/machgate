#ifndef MACHGATE_SHIM_FILE_ABI_H
#define MACHGATE_SHIM_FILE_ABI_H

__attribute__((visibility("hidden")))
int machgate_linux_at_flags(int darwin_flags);

__attribute__((visibility("hidden")))
int machgate_linux_dirfd(int darwin_dirfd);

__attribute__((visibility("hidden")))
int machgate_linux_open_flags(int darwin_flags);

__attribute__((visibility("hidden")))
int machgate_darwin_open_flags(int linux_flags);

__attribute__((visibility("hidden")))
int machgate_darwin_open_uses_mode(int darwin_flags);

#endif
