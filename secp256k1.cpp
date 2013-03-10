#include <stdio.h>

#include "num.h"
#include "field.h"
#include "group.h"
#include "ecmult.h"
#include "ecdsa.h"

using namespace secp256k1;

int main() {
    Context ctx;
    FieldElem x;
    const Number &order = GetGroupConst().order;
    Number r(ctx), s(ctx), m(ctx);
    Signature sig(ctx);
    x.SetHex("a357ae915c4a65281309edf20504740f0eb3343990216b4f81063cb65f2f7e0f");
    int cnt = 0;
    int good = 0;
    for (int i=0; i<1000000; i++) {
//        ECMult(ctx, a, a, an, gn);
//        an.SetModMul(ctx, af, order);
//        gn.SetModMul(ctx, gf, order);
//          an.Inc();
//          gn.Inc();
        r.SetPseudoRand(order);
        s.SetPseudoRand(order);
        if (i == 0)
            x.SetSquare(x);
        m.SetPseudoRand(order);
        sig.SetRS(r,s);
        GroupElemJac pubkey; pubkey.SetCompressed(x, true);
        if (pubkey.IsValid()) {
            cnt++;
            good += sig.Verify(ctx, pubkey, m);
        }
    }
    printf("%i/%i\n", good, cnt);
    return 0;
}
