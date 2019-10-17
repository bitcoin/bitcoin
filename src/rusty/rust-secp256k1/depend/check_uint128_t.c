
#include <stdint.h>

int main(void) {
    __uint128_t var_128;
    uint64_t var_64;

    /* Try to shut up "unused variable" warnings */
    var_64 = 100;
    var_128 = 100;
    if (var_64 == var_128) {
        var_64 = 20;
    }
    return 0;
}

