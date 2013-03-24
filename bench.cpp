#include <stdio.h>

#include "num.cpp"
#include "field.cpp"
#include "group.cpp"
#include "ecmult.cpp"
#include "ecdsa.cpp"

using namespace secp256k1;

int main() {
    FieldElem x;
    const secp256k1_num_t &order = GetGroupConst().order;
    secp256k1_num_t r, s, m;
    secp256k1_num_start();
    secp256k1_num_init(&r);
    secp256k1_num_init(&s);
    secp256k1_num_init(&m);
    Signature sig;
    x.SetHex("a357ae915c4a65281309edf20504740f0eb3343990216b4f81063cb65f2f7e0f");
    int cnt = 0;
    int good = 0;
    for (int i=0; i<1000000; i++) {
        secp256k1_num_set_rand(&r, &order);
        secp256k1_num_set_rand(&s, &order);
        secp256k1_num_set_rand(&m, &order);
        sig.SetRS(r,s);
        GroupElemJac pubkey; pubkey.SetCompressed(x, true);
        if (pubkey.IsValid()) {
            cnt++;
            good += sig.Verify(pubkey, m);
        }
     }
    printf("%i/%i\n", good, cnt);
    secp256k1_num_free(&r);
    secp256k1_num_free(&s);
    secp256k1_num_free(&m);
    return 0;
}
