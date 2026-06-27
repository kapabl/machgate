#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct darwin_sigaction_for_test {
	uint64_t handler;
	uint32_t mask;
	uint32_t flags;
};

static void unexpected_handler(int signum)
{
	(void)signum;
	_exit(80);
}

static uintptr_t trigger_tree_insert_fault(uintptr_t* tree, uintptr_t* node)
{
	uintptr_t result = 0;

#if defined(__aarch64__)
	__asm__ volatile(
		"mov x0, %[tree]\n"
		"add x1, %[tree], #8\n"
		"mov x2, x1\n"
		"mov x3, %[node]\n"
		"mov x8, xzr\n"
		".word 0xf9400108\n"
		"mov %[result], x8\n"
		: [result] "=r"(result)
		: [tree] "r"(tree), [node] "r"(node)
		: "x0", "x1", "x2", "x3", "x8", "memory");
#else
	(void)tree;
	(void)node;
#endif

	return result;
}

int main(void)
{
	typedef int (*darwin_sigaction_fn)(int, const void*, void*);
	darwin_sigaction_fn shim_sigaction =
		(darwin_sigaction_fn)dlsym(RTLD_DEFAULT, "sigaction");
	struct darwin_sigaction_for_test action = {
		.handler = (uint64_t)(uintptr_t)unexpected_handler,
		.mask = 0,
		.flags = 0,
	};
	uintptr_t tree[4] = {0};
	uintptr_t node[4] = {0};
	uintptr_t result;

	if (!shim_sigaction) {
		fprintf(stderr, "sigaction symbol not found\n");
		return 2;
	}
	setenv("MACHGATE_TRACE_SIGNALS", "1", 1);
	if (shim_sigaction(11, &action, NULL) != 0) {
		perror("shim sigaction");
		return 3;
	}

	tree[1] = (uintptr_t)node;
	result = trigger_tree_insert_fault(tree, node);

	if (result != (uintptr_t)node) {
		fprintf(stderr, "result=%p expected=%p\n",
		        (void*)result, (void*)node);
		return 4;
	}
	if (tree[0] != (uintptr_t)&tree[1]) {
		fprintf(stderr, "tree[0]=%p expected=%p\n",
		        (void*)tree[0], (void*)&tree[1]);
		return 5;
	}

	return 0;
}
