#include "native_structor_adapter.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

#define MACHGATE_STRUCTOR_ISLAND_SIZE 32u
#define MACHGATE_MAX_STRUCTOR_ADAPTERS 4096u
#define MACHGATE_MAX_STRUCTOR_ADAPTER_PAGES 32u
#define MACHGATE_ADAPTER_SEARCH_LIMIT (1u << 27)
#define MACHGATE_ADAPTER_SEARCH_STEP (1024u * 1024u)

struct structor_adapter_page {
	uint8_t* address;
	size_t used;
};

struct structor_adapter_entry {
	uintptr_t target;
	uintptr_t island;
};

struct machgate_structor_adapters {
	struct machgate_guest_image* image;
	size_t page_size;
	struct structor_adapter_page pages[MACHGATE_MAX_STRUCTOR_ADAPTER_PAGES];
	size_t page_count;
	struct structor_adapter_entry entries[MACHGATE_MAX_STRUCTOR_ADAPTERS];
	size_t entry_count;
};

void machgate_native_structor_adapter_entry(void);

static int read_source_name(const char** cursor)
{
	const char* start = *cursor;
	if (*start < '0' || *start > '9')
		return 0;

	size_t length = 0;
	const char* current = start;
	while (*current >= '0' && *current <= '9') {
		if (length > (SIZE_MAX - 9u) / 10u)
			return -1;
		length = length * 10u + (size_t)(*current - '0');
		current++;
	}
	if (!length || strlen(current) < length)
		return -1;
	*cursor = current + length;
	return 1;
}

static void skip_substitution(const char** cursor)
{
	const char* current = *cursor + 1;
	if (*current == 't' || *current == 'a' || *current == 'b' ||
	    *current == 's' || *current == 'i' || *current == 'o' ||
	    *current == 'd') {
		*cursor = current + 1;
		return;
	}
	while (*current && *current != '_')
		current++;
	if (*current == '_')
		current++;
	*cursor = current;
}

static void skip_template_parameter(const char** cursor)
{
	const char* current = *cursor + 1;
	while (*current >= '0' && *current <= '9')
		current++;
	if (*current == '_')
		current++;
	*cursor = current;
}

int machgate_native_symbol_has_structor_return(const char* guest_symbol)
{
	const char* name = guest_symbol;
	if (name[0] == '_' && name[1] == '_' && name[2] == 'Z')
		name++;
	if (strncmp(name, "_ZN", 3) != 0)
		return 0;

	const char* cursor = name + 3;
	size_t nesting = 0;
	size_t literal_nesting = 0;
	int literal_external_name = 0;
	int found = 0;
	while (*cursor) {
		if (literal_nesting && cursor[0] == '_' && cursor[1] == 'Z') {
			literal_external_name = 1;
			cursor += 2;
			continue;
		}
		if (!literal_nesting || literal_external_name) {
			int source_result = read_source_name(&cursor);
			if (source_result < 0)
				return 0;
			if (source_result > 0)
				continue;
		}

		if (*cursor == 'S') {
			skip_substitution(&cursor);
			continue;
		}
		if (*cursor == 'T') {
			skip_template_parameter(&cursor);
			continue;
		}
		if (*cursor == 'E') {
			if (nesting) {
				if (literal_nesting == nesting) {
					literal_nesting = 0;
					literal_external_name = 0;
				}
				nesting--;
				cursor++;
				continue;
			}
			return found;
		}

		if (!nesting &&
		    ((*cursor == 'C' && (cursor[1] == '1' || cursor[1] == '2')) ||
		     (*cursor == 'D' && (cursor[1] == '1' || cursor[1] == '2')))) {
			found = 1;
			cursor += 2;
			continue;
		}
		if (!nesting && *cursor == 'C' && cursor[1] == 'I' &&
		    (cursor[2] == '1' || cursor[2] == '2')) {
			found = 1;
			cursor += 3;
			continue;
		}

		int opens_group =
			*cursor == 'I' || *cursor == 'F' || *cursor == 'L' ||
		    *cursor == 'X' || *cursor == 'J' || *cursor == 'N' ||
		    *cursor == 'Z' ||
		    (*cursor == 'D' && (cursor[1] == 't' || cursor[1] == 'T')) ||
		    (*cursor == 'D' && (cursor[1] == 'O' || cursor[1] == 'w')) ||
		    (*cursor == 'U' && cursor[1] == 'l') ||
		    ((*cursor == 'c' || *cursor == 'i' || *cursor == 't') &&
		     cursor[1] == 'l');
		if (opens_group) {
			nesting++;
			if (*cursor == 'L' && !literal_nesting)
				literal_nesting = nesting;
		}
		cursor++;
	}
	return 0;
}

static uintptr_t align_up(uintptr_t value, size_t alignment)
{
	if (value > UINTPTR_MAX - (alignment - 1))
		return 0;
	return (value + alignment - 1) & ~((uintptr_t)alignment - 1);
}

