#include "rbpf/users/rbpf/rbpf_helpers.h"

uint32_t get_random_num(void *ctx)
{
    (void)ctx;

    uint32_t random_num = 0;

    bpf_get_prandom_buf(&random_num, sizeof(random_num));

    return random_num;
}
