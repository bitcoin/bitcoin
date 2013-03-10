#include <assert.h>

#include "num.h"
#include "field.h"
#include "group.h"
#include "ecmult.h"
#include "ecdsa.h"

using namespace secp256k1;

void test_ecmult() {
    Context ctx;
    FieldElem ax; ax.SetHex("8b30bbe9ae2a990696b22f670709dff3727fd8bc04d3362c6c7bf458e2846004");
    FieldElem ay; ay.SetHex("a357ae915c4a65281309edf20504740f0eb3343990216b4f81063cb65f2f7e0f");
    GroupElemJac a(ax,ay);
    Number xn(ctx); xn.SetHex("84cc5452f7fde1edb4d38a8ce9b1b84ccef31f146e569be9705d357a42985407");
    Number gn(ctx); gn.SetHex("a1e58d22553dcd42b23980625d4c57a96e9323d42b3152e5ca2c3990edc7c9de");
    Number xf(ctx); xf.SetHex("1337");
    Number gf(ctx); gf.SetHex("7113");
    Number ae(ctx); ae.SetHex("01");
    Number ge(ctx); ge.SetHex("00");
    GroupElemJac x = a;
    const Number &order = GetGroupConst().order;
    for (int i=0; i<1000; i++) {
        // in each iteration, compute X = xn*X + gn*G;
        ECMult(ctx, x, x, xn, gn);
        // also compute ae and ge: the actual accumulated factors for A and G
        // if X was (ae*A+ge*G), xn*X + gn*G results in (xn*ae*A + (xn*ge+gn)*G)
        ae.SetModMul(ctx, ae, xn, order);
        ge.SetModMul(ctx, ge, xn, order);
        ge.SetAdd(ctx, ge, gn);
        ge.SetMod(ctx, ge, order);
        // modify xn and gn
        xn.SetModMul(ctx, xn, xf, order);
        gn.SetModMul(ctx, gn, gf, order);
    }
    std::string res = x.ToString();
    assert(res == "(D37F97BBF58B4ECA238329D272C9AF0194F062B851EDF9B40F2294FA00BBFCA2,B127748E9A9F347257051588D44A1B822CA731833B2653AA3646C59A8ADAF295)");
    // redo the computation, but directly with the resulting ae and ge coefficients:
    GroupElemJac x2; ECMult(ctx, x2, a, ae, ge);
    std::string res2 = x2.ToString();
    printf("res=%s res2=%s\n", res.c_str(), res2.c_str());
    assert(res == res2);
}


int main(void) {
    test_ecmult();
    return 0;
}
