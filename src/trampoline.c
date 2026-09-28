#include "trampoline.h"

#include "arm64_enc.h"
#include "macho_defs.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void* address_for_file_offset(struct mach_header_64* header,
				     uint64_t file_offset,
				     uint64_t required_size)
{
	uint8_t* cursor = (uint8_t*)(header + 1);
	for (uint32_t index = 0; index < header->ncmds; index++) {
		struct load_command* command = (void*)cursor;
		if (command->cmd == LC_SEGMENT_64) {
			struct segment_command_64* segment = (void*)command;
			if (file_offset >= segment->fileoff &&
			    file_offset - segment->fileoff <= segment->filesize &&
			    required_size <= segment->filesize -
			                     (file_offset - segment->fileoff)) {
				return (void*)(uintptr_t)(segment->vmaddr +
				                         file_offset - segment->fileoff);
			}
		}
		cursor += command->cmdsize;
	}
	return NULL;
}

static int patch_function(struct machgate_guest_image* image, uintptr_t function,
			  uintptr_t replacement, char* error,
			  size_t error_size)
{
	long page_size_result = sysconf(_SC_PAGESIZE);
	if (page_size_result <= 0) {
		snprintf(error, error_size, "could not determine host page size");
		return -1;
	}
	size_t page_size = (size_t)page_size_result;
	uint32_t* island = machgate_pool_alloc(image, page_size);
	if (!island) {
		snprintf(error, error_size,
		         "could not allocate a nearby trampoline island");
		return -1;
	}

	int64_t byte_distance = (char*)island - (char*)function;
	if (!arm64_branch_in_range(byte_distance)) {
		snprintf(error, error_size, "trampoline island is out of branch range");
		return -1;
	}

	island[0] = 0x58000050u;
	island[1] = 0xd61f0200u;
	memcpy(&island[2], &replacement, sizeof(replacement));
	__builtin___clear_cache((char*)island, (char*)island + 16);
	if (mprotect(island, page_size, PROT_READ | PROT_EXEC) != 0) {
		snprintf(error, error_size,
		         "could not protect trampoline island: %s", strerror(errno));
		return -1;
	}

	uintptr_t page = function & ~((uintptr_t)page_size - 1);
	if (mprotect((void*)page, page_size,
	             PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
		snprintf(error, error_size,
		         "could not make guest function writable: %s", strerror(errno));
		return -1;
	}
	*(uint32_t*)function = enc_b((int32_t)(byte_distance / 4));
	__builtin___clear_cache((char*)function, (char*)function + 4);
	if (mprotect((void*)page, page_size, PROT_READ | PROT_EXEC) != 0) {
		snprintf(error, error_size,
		         "could not restore guest function protection: %s",
		         strerror(errno));
		return -1;
	}
	return 0;
}

int machgate_apply_trampolines(struct machgate_guest_image* image, char* error,
			       size_t error_size)
{
	const char* library_path = getenv("MACHGATE_TRAMPOLINE_LIB");
	const char* symbol_prefix = getenv("MACHGATE_TRAMPOLINE_PREFIX");
	if (!library_path || !*library_path || !symbol_prefix || !*symbol_prefix)
		return 0;

	void* library = dlopen(library_path, RTLD_NOW | RTLD_LOCAL);
	if (!library) {
		snprintf(error, error_size, "could not open trampoline library: %s",
		         dlerror());
		return -1;
	}

	struct mach_header_64* header = image->header;
	struct symtab_command* symbol_table_command = NULL;
	uint8_t* cursor = (uint8_t*)(header + 1);
	for (uint32_t index = 0; index < header->ncmds; index++) {
		struct load_command* command = (void*)cursor;
		if (command->cmd == LC_SYMTAB)
			symbol_table_command = (void*)command;
		cursor += command->cmdsize;
	}
	if (!symbol_table_command) {
		snprintf(error, error_size, "Mach-O has no symbol table");
		return -1;
	}

	uint64_t symbol_bytes =
		(uint64_t)symbol_table_command->nsyms * sizeof(struct nlist_64);
	struct nlist_64* symbols = address_for_file_offset(
		header, symbol_table_command->symoff, symbol_bytes);
	char* strings = address_for_file_offset(
		header, symbol_table_command->stroff, symbol_table_command->strsize);
	if (!symbols || !strings) {
		snprintf(error, error_size, "Mach-O symbol table is not mapped");
		return -1;
	}

	size_t prefix_length = strlen(symbol_prefix);
	int patched = 0;
	for (uint32_t index = 0; index < symbol_table_command->nsyms; index++) {
		struct nlist_64* symbol = &symbols[index];
		if ((symbol->n_type & N_STAB) != 0 ||
		    (symbol->n_type & N_TYPE) != N_SECT || !symbol->n_value ||
		    symbol->n_strx >= symbol_table_command->strsize)
			continue;
		const char* name = strings + symbol->n_strx;
		size_t remaining = symbol_table_command->strsize - symbol->n_strx;
		if (!memchr(name, '\0', remaining) ||
		    strncmp(name, symbol_prefix, prefix_length) != 0)
			continue;

		const char* native_name = name[0] == '_' ? name + 1 : name;
		void* replacement = dlsym(library, native_name);
		if (!replacement)
			continue;
		if (patch_function(image,
		                   (uintptr_t)symbol->n_value + image->slide,
		                   (uintptr_t)replacement, error, error_size) != 0)
			return -1;
		patched++;
	}
	return patched;
}
