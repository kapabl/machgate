#include "domain/provider_decision.h"

#include <stddef.h>
#include <string.h>

static int expect_decision(const char* value,
			   enum machgate_provider_decision_kind expected_kind,
			   const char* expected_path)
{
	enum machgate_provider_decision_kind kind;
	const char* path;
	if (machgate_provider_decision_parse(value, &kind, &path) != 0 ||
	    kind != expected_kind)
		return -1;
	if (!expected_path)
		return path ? -1 : 0;
	return path && strcmp(path, expected_path) == 0 ? 0 : -1;
}

int main(void)
{
	if (expect_decision("UNMAPPED", MACHGATE_PROVIDER_UNMAPPED, NULL) != 0 ||
	    expect_decision("SKIP", MACHGATE_PROVIDER_SKIP, NULL) != 0 ||
	    expect_decision("STUB", MACHGATE_PROVIDER_STUB, NULL) != 0 ||
	    expect_decision("provider.so", MACHGATE_PROVIDER_MAP,
			    "provider.so") != 0 ||
	    expect_decision("MAP: provider.so", MACHGATE_PROVIDER_MAP,
			    "provider.so") != 0 ||
	    expect_decision("PARTIAL: provider.so", MACHGATE_PROVIDER_PARTIAL,
			    "provider.so") != 0)
		return 1;

	enum machgate_provider_decision_kind kind;
	const char* path;
	return machgate_provider_decision_parse("MAP: ", &kind, &path) == 0;
}
