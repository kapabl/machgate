typedef __builtin_va_list darwin_va_list;

extern void* stdout;
extern int fflush(void* stream);
extern int printf(const char* format, ...);
extern int snprintf(char* buffer, unsigned long buffer_size,
	            const char* format, ...);
extern int vfprintf(void* stream, const char* format, darwin_va_list arguments);
extern int vsnprintf(char* buffer, unsigned long buffer_size,
	             const char* format, darwin_va_list arguments);

static int print_with_darwin_list(const char* format, ...)
{
	darwin_va_list arguments;
	__builtin_va_start(arguments, format);
	int result = vfprintf(stdout, format, arguments);
	__builtin_va_end(arguments);
	return result;
}

static int format_with_darwin_list(char* buffer, unsigned long buffer_size,
	                           const char* format, ...)
{
	darwin_va_list arguments;
	__builtin_va_start(arguments, format);
	int result = vsnprintf(buffer, buffer_size, format, arguments);
	__builtin_va_end(arguments);
	return result;
}

static void exit_guest(int status)
{
	register unsigned long result __asm__("x0") = (unsigned long)status;
	register unsigned long number __asm__("x16") = 1;
	__asm__ volatile("svc #0x80" : : "r"(result), "r"(number) : "memory");
	__builtin_unreachable();
}

int main(void)
{
	char buffer[64];
	char direct_buffer[64];
	int printf_result = printf("printf:%d:%s:%.1f\n", 7, "clean", 3.5);
	int snprintf_result = snprintf(
		direct_buffer, sizeof(direct_buffer),
		"snprintf:%d:%s:%.1f", 8, "direct", 0.5);
	int vfprintf_result = print_with_darwin_list(
		"vfprintf:%lld:%s:%.2f\n", 9000000001LL, "room", 2.25);
	int vsnprintf_result = format_with_darwin_list(
		buffer, sizeof(buffer), "vsnprintf:%u:%s:%.1f", 42, "safe", 1.5);
	int buffer_print_result = printf("%s\n", buffer);
	int direct_print_result = printf("%s\n", direct_buffer);
	int flush_result = fflush(stdout);

	if (printf_result != 19 || snprintf_result != 21 ||
	    vfprintf_result != 30 || direct_print_result != 22 ||
	    vsnprintf_result != 21 || buffer_print_result != 22 ||
	    flush_result != 0)
		exit_guest(71);
	exit_guest(0);
}
