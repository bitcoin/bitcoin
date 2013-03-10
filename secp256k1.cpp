#include <stdio.h>

#include "num.h"
#include "field.h"
#include "group.h"
#include "scalar.h"
#include "ecmult.h"

using namespace secp256k1;

int main() {
    Context ctx;
    FieldElem x,y;
    x.SetHex("8b30bbe9ae2a990696b22f670709dff3727fd8bc04d3362c6c7bf458e2846004");
    y.SetHex("a357ae915c4a65281309edf20504740f0eb3343990216b4f81063cb65f2f7e0f");
    GroupElemJac a(x,y);
    printf("a=%s\n", a.ToString().c_str());
    Scalar an(ctx);
    an.SetHex("8b30bce9ad2a890696b23f671709eff3727fd8cc04d3362c6c7bf458f2846fff");
    Scalar af(ctx);
    af.SetHex("1337");
    printf("an=%s\n", an.ToString().c_str());
    Scalar gn(ctx);
    gn.SetHex("f557be925d4b65381409fdf30514750f1eb4343a91216a4f71163cb35f2f6e0e");
    Scalar gf(ctx);
    gf.SetHex("2113");
    printf("gn=%s\n", gn.ToString().c_str());
    for (int i=0; i<50000; i++) {
        ECMult(ctx, a, a, an, gn);
        an.Multiply(ctx, af);
        gn.Multiply(ctx, gf);
    }
    printf("%s\n", a.ToString().c_str());
    return 0;
}
