static _Thread_local unsigned char large_storage[8192];
static _Thread_local unsigned char allocation_trigger;

int main(void)
{
	large_storage[sizeof(large_storage) - 1] = 0x47;
	allocation_trigger = 0x4d;
	register unsigned long status __asm__("x0") =
		large_storage[sizeof(large_storage) - 1] == 0x47 &&
		allocation_trigger == 0x4d ? 0 : 51;
	register unsigned long syscall_number __asm__("x16") = 1;
	__asm__ volatile("svc #0x80"
	                 :
	                 : "r"(status), "r"(syscall_number)
	                 : "memory");
	__builtin_unreachable();
}
