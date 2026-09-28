#ifndef MACHGATE_APPLICATION_GUEST_SESSION_H
#define MACHGATE_APPLICATION_GUEST_SESSION_H

int machgate_run_guest(const char* executable_path, int argument_count,
		       char** arguments, char** environment);

#endif
