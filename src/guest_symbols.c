#include "guest_symbols.h"

#include "macho_defs.h"

#include <stdint.h>
#include <string.h>

static void* address_for_file_offset(struct mach_header_64* header,
				     intptr_t slide, uint64_t file_offset,
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
				return (void*)(uintptr_t)(segment->vmaddr + slide +
				                         file_offset - segment->fileoff);
			}
		}
		cursor += command->cmdsize;
	}
	return NULL;
}

static uint32_t section_flags(struct mach_header_64* header,
			      uint8_t target_section)
{
	uint8_t section_number = 1;
	uint8_t* cursor = (uint8_t*)(header + 1);
	for (uint32_t index = 0; index < header->ncmds; index++) {
		struct load_command* command = (void*)cursor;
		if (command->cmd == LC_SEGMENT_64) {
			struct segment_command_64* segment = (void*)command;
			struct section_64* sections = (void*)(segment + 1);
			for (uint32_t section_index = 0;
			     section_index < segment->nsects;
			     section_index++, section_number++) {
				if (section_number == target_section)
					return sections[section_index].flags;
			}
		}
		cursor += command->cmdsize;
	}
	return S_SYMBOL_STUBS;
}

void* machgate_find_guest_definition(struct machgate_guest_image* image,
				     const char* name)
{
	struct mach_header_64* header = image->header;
	struct symtab_command* symbol_table = NULL;
	uint8_t* cursor = (uint8_t*)(header + 1);
	for (uint32_t index = 0; index < header->ncmds; index++) {
		struct load_command* command = (void*)cursor;
		if (command->cmd == LC_SYMTAB)
			symbol_table = (void*)command;
		cursor += command->cmdsize;
	}
	if (!symbol_table)
		return NULL;

	uint64_t symbol_bytes =
		(uint64_t)symbol_table->nsyms * sizeof(struct nlist_64);
	struct nlist_64* symbols = address_for_file_offset(
		header, image->slide, symbol_table->symoff, symbol_bytes);
	char* strings = address_for_file_offset(
		header, image->slide, symbol_table->stroff, symbol_table->strsize);
	if (!symbols || !strings)
		return NULL;

	for (uint32_t index = 0; index < symbol_table->nsyms; index++) {
		struct nlist_64* symbol = &symbols[index];
		if ((symbol->n_type & N_STAB) != 0 ||
		    (symbol->n_type & N_TYPE) != N_SECT ||
		    (symbol->n_type & N_EXT) == 0 || !symbol->n_value ||
		    !symbol->n_sect || symbol->n_strx >= symbol_table->strsize)
			continue;
		const char* symbol_name = strings + symbol->n_strx;
		size_t remaining = symbol_table->strsize - symbol->n_strx;
		if (!memchr(symbol_name, '\0', remaining) ||
		    strcmp(symbol_name, name) != 0)
			continue;
		if ((section_flags(header, symbol->n_sect) & SECTION_TYPE) ==
		    S_SYMBOL_STUBS)
			return NULL;
		return (void*)(uintptr_t)(symbol->n_value + image->slide);
	}
	return NULL;
}
