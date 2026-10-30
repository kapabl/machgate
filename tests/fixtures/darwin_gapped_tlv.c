static _Thread_local unsigned char initialized_storage = 0x2a;
static _Thread_local unsigned char zero_fill_storage __attribute__((aligned(4096)));

int main(void)
{
	unsigned long valid = initialized_storage == 0x2a &&
	                      zero_fill_storage == 0;
	zero_fill_storage = 0x53;
	valid = valid && initialized_storage == 0x2a &&
	        zero_fill_storage == 0x53;
	register unsigned long status __asm__("x0") = valid ? 0 : 52;
	register unsigned long syscall_number __asm__("x16") = 1;
	__asm__ volatile("svc #0x80"
	                 :
	                 : "r"(status), "r"(syscall_number)
	                 : "memory");
	__builtin_unreachable();
}
