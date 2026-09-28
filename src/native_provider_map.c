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

static int copy_provider_path(const char* value, char* path, size_t path_size,
			      char* error, size_t error_size)
{
	int written = snprintf(path, path_size, "%s", value);
	if (written < 0 || (size_t)written >= path_size) {
		snprintf(error, error_size, "native provider path is too long");
		return -1;
	}
	return 0;
}

static int parse_provider_decision(
	const char* value, enum machgate_provider_decision_kind* kind,
	char* path, size_t path_size, char* error, size_t error_size)
{
	path[0] = '\0';
	if (strcmp(value, "UNMAPPED") == 0) {
		*kind = MACHGATE_PROVIDER_UNMAPPED;
		return 0;
	}
	if (strcmp(value, "SKIP") == 0) {
		*kind = MACHGATE_PROVIDER_SKIP;
		return 0;
	}
	if (strcmp(value, "STUB") == 0) {
		*kind = MACHGATE_PROVIDER_STUB;
		return 0;
	}
	const char* provider_path = value;
	if (strncmp(value, "MAP:", 4) == 0) {
		provider_path = value + 4;
		*kind = MACHGATE_PROVIDER_MAP;
	} else if (strncmp(value, "PARTIAL:", 8) == 0) {
		provider_path = value + 8;
		*kind = MACHGATE_PROVIDER_PARTIAL;
	} else {
		*kind = MACHGATE_PROVIDER_MAP;
	}
	while (isspace((unsigned char)*provider_path))
		provider_path++;
	if (!*provider_path) {
		snprintf(error, error_size, "native provider path is empty");
		return -1;
	}
	return copy_provider_path(provider_path, path, path_size, error,
	                          error_size);
}

static int find_provider_in_map(const char* map_path, const char* key,
				enum machgate_provider_decision_kind* kind,
				char* path, size_t path_size, char* error,
				size_t error_size)
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
		if (parse_provider_decision(entry_path, kind, path, path_size,
		                            error, error_size) != 0) {
			result = -1;
			break;
		}
		result = 1;
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

static int remove_numeric_version(char* key)
{
	char* dot = strrchr(key, '.');
	if (!dot || !dot[1])
		return 0;
	for (char* cursor = dot + 1; *cursor; cursor++) {
		if (!isdigit((unsigned char)*cursor))
			return 0;
	}
	*dot = '\0';
	return 1;
}

int machgate_native_provider_decision(
	const char* install_name, enum machgate_provider_decision_kind* kind,
	char* path, size_t path_size, char* error, size_t error_size)
{
	*kind = MACHGATE_PROVIDER_UNMAPPED;
	path[0] = '\0';
	if (strstr(install_name, "libSystem")) {
		const char* shim = getenv("MACHGATE_SYSTEM_SHIM_LIB");
		if (shim && *shim)
			return parse_provider_decision(shim, kind, path, path_size,
			                               error, error_size);
	}

	const char* map_path = getenv("MACHGATE_DYLIB_MAP");
	if (!map_path || !*map_path)
		return 0;
	char key[256];
	if (install_name_key(install_name, key, sizeof(key)) != 0) {
		snprintf(error, error_size, "native dependency name is too long");
		return -1;
	}
	int result = find_provider_in_map(map_path, key, kind, path, path_size,
	                                  error, error_size);
	if (result < 0)
		return -1;
	if (result > 0)
		return 0;

	while (remove_numeric_version(key)) {
		result = find_provider_in_map(map_path, key, kind, path, path_size,
		                              error, error_size);
		if (result < 0)
			return -1;
		if (result > 0)
			return 0;
	}
	return 0;
}
