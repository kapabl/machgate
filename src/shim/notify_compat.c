#include <stddef.h>
#include <stdint.h>

#define NOTIFY_STATUS_INVALID_TOKEN 2u
#define NOTIFY_STATUS_FAILED 1000000u

uint32_t notify_register_file_descriptor(const char* name,
                                         int* notify_fd,
                                         int flags,
                                         int* out_token)
{
    (void)name;
    (void)flags;
    if (notify_fd != NULL) {
        *notify_fd = -1;
    }
    if (out_token != NULL) {
        *out_token = -1;
    }
    return NOTIFY_STATUS_FAILED;
}

uint32_t notify_cancel(int token)
{
    (void)token;
    return NOTIFY_STATUS_INVALID_TOKEN;
}

int notify_is_valid_token(int token)
{
    (void)token;
    return 0;
}
