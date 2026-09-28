#include "instruction_adaptation.h"

#include "arm64_enc.h"
#include "macho_defs.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

#define ARM64_REGISTER_MASK 0x0000001fu
#define ARM64_MRS_SYSTEM_REGISTER_MASK 0xffffffe0u
#define ARM64_MRS_TPIDRRO_EL0 0xd53bd060u
#define ARM64_LDR_LITERAL_64 0x58000000u
#define THREAD_CONTEXT_SIZE (16u * 1024u)
#define ADAPTATION_ISLAND_WORDS 4u
#define ADAPTATION_POOL_SIZE (64u * 1024u)
#define ADAPTATION_POOL_ALIGNMENT (1024u * 1024u)
#define ADAPTATION_MAX_POOLS 64u

struct adaptation_pool {
	uint32_t* base;
	uint32_t* next;
	uint32_t* end;
};

static _Alignas(THREAD_CONTEXT_SIZE)
    unsigned char guest_thread_context[THREAD_CONTEXT_SIZE];
static struct adaptation_pool adaptation_pools[ADAPTATION_MAX_POOLS];
static size_t adaptation_pool_count;

void* machgate_guest_thread_context(void)
{
	return guest_thread_context;
}

static int section_is_executable(const struct section_64* section)
{
	return section->flags &
	       (S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS);
}

static int section_range_is_valid(const struct segment_command_64* segment,
				  const struct section_64* section)
{
	if (section->addr < segment->vmaddr)
		return 0;
	uint64_t section_offset = section->addr - segment->vmaddr;
	return section_offset <= segment->vmsize &&
	       section->size <= segment->vmsize - section_offset;
}

static int island_reachable(const uint32_t* instruction,
			    const uint32_t* island)
{
	return arm64_branch_in_range((int64_t)(uintptr_t)island -
	                             (int64_t)(uintptr_t)instruction);
}

static struct adaptation_pool* register_pool(void* mapping, size_t size)
{
	if (adaptation_pool_count == ADAPTATION_MAX_POOLS)
		return NULL;
	struct adaptation_pool* result =
	    &adaptation_pools[adaptation_pool_count++];
	result->base = mapping;
	result->next = mapping;
	result->end = (uint32_t*)((char*)mapping + size);
	return result;
}

static struct adaptation_pool* allocate_pool_near(uint32_t* instruction)
{
	uintptr_t origin = (uintptr_t)instruction &
	                   ~((uintptr_t)ADAPTATION_POOL_ALIGNMENT - 1);
	for (uintptr_t distance = ADAPTATION_POOL_ALIGNMENT;
	     distance < (UINT64_C(1) << 27);
	     distance += ADAPTATION_POOL_ALIGNMENT) {
		uintptr_t candidates[2] = {0};
		if (origin >= distance)
			candidates[0] = origin - distance;
		if (origin <= UINTPTR_MAX - distance)
			candidates[1] = origin + distance;

		for (size_t index = 0; index < 2; index++) {
			if (!candidates[index] ||
			    !arm64_branch_in_range(
				    (int64_t)candidates[index] -
				    (int64_t)(uintptr_t)instruction))
				continue;
			void* mapping = mmap(
			    (void*)candidates[index], ADAPTATION_POOL_SIZE,
			    PROT_READ | PROT_WRITE,
			    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
			    -1, 0);
			if (mapping == MAP_FAILED)
				continue;
			struct adaptation_pool* result =
			    register_pool(mapping, ADAPTATION_POOL_SIZE);
			if (result)
				return result;
			munmap(mapping, ADAPTATION_POOL_SIZE);
			return NULL;
		}
	}
	return NULL;
}

static struct adaptation_pool* find_pool(uint32_t* instruction)
{
	for (size_t index = 0; index < adaptation_pool_count; index++) {
		struct adaptation_pool* pool = &adaptation_pools[index];
		if ((size_t)(pool->end - pool->next) >=
		        ADAPTATION_ISLAND_WORDS &&
		    island_reachable(instruction, pool->next))
			return pool;
	}
	return allocate_pool_near(instruction);
}

