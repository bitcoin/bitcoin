#ifndef _SECP256K1_ECMULT_
#define _SECP256K1_ECMULT_

#include <sstream>
#include <algorithm>

#include "group.h"
#include "scalar.h"

#define WINDOW_A 5
#define WINDOW_G 11

namespace secp256k1 {

template<typename G, int W> class WNAFPrecomp {
private:
    G pre[1 << (W-2)];

public:
    WNAFPrecomp(const G &base) {
        pre[0] = base;
        GroupElemJac x(base);
//        printf("base=%s x=%s\n", base.ToString().c_str(), x.ToString().c_str());
        GroupElemJac d; d.SetDouble(x);
//        printf("d=%s\n", d.ToString().c_str());
        for (int i=1; i<(1 << (W-2)); i++) {
            x.SetAdd(d,pre[i-1]);
            pre[i].SetJac(x);
//            printf("precomp %s*%i = %s\n", base.ToString().c_str(), i*2 +1, pre[i].ToString().c_str());
        }
    }

    void Get(G &out, int exp) const {
        assert((exp & 1) == 1);
        assert(exp >= -((1 << (W-1)) - 1));
        assert(exp <=  ((1 << (W-1)) - 1));
        if (exp > 0) {
            out = pre[(exp-1)/2];
        } else {
            out.SetNeg(pre[(-exp-1)/2]);
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
    WNAF(Context &ctx, const Scalar &exp, int w) : used(0) {
        int zeroes = 0;
        Context ct(ctx);
        Scalar x(ct);
        x.SetNumber(exp);
        while (!x.IsZero()) {
            while (!x.IsOdd()) {
                zeroes++;
                x.Shift1();
            }
            int word = x.ShiftLowBits(ctx,w);
            if (word & (1 << (w-1))) {
                x.Inc();
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
        assert(pos >= 0 && pos < used);
        return naf[pos];
    }

    std::string ToString() {
        std::stringstream ss;
        ss << "(";
        for (int i=0; i<GetSize(); i++) {
            ss << Get(used-1-i);
            if (i != used-1)
                ss << ',';
        }
        ss << ")";
        return ss.str();
    }
};

class ECMultConsts {
public:
    const WNAFPrecomp<GroupElem,WINDOW_G> wpg;

    ECMultConsts() : wpg(GetGroupConst().g) {}
};

const ECMultConsts &GetECMultConsts() {
    static const ECMultConsts ecmult_consts;
    return ecmult_consts;
}

void ECMult(Context &ctx, GroupElemJac &out, const GroupElemJac &a, Scalar &an, Scalar &gn) {
    WNAF<256> wa(ctx, an, WINDOW_A);
    WNAF<256> wg(ctx, gn, WINDOW_G);
    WNAFPrecomp<GroupElemJac,WINDOW_A> wpa(a);
    const WNAFPrecomp<GroupElem,WINDOW_G> &wpg = GetECMultConsts().wpg;

    int size_a = wa.GetSize();
    int size_g = wg.GetSize();
    int size = std::max(size_a, size_g);

    out = GroupElemJac();
    GroupElemJac tmpj;
    GroupElem tmpa;

    for (int i=size-1; i>=0; i--) {
        out.SetDouble(out);
        int nw;
        if (i < size_a && (nw = wa.Get(i))) {
            wpa.Get(tmpj, nw);
            out.SetAdd(out, tmpj);
        }
        if (i < size_g && (nw = wg.Get(i))) {
            wpg.Get(tmpa, nw);
            out.SetAdd(out, tmpa);
        }
    }
}

}

#endif
