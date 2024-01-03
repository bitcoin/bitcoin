#include <blsct/external_api/blsct.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/mcl/mcl_init.h>
#include <cstdio>

extern "C" {

void BlsctInit() {
    MclInit for_side_effect_only;
}

void BlsctTest() {

    Mcl::Scalar a(1);
    Mcl::Scalar b(2);

    auto c = a + b;

    auto s = c.GetString();

    printf("The answer is %s\n", s.c_str());
}

} // extern "C"
