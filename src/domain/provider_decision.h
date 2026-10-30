#ifndef MACHGATE_DOMAIN_PROVIDER_DECISION_H
#define MACHGATE_DOMAIN_PROVIDER_DECISION_H

enum machgate_provider_decision_kind {
	MACHGATE_PROVIDER_UNMAPPED,
	MACHGATE_PROVIDER_MAP,
	MACHGATE_PROVIDER_SKIP,
	MACHGATE_PROVIDER_STUB,
	MACHGATE_PROVIDER_PARTIAL,
};

int machgate_provider_decision_parse(
	const char* value, enum machgate_provider_decision_kind* kind,
	const char** provider_path);

#endif
