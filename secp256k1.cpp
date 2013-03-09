#include <stdio.h>

#include "num.h"
#include "scalar.h"
#include "field.h"
#include "group.h"
#include "ecmult.h"

using namespace secp256k1;

int main() {
    Context ctx;
    Scalar scal(ctx);
    scal.SetHex("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
    printf("scal=%s\n", scal.ToString().c_str());
    WNAF<256> w(ctx, scal, 5);
    printf("wnaf=%s\n", w.ToString().c_str());
    return 0;
}
