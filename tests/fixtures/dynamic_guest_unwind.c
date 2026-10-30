typedef unsigned long guest_address;

struct _Unwind_Context;

extern int _Unwind_Backtrace(
	int (*callback)(struct _Unwind_Context*, void*), void* argument);
extern guest_address _Unwind_GetIP(struct _Unwind_Context* context);

struct unwind_probe {
	guest_address function_start;
	int found_guest_frame;
};

int main(void);

static int record_guest_frame(struct _Unwind_Context* context, void* argument)
{
	struct unwind_probe* probe = argument;
	guest_address instruction_pointer = _Unwind_GetIP(context);
	if (instruction_pointer >= probe->function_start &&
	    instruction_pointer < probe->function_start + 256)
		probe->found_guest_frame = 1;
	return 0;
}

__attribute__((noinline)) static int walk_guest_frames(void)
{
	struct unwind_probe probe = {
		.function_start = (guest_address)main,
		.found_guest_frame = 0,
	};
	_Unwind_Backtrace(record_guest_frame, &probe);
	return probe.found_guest_frame;
}

int main(void)
{
	register unsigned long status __asm__("x0") =
		walk_guest_frames() ? 0 : 44;
	register unsigned long syscall_number __asm__("x16") = 1;
	__asm__ volatile("svc #0x80"
	                 :
	                 : "r"(status), "r"(syscall_number)
	                 : "memory");
	__builtin_unreachable();
}
