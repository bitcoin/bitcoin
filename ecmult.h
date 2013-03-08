#ifndef _SECP256K1_ECMULT_
#define _SECP256K1_ECMULT_

#include "group.h"
#include "scalar.h"

namespace secp256k1 {

template<typename G, int W> class WNAFPrecomp {
private:
    G pre[1 << (W-2)];

public:
    WNAFPrecomp(G &base) {
        pre[0] = base;
        GroupElemJac x = base;
        GroupElemJac d; d.SetDouble(x);
        for (int i=1; i<(1 << (W-2)); i++) {
            x.SetAdd(d,pre[i-1]);
            pre[i] = x;
        }
    }

    void Get(G &out, int exp) {
        assert((exp & 1) == 1);
        assert(exp >= -((1 << (W-1)) - 1));
        assert(exp <=  ((1 << (W-1)) - 1));
        if (exp > 0) {
            out = pre[(exp-1)/2];
        } else {
            out.SetNeg(pre[(1-exp)/2]);
        }
    }
};

class WNAF {
private:

};

}

#endif
