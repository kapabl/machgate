typedef unsigned long guest_address;

struct _Unwind_Context;

extern int _Unwind_Backtrace(
	int (*callback)(struct _Unwind_Context*, void*), void* argument);
extern guest_address _Unwind_GetIP(struct _Unwind_Context* context);
extern int walk_frame_saved(int (*callback)(void));
extern char walk_frame_saved_end;
extern int walk_frameless_saved(void);

struct unwind_probe {
	int found_saved_frame;
	int found_caller_after_saved_frame;
};

static int record_guest_frame(struct _Unwind_Context* context, void* argument)
{
	struct unwind_probe* probe = argument;
	guest_address instruction_pointer = _Unwind_GetIP(context);
	guest_address frame_start = (guest_address)walk_frame_saved;
	guest_address frame_end = (guest_address)&walk_frame_saved_end;
	if (instruction_pointer >= frame_start && instruction_pointer < frame_end) {
		probe->found_saved_frame = 1;
	} else if (probe->found_saved_frame) {
		probe->found_caller_after_saved_frame = 1;
	}
	return 0;
}

__attribute__((noinline)) static int trace_saved_frame(void)
{
	struct unwind_probe probe = {0};
	_Unwind_Backtrace(record_guest_frame, &probe);
	return probe.found_saved_frame &&
	       probe.found_caller_after_saved_frame;
}

int main(void)
{
	int frame_result = walk_frame_saved(trace_saved_frame);
	int frameless_result = walk_frameless_saved();
	register unsigned long status __asm__("x0") =
		(frame_result && frameless_result == 42) ? 0 : 45;
	register unsigned long syscall_number __asm__("x16") = 1;
	__asm__ volatile("svc #0x80"
	                 :
	                 : "r"(status), "r"(syscall_number)
	                 : "memory");
	__builtin_unreachable();
}
