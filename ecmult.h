#ifndef _SECP256K1_ECMULT_
#define _SECP256K1_ECMULT_

#include <sstream>
#include <algorithm>

#include "group.h"
#include "scalar.h"

namespace secp256k1 {

template<typename G, int W> class WNAFPrecomp {
private:
    G pre[1 << (W-2)];

public:
    WNAFPrecomp(const G &base) {
        pre[0] = base;
        GroupElemJac x = base;
        GroupElemJac d; d.SetDouble(x);
        for (int i=1; i<(1 << (W-2)); i++) {
            x.SetAdd(d,pre[i-1]);
            pre[i].SetJac(x);
        }
    }

    void Get(G &out, int exp) const {
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

template<int B> class WNAF {
private:
    int naf[B+1];
    int used;

    void PushNAF(int num, int zeroes) {
        for (int i=0; i<zeroes; i++) {
            naf[used++]=0;
        }
        naf[used++]=num;
    }

public:
    WNAF(Context &ctx, Scalar &exp, int w) : used(0) {
        int zeroes = 0;
        while (!exp.IsZero()) {
            while (!exp.IsOdd()) {
                zeroes++;
                exp.Shift1();
            }
            int word = exp.ShiftLowBits(ctx,w);
            if (word & (1 << (w-1))) {
                exp.Inc();
                PushNAF(word - (1 << w), zeroes);
            } else {
                PushNAF(word, zeroes);
            }
            zeroes = w-1;
        }
    }

    int GetSize() const {
        return used;
    }

    int Get(int pos) const {
        return naf[used-1-pos];
    }

    std::string ToString() {
        std::stringstream ss;
        ss << "(";
        for (int i=0; i<GetSize(); i++) {
            ss << Get(i);
            if (i != used-1)
                ss << ',';
        }
        ss << ")";
        return ss.str();
    }
};

class ECMultConsts {
public:
    const WNAFPrecomp<GroupElem,10> wpg;

    ECMultConsts() : wpg(GetGroupConst().g) {}
};

const ECMultConsts &GetECMultConsts() {
    static const ECMultConsts ecmult_consts;
    return ecmult_consts;
}

void ECMult(Context &ctx, GroupElemJac &out, GroupElemJac &a, Scalar &an, Scalar &gn) {
    WNAF<256> wa(ctx, an, 5);
    WNAF<256> wg(ctx, gn, 10);
    WNAFPrecomp<GroupElemJac,5> wpa(a);
    const WNAFPrecomp<GroupElem,10> &wpg = GetECMultConsts().wpg;

    int size = std::max(wa.GetSize(), wg.GetSize());

    out = GroupElemJac();
    GroupElemJac tmpj;
    GroupElem tmpa;

    for (int i=0; i<size; i++) {
        out.SetDouble(out);
        int anw = wa.Get(i);
        if (anw) {
            wpa.Get(tmpj, anw);
            out.SetAdd(out, tmpj);
        }
        int gnw = wg.Get(i);
        if (gnw) {
            wpg.Get(tmpa, gnw);
            out.SetAdd(out, tmpa);
        }
    }
}

}

#endif
