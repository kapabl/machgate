#include "application/process_launch_context.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MACHGATE_APPLE_EXECUTABLE_PATH_PREFIX "executable_path="

static size_t pointer_count(char** vector)
{
	size_t result = 0;
	while (vector && vector[result])
		result++;
	return result;
}

static uintptr_t align_down(uintptr_t value, size_t alignment)
{
	return value & ~((uintptr_t)alignment - 1);
}

int machgate_prepare_process_launch_context(
	uintptr_t stack_top, size_t stack_size, int argument_count,
	char** arguments, char** environment, const char* executable_path,
	struct machgate_process_launch_context* context,
	char* error, size_t error_size)
{
	if (argument_count < 0 || !arguments || !executable_path || !context) {
		snprintf(error, error_size, "invalid guest process launch context");
		return -1;
	}
	if (stack_size > stack_top) {
		snprintf(error, error_size, "invalid guest stack range");
		return -1;
	}

	uintptr_t stack_bottom = stack_top - stack_size;
	size_t environment_count = pointer_count(environment);
	size_t prefix_size = sizeof(MACHGATE_APPLE_EXECUTABLE_PATH_PREFIX) - 1;
	size_t path_size = strlen(executable_path);
	if (path_size > SIZE_MAX - prefix_size - 1) {
		snprintf(error, error_size, "guest executable path is too long");
		return -1;
	}

	size_t apple_string_size = prefix_size + path_size + 1;
	if (apple_string_size > stack_size) {
		snprintf(error, error_size, "guest launch context exceeds stack");
		return -1;
	}
	uintptr_t cursor = stack_top - apple_string_size;
	char* apple_executable_path = (void*)cursor;
	memcpy(apple_executable_path, MACHGATE_APPLE_EXECUTABLE_PATH_PREFIX,
	       prefix_size);
	memcpy(apple_executable_path + prefix_size, executable_path,
	       path_size + 1);

	if ((size_t)argument_count >
	    SIZE_MAX - environment_count - 5) {
		snprintf(error, error_size, "guest launch vector is too large");
		return -1;
	}
	size_t word_count = (size_t)argument_count + environment_count + 5;
	if (word_count > SIZE_MAX / sizeof(uintptr_t)) {
		snprintf(error, error_size, "guest launch vector is too large");
		return -1;
	}
	size_t vector_size = word_count * sizeof(uintptr_t);
	cursor = align_down(cursor, 16);
	if (cursor < stack_bottom || vector_size > cursor - stack_bottom) {
		snprintf(error, error_size, "guest launch context exceeds stack");
		return -1;
	}
	cursor = align_down(cursor - vector_size, 16);
	if (cursor < stack_bottom) {
		snprintf(error, error_size, "guest launch context exceeds stack");
		return -1;
	}

	uintptr_t* words = (void*)cursor;
	size_t word_index = 0;
	words[word_index++] = (uintptr_t)argument_count;
	context->arguments = (void*)&words[word_index];
	for (int argument_index = 0;
	     argument_index < argument_count;
	     argument_index++)
		words[word_index++] = (uintptr_t)arguments[argument_index];
	words[word_index++] = 0;

	context->environment = (void*)&words[word_index];
	for (size_t environment_index = 0;
	     environment_index < environment_count;
	     environment_index++)
		words[word_index++] = (uintptr_t)environment[environment_index];
	words[word_index++] = 0;

	context->apple = (void*)&words[word_index];
	words[word_index++] = (uintptr_t)apple_executable_path;
	words[word_index] = 0;
	context->stack_pointer = cursor;
	return 0;
}
