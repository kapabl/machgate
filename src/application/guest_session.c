#include "application/guest_session.h"
#include "application/guest_unwind.h"
#include "application/process_launch_context.h"

#include "guest_allocator_bridge.h"
#include "loader.h"
#include "macho_defs.h"
#include "native_imports.h"
#include "syscall/syscall_gate.h"
#include "trampoline.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MACHGATE_GUEST_STACK_SIZE (8u * 1024u * 1024u)

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
				    char** environment,
				    char** apple);
void machgate_call_guest_initializer(void* function, int argument_count,
				     char** arguments, char** environment,
				     char** apple, void* stack_pointer);

typedef int (*configure_tlv_image_function)(void*, size_t, size_t);

static int initializer_range_valid(const struct machgate_guest_image* image,
				   uintptr_t address, uint64_t size)
{
	return address >= image->mapping_start && address <= image->mapping_end &&
	       size <= image->mapping_end - address;
}

static int apply_image_slide(uint64_t address, intptr_t slide,
			     uintptr_t* result)
{
	if (slide >= 0) {
		uintptr_t positive_slide = (uintptr_t)slide;
		if (address > UINTPTR_MAX - positive_slide)
			return -1;
		*result = (uintptr_t)address + positive_slide;
		return 0;
	}

	uintptr_t negative_slide = (uintptr_t)(-(slide + 1)) + 1;
	if (address < negative_slide)
		return -1;
	*result = (uintptr_t)address - negative_slide;
	return 0;
}

static void call_guest_initializer(void* function, int argument_count,
				   char** arguments, char** environment,
				   char** apple, uintptr_t stack_pointer)
{
	machgate_call_guest_initializer(function, argument_count, arguments,
	                                environment, apple,
	                                (void*)stack_pointer);
}

static int run_pointer_initializers(struct machgate_guest_image* image,
				    const struct section_64* section,
				    int argument_count, char** arguments,
				    char** environment, char** apple,
				    uintptr_t stack_pointer, char* error,
				    size_t error_size)
{
	uintptr_t section_address = section->addr + image->slide;
	if (section->size % sizeof(uintptr_t) != 0 ||
	    !initializer_range_valid(image, section_address, section->size)) {
		snprintf(error, error_size,
		         "Mach-O has an invalid initializer-pointer section");
		return -1;
	}

	uintptr_t* functions = (void*)section_address;
	size_t count = section->size / sizeof(*functions);
	for (size_t function_index = 0; function_index < count;
	     function_index++) {
		if (!functions[function_index])
			continue;
		uintptr_t function = functions[function_index] + image->slide;
		if (!initializer_range_valid(image, function, 1)) {
			snprintf(error, error_size,
			         "Mach-O initializer address is outside the image");
			return -1;
		}
		call_guest_initializer((void*)function, argument_count, arguments,
		                       environment, apple, stack_pointer);
	}
	return 0;
}

static int run_offset_initializers(struct machgate_guest_image* image,
				   const struct section_64* section,
				   int argument_count, char** arguments,
				   char** environment, char** apple,
				   uintptr_t stack_pointer, char* error,
				   size_t error_size)
{
	uintptr_t section_address = section->addr + image->slide;
	if (section->size % sizeof(uint32_t) != 0 ||
	    !initializer_range_valid(image, section_address, section->size)) {
		snprintf(error, error_size,
		         "Mach-O has an invalid initializer-offset section");
		return -1;
	}

	uint32_t* offsets = (void*)section_address;
	size_t count = section->size / sizeof(*offsets);
	for (size_t offset_index = 0; offset_index < count; offset_index++) {
		uintptr_t function = (uintptr_t)image->header + offsets[offset_index];
		if (!initializer_range_valid(image, function, 1)) {
			snprintf(error, error_size,
			         "Mach-O initializer offset is outside the image");
			return -1;
		}
		call_guest_initializer((void*)function, argument_count, arguments,
		                       environment, apple, stack_pointer);
	}
	return 0;
}

static int run_static_initializers(struct machgate_guest_image* image,
				   int argument_count, char** arguments,
				   char** environment, char** apple,
				   uintptr_t stack_pointer, char* error,
				   size_t error_size)
{
	struct mach_header_64* header = image->header;
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
				uint32_t section_type = section->flags & SECTION_TYPE;
				if (section_type == S_MOD_INIT_FUNC_POINTERS &&
				    run_pointer_initializers(
					    image, section, argument_count, arguments,
					    environment, apple, stack_pointer, error,
					    error_size) != 0)
					return -1;
				if (section_type == S_INIT_FUNC_OFFSETS &&
				    run_offset_initializers(
					    image, section, argument_count, arguments,
					    environment, apple, stack_pointer, error,
					    error_size) != 0)
					return -1;
			}
		}
		cursor += command->cmdsize;
	}
	return 0;
}

