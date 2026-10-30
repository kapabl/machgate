#include "domain/provider_decision.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

int machgate_provider_decision_parse(
	const char* value, enum machgate_provider_decision_kind* kind,
	const char** provider_path)
{
	if (!value || !kind || !provider_path)
		return -1;
	*provider_path = NULL;
	if (strcmp(value, "UNMAPPED") == 0) {
		*kind = MACHGATE_PROVIDER_UNMAPPED;
		return 0;
	}
	if (strcmp(value, "SKIP") == 0) {
		*kind = MACHGATE_PROVIDER_SKIP;
		return 0;
	}
	if (strcmp(value, "STUB") == 0) {
		*kind = MACHGATE_PROVIDER_STUB;
		return 0;
	}

	if (strncmp(value, "MAP:", 4) == 0) {
		*kind = MACHGATE_PROVIDER_MAP;
		value += 4;
	} else if (strncmp(value, "PARTIAL:", 8) == 0) {
		*kind = MACHGATE_PROVIDER_PARTIAL;
		value += 8;
	} else {
		*kind = MACHGATE_PROVIDER_MAP;
	}
	while (isspace((unsigned char)*value))
		value++;
	if (!*value)
		return -1;
	*provider_path = value;
	return 0;
}
