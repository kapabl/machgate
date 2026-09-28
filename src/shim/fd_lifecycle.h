#ifndef MACHGATE_SHIM_FD_LIFECYCLE_H
#define MACHGATE_SHIM_FD_LIFECYCLE_H

__attribute__((visibility("hidden")))
void machgate_fd_did_close(int descriptor);

__attribute__((visibility("hidden")))
void machgate_fd_did_duplicate(int source_descriptor,
	                       int duplicate_descriptor);

#endif
