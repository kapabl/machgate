#include <stddef.h>
#include <stdint.h>

typedef const void* CFArrayRef;
typedef int32_t OSStatus;

extern int CCRandomGenerateBytes(void* bytes, size_t count);

OSStatus SecRandomCopyBytes(const void* random_source, size_t count,
	                    uint8_t* bytes)
{
	(void)random_source;
	return CCRandomGenerateBytes(bytes, count) == 0 ? 0 : -1;
}

OSStatus SecTrustSettingsCopyCertificates(int domain,
	                                  CFArrayRef* certificates)
{
	(void)domain;
	if (certificates)
		*certificates = NULL;
	return -25300;
}
