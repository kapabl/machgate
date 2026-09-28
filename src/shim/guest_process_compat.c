#define _GNU_SOURCE

#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

extern char** environ;

static void* guest_process_value(const char* symbol)
{
	return dlsym(RTLD_DEFAULT, symbol);
}

static const char* guest_executable_path(void)
{
	const char** location = guest_process_value(
		"__machgate_guest_executable_path");
	if (!location)
		return NULL;
	return *location;
}

int* _NSGetArgc(void)
{
	static int empty_argument_count;
	int* argument_count = guest_process_value("__machgate_guest_argc");
	return argument_count ? argument_count : &empty_argument_count;
}

char*** _NSGetArgv(void)
{
	static char** empty_arguments;
	char*** arguments = guest_process_value("__machgate_guest_argv");
	return arguments ? arguments : &empty_arguments;
}

char*** _NSGetEnviron(void)
{
	char*** environment = guest_process_value("__machgate_guest_envp");
	if (environment && *environment)
		return environment;
	return &environ;
}

static char* find_guest_environment_value(const char* name)
{
	if (!name)
		return NULL;

	char*** environment = guest_process_value("__machgate_guest_envp");
	if (!environment || !*environment)
		return NULL;

	size_t name_length = strlen(name);
	for (size_t entry_index = 0; (*environment)[entry_index]; entry_index++) {
		char* entry = (*environment)[entry_index];
		char* separator = strchr(entry, '=');
		if (separator && (size_t)(separator - entry) == name_length &&
		    memcmp(entry, name, name_length) == 0)
			return separator + 1;
	}
	return NULL;
}

char* machgate_getenv(const char* name) __asm__("getenv");
char* machgate_getenv(const char* name)
{
	char* value = find_guest_environment_value(name);
	if (value)
		return value;

	typedef char* (*host_getenv_operation)(const char*);
	static host_getenv_operation host_getenv;
	if (!host_getenv)
		host_getenv = (host_getenv_operation)dlsym(RTLD_NEXT, "getenv");
	return host_getenv ? host_getenv(name) : NULL;
}

static int read_process_executable(pid_t process, char* buffer,
				   size_t buffer_size)
{
	char link_path[64];
	int link_length = snprintf(link_path, sizeof(link_path),
	                           "/proc/%d/exe", (int)process);
	if (link_length < 0 || (size_t)link_length >= sizeof(link_path))
		return -1;

	ssize_t result = readlink(link_path, buffer, buffer_size - 1);
	if (result < 0)
		return -1;
	buffer[result] = '\0';
	return 0;
}

int _NSGetExecutablePath(char* buffer, uint32_t* buffer_size)
{
	if (!buffer || !buffer_size)
		return -1;

	char host_path[4096];
	const char* path = guest_executable_path();
	if (!path) {
		if (read_process_executable(getpid(), host_path, sizeof(host_path)) != 0)
			return -1;
		path = host_path;
	}

	size_t required_size = strlen(path) + 1;
	if (required_size > *buffer_size) {
		*buffer_size = (uint32_t)required_size;
		return -1;
	}

	memcpy(buffer, path, required_size);
	*buffer_size = (uint32_t)(required_size - 1);
	return 0;
}

int proc_pidpath(int process, void* buffer, uint32_t buffer_size)
{
	if (!buffer || !buffer_size) {
		errno = EINVAL;
		return 0;
	}

	char host_path[4096];
	const char* path = NULL;
	if (process == getpid())
		path = guest_executable_path();
	if (!path) {
		if (read_process_executable(process, host_path, sizeof(host_path)) != 0)
			return 0;
		path = host_path;
	}

	size_t required_size = strlen(path) + 1;
	if (required_size > buffer_size) {
		errno = ENOBUFS;
		return 0;
	}
	memcpy(buffer, path, required_size);
	return (int)(required_size - 1);
}

const char* getprogname(void)
{
	const char* path = guest_executable_path();
	if (!path)
		return "machgate";

	const char* separator = strrchr(path, '/');
	return separator ? separator + 1 : path;
}
