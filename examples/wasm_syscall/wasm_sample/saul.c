#include <stdint.h>
#include "user/saul.h"

#define WASM_EXPORT __attribute__((visibility("default")))

WASM_EXPORT int main(int argc, char **argv)
{
    uintptr_t dev = saul_reg_find_nth(0);
    float val = saul_reg_read(dev);
    return 0;
}

