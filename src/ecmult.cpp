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

template<int W> class WNAFPrecompJac {
private:
    secp256k1_gej_t pre[1 << (W-2)];

public:
    WNAFPrecompJac() {}

    void Build(const secp256k1_gej_t &base) {
        pre[0] = base;
        secp256k1_gej_t d; secp256k1_gej_double(&d, &pre[0]);
        for (int i=1; i<(1 << (W-2)); i++)
            secp256k1_gej_add(&pre[i], &d, &pre[i-1]);
    }

    WNAFPrecompJac(const secp256k1_gej_t &base) {
        Build(base);
    }

    void Get(secp256k1_gej_t &out, int exp) const {
        assert((exp & 1) == 1);
        assert(exp >= -((1 << (W-1)) - 1));
        assert(exp <=  ((1 << (W-1)) - 1));
        if (exp > 0) {
            out = pre[(exp-1)/2];
        } else {
            secp256k1_gej_neg(&out, &pre[(-exp-1)/2]);
        }
    }
};

template<int W> class WNAFPrecompAff {
private:
    secp256k1_ge_t pre[1 << (W-2)];

public:
    WNAFPrecompAff() {}

    void Build(const secp256k1_ge_t &base) {
        pre[0] = base;
        secp256k1_gej_t x; secp256k1_gej_set_ge(&x, &base);
        secp256k1_gej_t d; secp256k1_gej_double(&d, &x);
        for (int i=1; i<(1 << (W-2)); i++) {
            secp256k1_gej_add_ge(&x, &d, &pre[i-1]);
            secp256k1_ge_set_gej(&pre[i], &x);
        }
    }

    WNAFPrecompAff(const secp256k1_ge_t &base) {
        Build(base);
    }

    void Get(secp256k1_ge_t &out, int exp) const {
        assert((exp & 1) == 1);
        assert(exp >= -((1 << (W-1)) - 1));
        assert(exp <=  ((1 << (W-1)) - 1));
        if (exp > 0) {
            out = pre[(exp-1)/2];
        } else {
            secp256k1_ge_neg(&out, &pre[(-exp-1)/2]);
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
    WNAFPrecompAff<WINDOW_G> wpg;
    WNAFPrecompAff<WINDOW_G> wpg128;
    secp256k1_ge_t prec[64][16]; // prec[j][i] = 16^j * (i+1) * G
    secp256k1_ge_t fin; // -(sum(prec[j][0], j=0..63))

    ECMultConsts() {
        const secp256k1_ge_t &g = secp256k1_ge_consts->g;
        secp256k1_gej_t g128j; secp256k1_gej_set_ge(&g128j, &g);
        for (int i=0; i<128; i++)
            secp256k1_gej_double(&g128j, &g128j);
        secp256k1_ge_t g128; secp256k1_ge_set_gej(&g128, &g128j);
        wpg.Build(g);
        wpg128.Build(g128);
        secp256k1_gej_t gg; secp256k1_gej_set_ge(&gg, &g);
        secp256k1_ge_t ad = g;
        secp256k1_gej_t fn; secp256k1_gej_set_infinity(&fn);
        for (int j=0; j<64; j++) {
            secp256k1_ge_set_gej(&prec[j][0], &gg);
            secp256k1_gej_add(&fn, &fn, &gg);
            for (int i=1; i<16; i++) {
                secp256k1_gej_add_ge(&gg, &gg, &ad);
                secp256k1_ge_set_gej(&prec[j][i], &gg);
            }
            ad = prec[j][15];
        }
        secp256k1_ge_set_gej(&fin, &fn);
        secp256k1_ge_neg(&fin, &fin);
    }
};

const ECMultConsts &GetECMultConsts() {
    static const ECMultConsts ecmult_consts;
    return ecmult_consts;
}

void ECMultBase(secp256k1_gej_t &out, const secp256k1_num_t &gn) {
    secp256k1_num_t n;
    secp256k1_num_init(&n);
    secp256k1_num_copy(&n, &gn);
    const ECMultConsts &c = GetECMultConsts();
    secp256k1_gej_set_ge(&out, &c.prec[0][secp256k1_num_shift(&n, 4)]);
    for (int j=1; j<64; j++) {
        secp256k1_gej_add_ge(&out, &out, &c.prec[j][secp256k1_num_shift(&n, 4)]);
    }
    secp256k1_num_free(&n);
    secp256k1_gej_add_ge(&out, &out, &c.fin);
}

void ECMult(secp256k1_gej_t &out, const secp256k1_gej_t &a, const secp256k1_num_t &an, const secp256k1_num_t &gn) {
    secp256k1_num_t an1, an2;
    secp256k1_num_t gn1, gn2;

    secp256k1_num_init(&an1);
    secp256k1_num_init(&an2);
    secp256k1_num_init(&gn1);
    secp256k1_num_init(&gn2);

    secp256k1_gej_split_exp(&an1, &an2, &an);
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
    secp256k1_gej_t a2; secp256k1_gej_mul_lambda(&a2, &a);
    WNAFPrecompJac<WINDOW_A> wpa1(a);
    WNAFPrecompJac<WINDOW_A> wpa2(a2);
    const ECMultConsts &c = GetECMultConsts();

    int size_a1 = wa1.GetSize();
    int size_a2 = wa2.GetSize();
    int size_g1 = wg1.GetSize();
    int size_g2 = wg2.GetSize();
    int size = std::max(std::max(size_a1, size_a2), std::max(size_g1, size_g2));

    out; secp256k1_gej_set_infinity(&out);
    secp256k1_gej_t tmpj;
    secp256k1_ge_t tmpa;

    for (int i=size-1; i>=0; i--) {
        secp256k1_gej_double(&out, &out);
        int nw;
        if (i < size_a1 && (nw = wa1.Get(i))) {
            wpa1.Get(tmpj, nw);
            secp256k1_gej_add(&out, &out, &tmpj);
        }
        if (i < size_a2 && (nw = wa2.Get(i))) {
            wpa2.Get(tmpj, nw);
            secp256k1_gej_add(&out, &out, &tmpj);
        }
        if (i < size_g1 && (nw = wg1.Get(i))) {
            c.wpg.Get(tmpa, nw);
            secp256k1_gej_add_ge(&out, &out, &tmpa);
        }
        if (i < size_g2 && (nw = wg2.Get(i))) {
            c.wpg128.Get(tmpa, nw);
            secp256k1_gej_add_ge(&out, &out, &tmpa);
        }
    }

    secp256k1_num_free(&an1);
    secp256k1_num_free(&an2);
    secp256k1_num_free(&gn1);
    secp256k1_num_free(&gn2);
}

}
