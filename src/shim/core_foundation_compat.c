#include <stdint.h>
#include <stdlib.h>

typedef int64_t CFIndex;
typedef const void* CFAllocatorRef;
typedef const void* CFDictionaryRef;
typedef const void* CFErrorRef;
typedef const void* CFStringRef;
typedef uint32_t CFStringEncoding;

enum {
	CF_TYPE_ID_BOOLEAN = 5,
};

struct cf_boolean_value {
	uintptr_t type_id;
};

struct cf_error_value {
	CFStringRef domain;
	CFIndex code;
	CFDictionaryRef user_info;
};

extern CFStringRef CFStringCreateWithCString(
	CFAllocatorRef allocator, const char* string, CFStringEncoding encoding);

static const char cf_null_value[] = "MachGate/kCFNull";
static const struct cf_boolean_value cf_boolean_false_value = {
	CF_TYPE_ID_BOOLEAN,
};
static const char cf_run_loop_default_mode_value[] =
	"kCFRunLoopDefaultMode";
static const char cf_error_domain_os_status_value[] =
	"NSOSStatusErrorDomain";

const void* kCFNull = cf_null_value;
const void* kCFBooleanFalse = &cf_boolean_false_value;
const void* kCFRunLoopDefaultMode = cf_run_loop_default_mode_value;
const void* kCFErrorDomainOSStatus = cf_error_domain_os_status_value;

CFErrorRef CFErrorCreate(CFAllocatorRef allocator, CFStringRef domain,
			 CFIndex code, CFDictionaryRef user_info)
{
	(void)allocator;

	if (!domain)
		return NULL;

	struct cf_error_value* error = calloc(1, sizeof(*error));
	if (!error)
		return NULL;
	error->domain = domain;
	error->code = code;
	error->user_info = user_info;
	return error;
}

CFIndex CFErrorGetCode(CFErrorRef error_ref)
{
	const struct cf_error_value* error = error_ref;
	return error ? error->code : 0;
}

CFStringRef CFErrorGetDomain(CFErrorRef error_ref)
{
	const struct cf_error_value* error = error_ref;
	return error ? error->domain : NULL;
}

CFStringRef CFStringCreateWithCStringNoCopy(
	CFAllocatorRef allocator, const char* string, CFStringEncoding encoding,
	CFAllocatorRef contents_deallocator)
{
	(void)contents_deallocator;
	return CFStringCreateWithCString(allocator, string, encoding);
}