static int configure_guest_tlv_image(struct machgate_guest_image* image,
				     char* error, size_t error_size)
{
	const struct section_64* initial_data = NULL;
	const struct section_64* zero_fill = NULL;
	struct mach_header_64* header = image->header;
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
				uint32_t section_type = section->flags & SECTION_TYPE;
				if (section_type == S_THREAD_LOCAL_REGULAR) {
					if (initial_data) {
						snprintf(error, error_size,
						         "Mach-O has multiple initialized TLV sections");
						return -1;
					}
					initial_data = section;
				}
				if (section_type == S_THREAD_LOCAL_ZEROFILL) {
					if (zero_fill) {
						snprintf(error, error_size,
						         "Mach-O has multiple zero-fill TLV sections");
						return -1;
					}
					zero_fill = section;
				}
			}
		}
		cursor += command->cmdsize;
	}

	if (!initial_data && !zero_fill)
		return 0;

	uintptr_t image_base;
	size_t image_size;
	size_t bss_size = 0;
	if (initial_data) {
		if (apply_image_slide(initial_data->addr, image->slide,
		                      &image_base) != 0 ||
		    initial_data->size > SIZE_MAX) {
			snprintf(error, error_size,
			         "Mach-O initialized TLV section overflows host address limits");
			return -1;
		}
		image_size = initial_data->size;
		if (!initializer_range_valid(image, image_base, image_size)) {
			snprintf(error, error_size,
			         "Mach-O initialized TLV section is outside the image");
			return -1;
		}
		if (zero_fill) {
			uintptr_t zero_fill_address;
			if (apply_image_slide(zero_fill->addr, image->slide,
			                      &zero_fill_address) != 0 ||
			    zero_fill->size > SIZE_MAX) {
				snprintf(error, error_size,
				         "Mach-O zero-fill TLV section overflows host address limits");
				return -1;
			}
			if (image_size > UINTPTR_MAX - image_base) {
				snprintf(error, error_size,
				         "Mach-O initialized TLV section overflows its address range");
				return -1;
			}
			uintptr_t initialized_end = image_base + image_size;
			if (zero_fill_address < initialized_end ||
			    !initializer_range_valid(image, zero_fill_address,
			                             zero_fill->size)) {
				snprintf(error, error_size,
				         "Mach-O TLV sections have an invalid layout");
				return -1;
			}
			size_t alignment_padding = zero_fill_address - initialized_end;
			if (zero_fill->size > SIZE_MAX - alignment_padding) {
				snprintf(error, error_size,
				         "Mach-O TLV image size overflows host size limits");
				return -1;
			}
			bss_size = alignment_padding + zero_fill->size;
		}
	} else {
		if (apply_image_slide(zero_fill->addr, image->slide,
		                      &image_base) != 0 ||
		    zero_fill->size > SIZE_MAX) {
			snprintf(error, error_size,
			         "Mach-O zero-fill TLV section overflows host address limits");
			return -1;
		}
		image_size = 0;
		bss_size = zero_fill->size;
		if (!initializer_range_valid(image, image_base, bss_size)) {
			snprintf(error, error_size,
			         "Mach-O zero-fill TLV section is outside the image");
			return -1;
		}
	}

	configure_tlv_image_function configure =
		(configure_tlv_image_function)dlsym(RTLD_DEFAULT,
		                                     "machgate_configure_tlv_image");
	if (!configure) {
		snprintf(error, error_size,
		         "native provider does not support the Mach-O TLV image");
		return -1;
	}
	if (configure((void*)image_base, image_size, bss_size) != 0) {
		snprintf(error, error_size,
		         "native provider rejected the Mach-O TLV image");
		return -1;
	}
	return 0;
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

int machgate_run_guest(const char* executable_path, int argument_count,
		       char** arguments, char** environment)
{
	char error[256];
	struct machgate_guest_image image;
	if (machgate_load_executable(executable_path, &image, error,
	                             sizeof(error)) != 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}
	if (machgate_bind_native_imports(&image, error, sizeof(error)) < 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}
	if (machgate_register_guest_unwind(&image, error, sizeof(error)) != 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}
	if (machgate_apply_trampolines(&image, error, sizeof(error)) < 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}
	if (syscall_gate_patch(&image) < 0) {
		fprintf(stderr, "machgate: could not patch Darwin syscall sites\n");
		return 1;
	}
	if (configure_guest_tlv_image(&image, error, sizeof(error)) != 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}

	uintptr_t guest_stack_pointer;
	if (allocate_guest_stack(&guest_stack_pointer, error, sizeof(error)) != 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}

	char* resolved_path = realpath(executable_path, NULL);
	if (!resolved_path)
		resolved_path = strdup(executable_path);
	if (!resolved_path) {
		fprintf(stderr, "machgate: could not save guest executable path\n");
		return 1;
	}

	__machgate_guest_argc = argument_count;
	__machgate_guest_executable_path = resolved_path;
	struct machgate_process_launch_context launch_context;
	if (machgate_prepare_process_launch_context(
		guest_stack_pointer, MACHGATE_GUEST_STACK_SIZE, argument_count,
		arguments, environment, resolved_path, &launch_context,
		error, sizeof(error)) != 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}
	__machgate_guest_argv = launch_context.arguments;
	__machgate_guest_envp = launch_context.environment;
	machgate_configure_guest_allocator_bridge(&image);
	if (run_static_initializers(
		    &image, argument_count, launch_context.arguments,
		    launch_context.environment, launch_context.apple,
		    launch_context.stack_pointer, error, sizeof(error)) != 0) {
		fprintf(stderr, "machgate: %s\n", error);
		return 1;
	}
	if (enter_guest_directory(resolved_path) != 0) {
		fprintf(stderr, "machgate: could not enter guest directory: %s\n",
		        strerror(errno));
		return 1;
	}

	machgate_enter_guest(image.entry_point, launch_context.stack_pointer,
	                     argument_count, launch_context.arguments,
	                     launch_context.environment, launch_context.apple);
}
