#define _GNU_SOURCE

#include "file_abi.h"

#include <fcntl.h>
#include <stddef.h>

enum darwin_at_value {
	DARWIN_AT_CURRENT_WORKING_DIRECTORY = -2,
	DARWIN_AT_CHECK_EFFECTIVE_ACCESS = 0x0010,
	DARWIN_AT_DO_NOT_FOLLOW_SYMLINK = 0x0020,
	DARWIN_AT_FOLLOW_SYMLINK = 0x0040,
	DARWIN_AT_REMOVE_DIRECTORY = 0x0080,
};

enum darwin_open_value {
	DARWIN_OPEN_NONBLOCKING = 0x00000004,
	DARWIN_OPEN_APPEND = 0x00000008,
	DARWIN_OPEN_ASYNCHRONOUS = 0x00000040,
	DARWIN_OPEN_NO_FOLLOW = 0x00000100,
	DARWIN_OPEN_CREATE = 0x00000200,
	DARWIN_OPEN_TRUNCATE = 0x00000400,
	DARWIN_OPEN_EXCLUSIVE = 0x00000800,
	DARWIN_OPEN_NO_CONTROLLING_TERMINAL = 0x00020000,
	DARWIN_OPEN_DIRECTORY = 0x00100000,
	DARWIN_OPEN_DATA_SYNCHRONIZED = 0x00400000,
	DARWIN_OPEN_CLOSE_ON_EXEC = 0x01000000,
};

struct open_flag_translation {
	int guest;
	int host;
};

static const struct open_flag_translation open_flag_translations[] = {
	{ DARWIN_OPEN_NONBLOCKING, O_NONBLOCK },
	{ DARWIN_OPEN_APPEND, O_APPEND },
	{ DARWIN_OPEN_ASYNCHRONOUS, FASYNC },
	{ DARWIN_OPEN_NO_FOLLOW, O_NOFOLLOW },
	{ DARWIN_OPEN_CREATE, O_CREAT },
	{ DARWIN_OPEN_TRUNCATE, O_TRUNC },
	{ DARWIN_OPEN_EXCLUSIVE, O_EXCL },
	{ DARWIN_OPEN_NO_CONTROLLING_TERMINAL, O_NOCTTY },
	{ DARWIN_OPEN_DIRECTORY, O_DIRECTORY },
	{ DARWIN_OPEN_DATA_SYNCHRONIZED, O_DSYNC },
	{ DARWIN_OPEN_CLOSE_ON_EXEC, O_CLOEXEC },
};

int machgate_linux_open_flags(int darwin_flags)
{
	int result = darwin_flags & O_ACCMODE;

	for (size_t index = 0;
	     index < sizeof(open_flag_translations) /
	             sizeof(open_flag_translations[0]);
	     index++) {
		if (darwin_flags & open_flag_translations[index].guest)
			result |= open_flag_translations[index].host;
	}
	return result;
}

int machgate_darwin_open_flags(int linux_flags)
{
	int result = linux_flags & O_ACCMODE;

	for (size_t index = 0;
	     index < sizeof(open_flag_translations) /
	             sizeof(open_flag_translations[0]);
	     index++) {
		if (linux_flags & open_flag_translations[index].host)
			result |= open_flag_translations[index].guest;
	}
	return result;
}

int machgate_darwin_open_uses_mode(int darwin_flags)
{
	return (darwin_flags & DARWIN_OPEN_CREATE) != 0;
}

int machgate_linux_dirfd(int darwin_dirfd)
{
	if (darwin_dirfd == DARWIN_AT_CURRENT_WORKING_DIRECTORY)
		return AT_FDCWD;
	return darwin_dirfd;
}

int machgate_linux_at_flags(int darwin_flags)
{
	int result = 0;

	if (darwin_flags & DARWIN_AT_CHECK_EFFECTIVE_ACCESS)
		result |= AT_EACCESS;
	if (darwin_flags & DARWIN_AT_DO_NOT_FOLLOW_SYMLINK)
		result |= AT_SYMLINK_NOFOLLOW;
	if (darwin_flags & DARWIN_AT_FOLLOW_SYMLINK)
		result |= AT_SYMLINK_FOLLOW;
	if (darwin_flags & DARWIN_AT_REMOVE_DIRECTORY)
		result |= AT_REMOVEDIR;
	return result;
}
