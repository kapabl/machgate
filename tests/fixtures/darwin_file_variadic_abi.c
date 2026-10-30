extern int close(int descriptor);
extern int fcntl(int descriptor, int command, ...);
extern int ioctl(int descriptor, unsigned long request, ...);
extern int pipe(int descriptors[2]);

enum {
	DARWIN_F_GETFD = 1,
	DARWIN_F_SETFD = 2,
	DARWIN_F_GETFL = 3,
	DARWIN_FD_CLOEXEC = 1,
	DARWIN_O_NONBLOCK = 0x00000004,
};

#define DARWIN_FIONBIO 0x8004667eul

static void exit_guest(int status)
{
	register unsigned long result __asm__("x0") = (unsigned long)status;
	register unsigned long number __asm__("x16") = 1;
	__asm__ volatile("svc #0x80" : : "r"(result), "r"(number) : "memory");
	__builtin_unreachable();
}

int main(void)
{
	int descriptors[2];
	if (pipe(descriptors) != 0)
		exit_guest(81);

	int enabled = 1;
	if (ioctl(descriptors[0], DARWIN_FIONBIO, &enabled) != 0)
		exit_guest(82);
	if ((fcntl(descriptors[0], DARWIN_F_GETFL) & DARWIN_O_NONBLOCK) == 0)
		exit_guest(83);
	if (fcntl(descriptors[0], DARWIN_F_SETFD, DARWIN_FD_CLOEXEC) != 0)
		exit_guest(84);
	if ((fcntl(descriptors[0], DARWIN_F_GETFD) & DARWIN_FD_CLOEXEC) == 0)
		exit_guest(85);

	if (close(descriptors[0]) != 0 || close(descriptors[1]) != 0)
		exit_guest(86);
	exit_guest(0);
}
