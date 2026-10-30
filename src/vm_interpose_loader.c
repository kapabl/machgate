#include "vm_interpose.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MACHGATE_VM_INTERPOSE_PATH_MAX 4096
#define MACHGATE_VM_INTERPOSE_ENV_MAX 8192

static int env_name_matches(const char* env_value, const char* name)
{
	size_t name_length = strlen(name);
	return strncmp(env_value, name, name_length) == 0 &&
	       env_value[name_length] == '=';
}

static int vm_interpose_preloaded(const char* ld_preload)
{
	return ld_preload && strstr(ld_preload, MACHGATE_VM_INTERPOSE_BASENAME);
}

static int read_loader_path(char* loader_path, size_t loader_path_size)
{
	ssize_t length;

	errno = 0;
	length = readlink("/proc/self/exe", loader_path, loader_path_size - 1);
	if (length < 0)
		return -1;
	if ((size_t)length >= loader_path_size - 1) {
		errno = ENAMETOOLONG;
		return -1;
	}

	loader_path[length] = '\0';
	return 0;
}

static int format_sibling_path(char* path, size_t path_size,
                               const char* loader_path,
                               const char* relative_name)
{
	char* slash;
	int written;

	slash = strrchr(loader_path, '/');
	if (!slash)
		written = snprintf(path, path_size, "%s", relative_name);
	else {
		size_t dir_length = (size_t)(slash - loader_path) + 1;
		written = snprintf(path, path_size, "%.*s%s",
		                   (int)dir_length, loader_path, relative_name);
	}
	if (written < 0 || (size_t)written >= path_size) {
		errno = ENAMETOOLONG;
		return -1;
	}
	return 0;
}

static int build_interpose_path(char* interpose_path, size_t interpose_path_size)
{
	char loader_path[MACHGATE_VM_INTERPOSE_PATH_MAX];

	if (read_loader_path(loader_path, sizeof(loader_path)) != 0)
		return -1;

	if (format_sibling_path(interpose_path, interpose_path_size, loader_path,
	                        MACHGATE_VM_INTERPOSE_BASENAME) == 0 &&
	    access(interpose_path, R_OK) == 0)
		return 0;

	if (format_sibling_path(interpose_path, interpose_path_size, loader_path,
	                        "../lib/" MACHGATE_VM_INTERPOSE_BASENAME) == 0 &&
	    access(interpose_path, R_OK) == 0)
		return 0;

	errno = ENOENT;
	return -1;
}

static char* build_ld_preload_value(const char* interpose_path,
                                    const char* existing_ld_preload,
                                    char* buffer, size_t buffer_size)
{
	int written;

	if (existing_ld_preload && *existing_ld_preload) {
		written = snprintf(buffer, buffer_size, "%s:%s",
		                   interpose_path, existing_ld_preload);
	} else {
		written = snprintf(buffer, buffer_size, "%s", interpose_path);
	}
	if (written < 0 || (size_t)written >= buffer_size) {
		errno = ENAMETOOLONG;
		return NULL;
	}
	return buffer;
}

static char** duplicate_env_with_ld_preload(char** envp,
                                            const char* ld_preload_value)
{
	size_t env_count = 0;
	size_t new_index = 0;
	char** new_envp;
	char* ld_preload_entry;

	if (envp) {
		while (envp[env_count])
			env_count++;
	}

	ld_preload_entry = malloc(strlen("LD_PRELOAD=") +
	                          strlen(ld_preload_value) + 1);
	if (!ld_preload_entry)
		return NULL;
	sprintf(ld_preload_entry, "LD_PRELOAD=%s", ld_preload_value);

	new_envp = calloc(env_count + 2, sizeof(char*));
	if (!new_envp) {
		free(ld_preload_entry);
		return NULL;
	}

	if (envp) {
		for (size_t index = 0; index < env_count; index++) {
			if (env_name_matches(envp[index], "LD_PRELOAD"))
				continue;
			new_envp[new_index++] = envp[index];
		}
	}
	new_envp[new_index++] = ld_preload_entry;
	new_envp[new_index] = NULL;
	return new_envp;
}

static void free_env_with_ld_preload(char** envp)
{
	if (!envp)
		return;
	for (size_t index = 0; envp[index]; index++) {
		if (env_name_matches(envp[index], "LD_PRELOAD")) {
			free(envp[index]);
			break;
		}
	}
	free(envp);
}

int machgate_ensure_vm_interpose(char** argv, char** envp)
{
	char interpose_path[MACHGATE_VM_INTERPOSE_PATH_MAX];
	char ld_preload_value[MACHGATE_VM_INTERPOSE_ENV_MAX];
	char loader_path[MACHGATE_VM_INTERPOSE_PATH_MAX];
	const char* existing_ld_preload;
	char** new_envp;

	existing_ld_preload = getenv("LD_PRELOAD");
	if (vm_interpose_preloaded(existing_ld_preload))
		return 0;

	if (build_interpose_path(interpose_path, sizeof(interpose_path)) != 0)
		return 0;
	if (!build_ld_preload_value(interpose_path, existing_ld_preload,
	                            ld_preload_value, sizeof(ld_preload_value)))
		return 0;
	if (read_loader_path(loader_path, sizeof(loader_path)) != 0)
		return 0;

	new_envp = duplicate_env_with_ld_preload(envp, ld_preload_value);
	if (!new_envp)
		return 0;

	execve(loader_path, argv, new_envp);
	free_env_with_ld_preload(new_envp);
	return 0;
}
