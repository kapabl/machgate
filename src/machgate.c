#include "guest_allocator_bridge.h"
#include "loader.h"
#include "macho_defs.h"
#include "native_imports.h"
#include "syscall/syscall_gate.h"
#include "trampoline.h"
#include "vm_interpose.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MACHGATE_GUEST_STACK_SIZE (8u * 1024u * 1024u)

int machgate_verbose;
int __machgate_guest_argc;
char** __machgate_guest_argv;
char** __machgate_guest_envp;
const char* __machgate_guest_executable_path;
void* __machgate_main_stack_top;
size_t __machgate_main_stack_size;

_Noreturn void machgate_enter_guest(uintptr_t entry_point,
				    uintptr_t stack_pointer,
				    int argument_count,
				    char** arguments,
				    char** environment);
void machgate_call_guest_initializer(void* function, int argument_count,
				     char** arguments, char** environment,
				     char** apple, void* stack_pointer);

static void run_static_initializers(struct load_results* loaded,
				    int argument_count, char** arguments,
				    char** environment,
				    uintptr_t stack_pointer)
{
	struct mach_header_64* header = loaded->mh;
	uint8_t* cursor = (uint8_t*)(header + 1);

	for (uint32_t command_index = 0;
	     command_index < header->ncmds;
	     command_index++) {
		struct load_command* command = (void*)cursor;
		if (command->cmd == LC_SEGMENT_64) {
			struct segment_command_64* segment = (void*)command;
			struct section_64* sections = (void*)(segment + 1);
			for (uint32_t section_index = 0;
			     section_index < segment->nsects;
			     section_index++) {
				struct section_64* section = &sections[section_index];
				if (strncmp(section->sectname, "__mod_init_func", 16) != 0)
					continue;
				size_t count = section->size / sizeof(uintptr_t);
				uintptr_t* functions =
					(void*)(uintptr_t)(section->addr + loaded->slide);
				for (size_t function_index = 0;
				     function_index < count;
				     function_index++) {
					if (!functions[function_index])
						continue;
					machgate_call_guest_initializer(
						(void*)(functions[function_index] + loaded->slide),
						argument_count, arguments, environment, NULL,
						(void*)stack_pointer);
				}
			}
		}
		cursor += command->cmdsize;
	}
}

static int enter_guest_directory(const char* executable_path)
{
	char* directory = strdup(executable_path);
	if (!directory)
		return -1;
	int result = 0;
	char* slash = strrchr(directory, '/');
	if (slash) {
		*slash = '\0';
		if (*directory)
			result = chdir(directory);
	}
	free(directory);
	return result;
}

static int allocate_guest_stack(uintptr_t* stack_pointer, char* error,
				size_t error_size)
{
	long page_size_result = sysconf(_SC_PAGESIZE);
	if (page_size_result <= 0) {
		snprintf(error, error_size, "could not determine host page size");
		return -1;
	}

	size_t page_size = (size_t)page_size_result;
	size_t mapping_size = MACHGATE_GUEST_STACK_SIZE + page_size;
	void* mapping = mmap(NULL, mapping_size, PROT_NONE,
	                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED) {
		snprintf(error, error_size, "could not allocate guest stack: %s",
		         strerror(errno));
		return -1;
	}

	void* stack_base = (char*)mapping + page_size;
	if (mprotect(stack_base, MACHGATE_GUEST_STACK_SIZE,
	             PROT_READ | PROT_WRITE) != 0) {
		snprintf(error, error_size, "could not enable guest stack: %s",
		         strerror(errno));
		munmap(mapping, mapping_size);
		return -1;
	}

	__machgate_main_stack_top =
		(char*)stack_base + MACHGATE_GUEST_STACK_SIZE;
	__machgate_main_stack_size = MACHGATE_GUEST_STACK_SIZE;
	*stack_pointer = (uintptr_t)__machgate_main_stack_top & ~(uintptr_t)15;
	return 0;
}

static void print_usage(const char* program)
{
	fprintf(stderr, "usage: %s [-v] <arm64-mach-o> [arguments...]\n",
	        program);
}

int main(int argument_count, char** arguments, char** environment)
{
	machgate_ensure_vm_interpose(arguments, environment);

	int guest_index = 1;
	if (guest_index < argument_count &&
	    strcmp(arguments[guest_index], "-v") == 0) {
		machgate_verbose = 1;
		guest_index++;
	}
	if (guest_index >= argument_count) {
		print_usage(arguments[0]);
		return 2;
	}

	char error[256];
	struct load_results loaded;
	if (machgate_load_executable(arguments[guest_index], &loaded, error,
	                             sizeof(error)) != 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}
	if (machgate_bind_native_imports(&loaded, error, sizeof(error)) < 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}
	if (machgate_apply_trampolines(&loaded, error, sizeof(error)) < 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}
	if (syscall_gate_patch(&loaded) < 0) {
		fprintf(stderr, "machgate: could not patch Darwin syscall sites\n");
		return 1;
	}

	uintptr_t guest_stack_pointer;
	if (allocate_guest_stack(&guest_stack_pointer, error, sizeof(error)) != 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}

	char* resolved_path = realpath(arguments[guest_index], NULL);
	if (!resolved_path)
		resolved_path = strdup(arguments[guest_index]);
	if (!resolved_path) {
		fprintf(stderr, "machgate: could not save guest executable path\n");
		return 1;
	}

	__machgate_guest_argc = argument_count - guest_index;
	__machgate_guest_argv = &arguments[guest_index];
	__machgate_guest_envp = environment;
	__machgate_guest_executable_path = resolved_path;
	machgate_configure_guest_allocator_bridge(&loaded);
	run_static_initializers(&loaded, __machgate_guest_argc,
	                        __machgate_guest_argv, __machgate_guest_envp,
	                        guest_stack_pointer);
	if (enter_guest_directory(__machgate_guest_executable_path) != 0) {
		fprintf(stderr, "machgate: could not enter guest directory: %s\n",
		        strerror(errno));
		return 1;
	}

	machgate_enter_guest(loaded.entry_point, guest_stack_pointer,
	                     __machgate_guest_argc, __machgate_guest_argv,
	                     __machgate_guest_envp);
}
