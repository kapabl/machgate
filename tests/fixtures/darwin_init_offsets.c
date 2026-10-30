static volatile unsigned long initialized_value;

__attribute__((constructor)) static void initialize_value(void)
{
	initialized_value = 0x4d47494eUL;
}

int main(void)
{
	register unsigned long status __asm__("x0") =
		initialized_value == 0x4d47494eUL ? 0 : 47;
	register unsigned long syscall_number __asm__("x16") = 1;
	__asm__ volatile("svc #0x80"
	                 :
	                 : "r"(status), "r"(syscall_number)
	                 : "memory");
	__builtin_unreachable();
}
