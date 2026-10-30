#include <stdint.h>

enum {
	MACHGATE_KERN_INVALID_ARGUMENT = 4,
	MACHGATE_KERN_NOT_SUPPORTED = 46,
	MACHGATE_TASK_CATEGORY_POLICY = 1,
	MACHGATE_TASK_DEFAULT_APPLICATION = 7,
	MACHGATE_TASK_CATEGORY_POLICY_COUNT = 1,
};

extern uint32_t mach_task_self_;

int32_t task_policy_set(uint32_t target_task, uint32_t flavor,
	                const int32_t* policy_info, uint32_t count)
{
	if (target_task != mach_task_self_ ||
	    flavor != MACHGATE_TASK_CATEGORY_POLICY || !policy_info ||
	    count != MACHGATE_TASK_CATEGORY_POLICY_COUNT ||
	    *policy_info != MACHGATE_TASK_DEFAULT_APPLICATION)
		return MACHGATE_KERN_INVALID_ARGUMENT;

	return MACHGATE_KERN_NOT_SUPPORTED;
}