static uint8_t* allocate_nearby_page(struct machgate_structor_adapters* adapters)
{
	uintptr_t start = align_up(adapters->image->mapping_end,
	                           adapters->page_size);
	if (!start)
		return NULL;
	for (uintptr_t distance = 0; distance < MACHGATE_ADAPTER_SEARCH_LIMIT;
	     distance += MACHGATE_ADAPTER_SEARCH_STEP) {
		if (start > UINTPTR_MAX - distance)
			break;
		void* result = mmap((void*)(start + distance), adapters->page_size,
		                    PROT_READ | PROT_WRITE,
		                    MAP_PRIVATE | MAP_ANONYMOUS |
		                        MAP_FIXED_NOREPLACE,
		                    -1, 0);
		if (result != MAP_FAILED)
			return result;
	}
	void* result = mmap(NULL, adapters->page_size, PROT_READ | PROT_WRITE,
	                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	return result == MAP_FAILED ? NULL : result;
}

static struct machgate_structor_adapters* create_adapters(
	struct machgate_guest_image* image, char* error, size_t error_size)
{
	long page_size_result = sysconf(_SC_PAGESIZE);
	if (page_size_result <= 0 ||
	    (size_t)page_size_result < MACHGATE_STRUCTOR_ISLAND_SIZE) {
		snprintf(error, error_size, "could not determine host page size");
		return NULL;
	}

	struct machgate_structor_adapters* result = calloc(1, sizeof(*result));
	if (!result) {
		snprintf(error, error_size,
		         "could not allocate native structor adapter state");
		return NULL;
	}
	result->image = image;
	result->page_size = (size_t)page_size_result;
	return result;
}

static struct structor_adapter_page* writable_page(
	struct machgate_structor_adapters* adapters, char* error,
	size_t error_size)
{
	if (adapters->page_count) {
		struct structor_adapter_page* page =
			&adapters->pages[adapters->page_count - 1];
		if (page->used <= adapters->page_size &&
		    MACHGATE_STRUCTOR_ISLAND_SIZE <=
			    adapters->page_size - page->used)
			return page;
	}
	if (adapters->page_count == MACHGATE_MAX_STRUCTOR_ADAPTER_PAGES) {
		snprintf(error, error_size,
		         "Mach-O requires too many native structor adapter pages");
		return NULL;
	}

	uint8_t* address = allocate_nearby_page(adapters);
	if (!address) {
		snprintf(error, error_size,
		         "could not allocate a nearby native structor adapter page");
		return NULL;
	}
	struct structor_adapter_page* result =
		&adapters->pages[adapters->page_count++];
	result->address = address;
	return result;
}

static uintptr_t emit_island(struct machgate_structor_adapters* adapters,
	                      uintptr_t target, char* error,
	                      size_t error_size)
{
	struct structor_adapter_page* page =
		writable_page(adapters, error, error_size);
	if (!page)
		return 0;

	uint8_t* island = page->address + page->used;
	uint32_t instructions[] = {
		0x58000090u,
		0x580000b1u,
		0xd61f0220u,
		0xd503201fu,
	};
	memcpy(island, instructions, sizeof(instructions));
	memcpy(island + 16, &target, sizeof(target));
	uintptr_t entry = (uintptr_t)machgate_native_structor_adapter_entry;
	memcpy(island + 24, &entry, sizeof(entry));
	page->used += MACHGATE_STRUCTOR_ISLAND_SIZE;
	return (uintptr_t)island;
}

int machgate_adapt_native_structor(
	struct machgate_guest_image* image,
	struct machgate_structor_adapters** adapters,
	const char* guest_symbol, uintptr_t native_target,
	uintptr_t* result, char* error, size_t error_size)
{
	*result = native_target;
	if (!machgate_native_symbol_has_structor_return(guest_symbol))
		return 0;

	if (!*adapters) {
		*adapters = create_adapters(image, error, error_size);
		if (!*adapters)
			return -1;
	}
	for (size_t index = 0; index < (*adapters)->entry_count; index++) {
		if ((*adapters)->entries[index].target != native_target)
			continue;
		*result = (*adapters)->entries[index].island;
		return 0;
	}
	if ((*adapters)->entry_count == MACHGATE_MAX_STRUCTOR_ADAPTERS) {
		snprintf(error, error_size,
		         "Mach-O requires too many native structor adapters");
		return -1;
	}

	uintptr_t island = emit_island(*adapters, native_target, error, error_size);
	if (!island)
		return -1;
	struct structor_adapter_entry* entry =
		&(*adapters)->entries[(*adapters)->entry_count++];
	entry->target = native_target;
	entry->island = island;
	*result = island;
	return 0;
}

int machgate_finish_native_structor_adapters(
	struct machgate_structor_adapters* adapters,
	char* error, size_t error_size)
{
	if (!adapters)
		return 0;
	for (size_t index = 0; index < adapters->page_count; index++) {
		struct structor_adapter_page* page = &adapters->pages[index];
		__builtin___clear_cache((char*)page->address,
		                        (char*)page->address + page->used);
		if (mprotect(page->address, adapters->page_size,
		             PROT_READ | PROT_EXEC) == 0)
			continue;
		snprintf(error, error_size,
		         "could not protect native structor adapters: %s",
		         strerror(errno));
		return -1;
	}
	free(adapters);
	return 0;
}

void machgate_discard_native_structor_adapters(
	struct machgate_structor_adapters* adapters)
{
	if (!adapters)
		return;
	for (size_t index = 0; index < adapters->page_count; index++)
		munmap(adapters->pages[index].address, adapters->page_size);
	free(adapters);
}
