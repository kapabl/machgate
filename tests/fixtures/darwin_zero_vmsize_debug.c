static int debug_exit_status(void)
{
	return 42;
}

int main(void)
{
	register unsigned long status __asm__("x0") = debug_exit_status();
	register unsigned long syscall_number __asm__("x16") = 1;
	__asm__ volatile("svc #0x80"
	                 :
	                 : "r"(status), "r"(syscall_number)
	                 : "memory");
	__builtin_unreachable();
}
