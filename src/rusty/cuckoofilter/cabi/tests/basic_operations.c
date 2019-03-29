#include <stdio.h>
#include <assert.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "rcf_cuckoofilter.h"
#ifdef __cplusplus
}
#endif

int main(int argc, char **argv) {
    rcf_cuckoofilter *filter = rcf_cuckoofilter_with_capacity(1024);
    rcf_cuckoofilter_status status;

    status = rcf_cuckoofilter_add(filter, 42);
    assert(status == RCF_OK);
    printf("added 42: %d\n", status == RCF_OK);

    status = rcf_cuckoofilter_contains(filter, 42);
    assert(status == RCF_OK);
    printf("contains 42: %d\n", status == RCF_OK);

    status = rcf_cuckoofilter_contains(filter, 4711);
    assert(status == RCF_NOT_FOUND);
    printf("contains 4711: %d\n", status == RCF_OK);

    status = rcf_cuckoofilter_delete(filter, 42);
    assert(status == RCF_OK);
    printf("deleted 42: %d\n", status == RCF_OK);

    status = rcf_cuckoofilter_contains(filter, 42);
    assert(status == RCF_NOT_FOUND);
    printf("contains 42: %d\n", status == RCF_OK);

    rcf_cuckoofilter_free(filter);

    return 0;
}
