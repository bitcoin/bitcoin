#include <stdio.h>

#include "num.h"
#include "field.h"
#include "group.h"
#include "ecmult.h"
#include "ecdsa.h"

using namespace secp256k1;

int main() {
    FieldElem x;
    const Number &order = GetGroupConst().order;
    Number r, s, m;
    Signature sig;
    x.SetHex("a357ae915c4a65281309edf20504740f0eb3343990216b4f81063cb65f2f7e0f");
    int cnt = 0;
    int good = 0;
    for (int i=0; i<1000000; i++) {
        r.SetPseudoRand(order);
        s.SetPseudoRand(order);
        m.SetPseudoRand(order);
        sig.SetRS(r,s);
        GroupElemJac pubkey; pubkey.SetCompressed(x, true);
        if (pubkey.IsValid()) {
            cnt++;
            good += sig.Verify(pubkey, m);
        }
     }
    printf("%i/%i\n", good, cnt);
    return 0;
}