static int adapt_thread_pointer_read(uint32_t* instruction, uint32_t encoding,
				     char* error, size_t error_size)
{
	struct adaptation_pool* pool = find_pool(instruction);
	if (!pool) {
		snprintf(error, error_size,
		         "could not allocate a nearby instruction-adaptation island");
		return -1;
	}

	uint32_t* island = pool->next;
	uint32_t destination_register = encoding & ARM64_REGISTER_MASK;
	int64_t return_distance = (int64_t)(uintptr_t)instruction -
	                          (int64_t)(uintptr_t)island;
	int64_t island_distance = -return_distance;
	island[0] = ARM64_LDR_LITERAL_64 | (2u << 5) | destination_register;
	island[1] = enc_b((int32_t)(return_distance / 4));
	uintptr_t context = (uintptr_t)guest_thread_context;
	memcpy(&island[2], &context, sizeof(context));
	*instruction = enc_b((int32_t)(island_distance / 4));
	pool->next += ADAPTATION_ISLAND_WORDS;
	return 0;
}

static int adapt_section(const struct machgate_guest_image* image,
			 const struct segment_command_64* segment,
			 const struct section_64* section, char* error,
			 size_t error_size)
{
	if (!section_is_executable(section))
		return 0;
	if (!section_range_is_valid(segment, section)) {
		snprintf(error, error_size,
		         "Mach-O executable section lies outside its segment");
		return -1;
	}

	int result = 0;
	for (uint64_t offset = 0; offset + sizeof(uint32_t) <= section->size;
	     offset += sizeof(uint32_t)) {
		uint32_t* instruction =
			(void*)(uintptr_t)(section->addr + image->slide + offset);
		uint32_t encoding;
		memcpy(&encoding, instruction, sizeof(encoding));
		if ((encoding & ARM64_MRS_SYSTEM_REGISTER_MASK) !=
		    ARM64_MRS_TPIDRRO_EL0)
			continue;
		if (adapt_thread_pointer_read(instruction, encoding, error,
		                              error_size) != 0)
			return -1;
		result++;
	}
	return result;
}

static int protect_adaptation_pools(char* error, size_t error_size)
{
	for (size_t index = 0; index < adaptation_pool_count; index++) {
		struct adaptation_pool* pool = &adaptation_pools[index];
		__builtin___clear_cache((char*)pool->base, (char*)pool->next);
		if (mprotect(pool->base, ADAPTATION_POOL_SIZE,
		             PROT_READ | PROT_EXEC) != 0) {
			snprintf(error, error_size,
			         "could not protect an instruction-adaptation island: %s",
			         strerror(errno));
			return -1;
		}
	}
	return 0;
}

int machgate_adapt_guest_instructions(struct machgate_guest_image* image,
				      char* error, size_t error_size)
{
	struct mach_header_64* header = image->header;
	uint8_t* cursor = (uint8_t*)(header + 1);
	int result = 0;

	adaptation_pool_count = 0;
	memset(guest_thread_context, 0, sizeof(guest_thread_context));
	*(uintptr_t*)guest_thread_context = (uintptr_t)guest_thread_context;

	for (uint32_t command_index = 0;
	     command_index < header->ncmds;
	     command_index++) {
		struct load_command* command = (void*)cursor;
		if (command->cmd == LC_SEGMENT_64) {
			struct segment_command_64* segment = (void*)command;
			if (segment->initprot & VM_PROT_EXECUTE) {
				struct section_64* sections = (void*)(segment + 1);
				for (uint32_t section_index = 0;
				     section_index < segment->nsects;
				     section_index++) {
					int section_result = adapt_section(
						image, segment, &sections[section_index],
						error, error_size);
					if (section_result < 0)
						return -1;
					result += section_result;
				}
			}
		}
		cursor += command->cmdsize;
	}

	if (protect_adaptation_pools(error, error_size) != 0)
		return -1;
	return result;
}
