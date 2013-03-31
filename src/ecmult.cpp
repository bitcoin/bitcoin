#include <sstream>
#include <algorithm>

#include "num.h"
#include "group.h"
#include "ecmult.h"

// optimal for 128-bit and 256-bit exponents
#define WINDOW_A 5

// larger numbers may result in slightly better performance, at the cost of
// exponentially larger precomputed tables. WINDOW_G == 13 results in 640 KiB.
#define WINDOW_G 14

namespace secp256k1 {

template<typename G, int W> class WNAFPrecomp {
private:
    G pre[1 << (W-2)];

public:
    WNAFPrecomp() {}

    void Build(const G &base) {
        pre[0] = base;
        GroupElemJac x(base);
        GroupElemJac d; d.SetDouble(x);
        for (int i=1; i<(1 << (W-2)); i++) {
            x.SetAdd(d,pre[i-1]);
            pre[i].SetJac(x);
        }
    }

    WNAFPrecomp(const G &base) {
        Build(base);
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
        assert(used < B+1);
        for (int i=0; i<zeroes; i++) {
            naf[used++]=0;
        }
        naf[used++]=num;
    }

public:
    WNAF(const secp256k1_num_t &exp, int w) : used(0) {
        int zeroes = 0;
        secp256k1_num_t x;
        secp256k1_num_init(&x);
        secp256k1_num_copy(&x, &exp);
        int sign = 1;
        if (secp256k1_num_is_neg(&x)) {
            sign = -1;
            secp256k1_num_negate(&x);
        }
        while (!secp256k1_num_is_zero(&x)) {
            while (!secp256k1_num_is_odd(&x)) {
                zeroes++;
                secp256k1_num_shift(&x, 1);
            }
            int word = secp256k1_num_shift(&x, w);
            if (word & (1 << (w-1))) {
                secp256k1_num_inc(&x);
                PushNAF(sign * (word - (1 << w)), zeroes);
            } else {
                PushNAF(sign * word, zeroes);
            }
            zeroes = w-1;
        }
        secp256k1_num_free(&x);
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
    WNAFPrecomp<GroupElem,WINDOW_G> wpg;
    WNAFPrecomp<GroupElem,WINDOW_G> wpg128;
    GroupElem prec[64][16]; // prec[j][i] = 16^j * (i+1) * G
    GroupElem fin; // -(sum(prec[j][0], j=0..63))

    ECMultConsts() {
        const GroupElem &g = GetGroupConst().g;
        GroupElemJac g128j(g);
        for (int i=0; i<128; i++)
            g128j.SetDouble(g128j);
        GroupElem g128; g128.SetJac(g128j);
        wpg.Build(g);
        wpg128.Build(g128);
        GroupElemJac gg(g);
        GroupElem ad(g);
        GroupElemJac fn;
        for (int j=0; j<64; j++) {
            prec[j][0].SetJac(gg);
            fn.SetAdd(fn, gg);
            for (int i=1; i<16; i++) {
                gg.SetAdd(gg, ad);
                prec[j][i].SetJac(gg);
            }
            ad = prec[j][15];
        }
        fn.SetNeg(fn);
        fin.SetJac(fn);
    }
};

const ECMultConsts &GetECMultConsts() {
    static const ECMultConsts ecmult_consts;
    return ecmult_consts;
}

void ECMultBase(GroupElemJac &out, const secp256k1_num_t &gn) {
    secp256k1_num_t n;
    secp256k1_num_init(&n);
    secp256k1_num_copy(&n, &gn);
    const ECMultConsts &c = GetECMultConsts();
    out.SetAffine(c.prec[0][secp256k1_num_shift(&n, 4)]);
    for (int j=1; j<64; j++) {
        out.SetAdd(out, c.prec[j][secp256k1_num_shift(&n, 4)]);
    }
    secp256k1_num_free(&n);
    out.SetAdd(out, c.fin);
}

void ECMult(GroupElemJac &out, const GroupElemJac &a, const secp256k1_num_t &an, const secp256k1_num_t &gn) {
    secp256k1_num_t an1, an2;
    secp256k1_num_t gn1, gn2;

    secp256k1_num_init(&an1);
    secp256k1_num_init(&an2);
    secp256k1_num_init(&gn1);
    secp256k1_num_init(&gn2);

    SplitExp(an, an1, an2);
//    printf("an=%s\n", an.ToString().c_str());
//    printf("an1=%s\n", an1.ToString().c_str());
//    printf("an2=%s\n", an2.ToString().c_str());
//    printf("an1.len=%i\n", an1.GetBits());
//    printf("an2.len=%i\n", an2.GetBits());
    secp256k1_num_split(&gn1, &gn2, &gn, 128);

    WNAF<128> wa1(an1, WINDOW_A);
    WNAF<128> wa2(an2, WINDOW_A);
    WNAF<128> wg1(gn1, WINDOW_G);
    WNAF<128> wg2(gn2, WINDOW_G);
    GroupElemJac a2; a2.SetMulLambda(a);
    WNAFPrecomp<GroupElemJac,WINDOW_A> wpa1(a);
    WNAFPrecomp<GroupElemJac,WINDOW_A> wpa2(a2);
    const ECMultConsts &c = GetECMultConsts();

    int size_a1 = wa1.GetSize();
    int size_a2 = wa2.GetSize();
    int size_g1 = wg1.GetSize();
    int size_g2 = wg2.GetSize();
    int size = std::max(std::max(size_a1, size_a2), std::max(size_g1, size_g2));

    out = GroupElemJac();
    GroupElemJac tmpj;
    GroupElem tmpa;

    for (int i=size-1; i>=0; i--) {
        out.SetDouble(out);
        int nw;
        if (i < size_a1 && (nw = wa1.Get(i))) {
            wpa1.Get(tmpj, nw);
            out.SetAdd(out, tmpj);
        }
        if (i < size_a2 && (nw = wa2.Get(i))) {
            wpa2.Get(tmpj, nw);
            out.SetAdd(out, tmpj);
        }
        if (i < size_g1 && (nw = wg1.Get(i))) {
            c.wpg.Get(tmpa, nw);
            out.SetAdd(out, tmpa);
        }
        if (i < size_g2 && (nw = wg2.Get(i))) {
            c.wpg128.Get(tmpa, nw);
            out.SetAdd(out, tmpa);
        }
    }

    secp256k1_num_free(&an1);
    secp256k1_num_free(&an2);
    secp256k1_num_free(&gn1);
    secp256k1_num_free(&gn2);
}

}
