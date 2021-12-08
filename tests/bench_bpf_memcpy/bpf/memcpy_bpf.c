#include <stddef.h>
#include "bpf/shared.h"
#include "unaligned.h"

typedef struct {
    const uint8_t * src;
    uint8_t * dst;
    uint32_t len;
} memcpy_ctx_t;

int memcpy_bpf(const memcpy_ctx_t *ctx)
{
    for (uint32_t i = 0; i < ctx->len; i++) {
        ctx->dst[i] = ctx->src[i];
    }
    return 0;
}
