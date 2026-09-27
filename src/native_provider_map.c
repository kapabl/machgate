#include "native_provider_map.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MACHGATE_PROVIDER_LINE_MAX 4096u

static char* trim_space(char* value)
{
	while (isspace((unsigned char)*value))
		value++;
	char* end = value + strlen(value);
	while (end > value && isspace((unsigned char)end[-1]))
		end--;
	*end = '\0';
	return value;
}

static int install_name_key(const char* install_name, char* key,
			    size_t key_size)
{
	const char* base = strrchr(install_name, '/');
	base = base ? base + 1 : install_name;
	size_t length = strlen(base);
	if (length > 6 && strcmp(base + length - 6, ".dylib") == 0)
		length -= 6;
	if (!length || length >= key_size)
		return -1;
	memcpy(key, base, length);
	key[length] = '\0';
	return 0;
}

static int copy_provider_path(const char* value, char* path,
			      size_t path_size, char* error,
			      size_t error_size)
{
	if (strcmp(value, "SKIP") == 0 || strcmp(value, "STUB") == 0)
		return 0;
	int written = snprintf(path, path_size, "%s", value);
	if (written < 0 || (size_t)written >= path_size) {
		snprintf(error, error_size, "native provider path is too long");
		return -1;
	}
	return 1;
}

static int find_provider_in_map(const char* map_path, const char* key,
				char* path, size_t path_size,
				char* error, size_t error_size)
{
	FILE* map = fopen(map_path, "r");
	if (!map) {
		snprintf(error, error_size, "could not open native provider map %s: %s",
		         map_path, strerror(errno));
		return -1;
	}

	char line[MACHGATE_PROVIDER_LINE_MAX];
	int result = 0;
	while (fgets(line, sizeof(line), map)) {
		if (!strchr(line, '\n') && !feof(map)) {
			snprintf(error, error_size,
			         "native provider map contains an overlong line");
			result = -1;
			break;
		}
		line[strcspn(line, "\r\n")] = '\0';
		char* entry = trim_space(line);
		if (!*entry || *entry == '#')
			continue;
		char* equals = strchr(entry, '=');
		if (!equals)
			continue;
		*equals = '\0';
		char* entry_key = trim_space(entry);
		char* entry_path = trim_space(equals + 1);
		if (strcmp(entry_key, key) != 0)
			continue;
		result = copy_provider_path(entry_path, path, path_size,
		                            error, error_size);
		break;
	}
	if (ferror(map) && result == 0) {
		snprintf(error, error_size, "could not read native provider map %s",
		         map_path);
		result = -1;
	}
	fclose(map);
	return result;
}

int machgate_native_provider_path(const char* install_name, char* path,
				  size_t path_size, char* error,
				  size_t error_size)
{
	if (strstr(install_name, "libSystem")) {
		const char* shim = getenv("MACHGATE_SYSTEM_SHIM_LIB");
		if (shim && *shim)
			return copy_provider_path(shim, path, path_size, error,
			                          error_size);
	}

	const char* map_path = getenv("MACHGATE_DYLIB_MAP");
	if (!map_path || !*map_path)
		return 0;
	char key[256];
	if (install_name_key(install_name, key, sizeof(key)) != 0) {
		snprintf(error, error_size, "native dependency name is too long");
		return -1;
	}
	return find_provider_in_map(map_path, key, path, path_size, error,
	                            error_size);
}
