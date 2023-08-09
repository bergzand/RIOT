#include "rbpf/users/rbpf/rbpf_helpers.h"

uint32_t send_coap(void *ctx)
{
    (void)ctx;

    uint8_t buf[64];
    int handle = gcoap_req_init(buf, sizeof(buf), 1);
    const char path[] = "/.well-known/core";
    coap_opt_add_uri(handle, path, sizeof(path));
    coap_opt_finish(handle, false);

    const char dest[] = "[ff02::1]";
    gcoap_req_send(handle, 64, dest, sizeof(dest));
}

