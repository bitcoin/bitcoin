#include <stdint.h>
#include <string>
#include <stdio.h>
#include <gmpxx.h>
#include <assert.h>

// #define VERIFY_BADNESS 1

namespace secp256k1 {


/** Implements arithmetic modulo FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F 
 * A FieldElem has an implicit 'badness':
 * - the output of SetSquare, SetMult and Normalize sets the badness to 1
 * - the inputs of SetSquare and SetMult cannot have badness above 8
 * - adding two FieldElems adds the badness together
 * - multiplying a FieldElem with a constant multiplies its badness by that constant
 * - taking the negative requires specifying the badness of the input, the output having badness 1 higher
 */
class FieldElem {
private:
    // X = sum(i=0..4, elem[i]*2^52)
    uint64_t n[5];
#ifdef VERIFY_BADNESS
    int badness;
#endif


public:

    FieldElem(int x = 0) {
        n[0] = x;
        n[1] = n[2] = n[3] = n[4] = 0;
#ifdef VERIFY_BADNESS
        badness = 1;
#endif
    }

    void Normalize() {
        uint64_t c;
        c = n[0];
        uint64_t t0 = c & 0xFFFFFFFFFFFFFULL;
        c = (c >> 52) + n[1];
        uint64_t t1 = c & 0xFFFFFFFFFFFFFULL;
        c = (c >> 52) + n[2];
        uint64_t t2 = c & 0xFFFFFFFFFFFFFULL;
        c = (c >> 52) + n[3];
        uint64_t t3 = c & 0xFFFFFFFFFFFFFULL;
        c = (c >> 52) + n[4];
        uint64_t t4 = c & 0x0FFFFFFFFFFFFULL;
        c >>= 48;
        if (c) {
            c = c * 0x1000003D1ULL + t0;
            t0 = c & 0xFFFFFFFFFFFFFULL;
            c = (c >> 52) + t1;
            t1 = c & 0xFFFFFFFFFFFFFULL;
            c = (c >> 52) + t2;
            t2 = c & 0xFFFFFFFFFFFFFULL;
            c = (c >> 52) + t3;
            t3 = c & 0xFFFFFFFFFFFFFULL;
            c = (c >> 52) + t4;
            t4 = c & 0x0FFFFFFFFFFFFULL;
            c >>= 48;
        }
        if (t4 == 0xFFFFFFFFFFFFULL && t3 == 0xFFFFFFFFFFFFFULL && t2 == 0xFFFFFFFFFFFFFULL && t3 == 0xFFFFFFFFFFFFF && t4 >= 0xFFFFEFFFFFC2FULL) {
            t4 = 0;
            t3 = 0;
            t2 = 0;
            t1 = 0;
            t0 -= 0xFFFFEFFFFFC2FULL;
        }
        n[0] = t0; n[1] = t1; n[2] = t2; n[3] = t3; n[4] = t4;
#ifdef VERIFY_BADNESS
        badness = 1;
#endif
    }

    bool IsZero() {
        Normalize();
        return n[0] == 0 && n[1] == 0 && n[2] == 0 && n[3] == 0 && n[4] == 0;
    }

    bool friend operator==(FieldElem &a, FieldElem &b) {
        a.Normalize();
        b.Normalize();
        return a.n[0] == b.n[0] && a.n[1] == b.n[1] && a.n[2] == b.n[2] && a.n[3] == b.n[3] && a.n[4] == b.n[4];
    }

    void Get(uint64_t *out) {
        Normalize();
        out[0] = n[0] | (n[1] << 52);
        out[1] = (n[1] >> 12) | (n[2] << 40);
        out[2] = (n[2] >> 24) | (n[3] << 28);
        out[3] = (n[3] >> 36) | (n[4] << 16);
    }

    void Set(const uint64_t *in) {
        n[0] = in[0] & 0xFFFFFFFFFFFFFULL;
        n[1] = ((in[0] >> 52) | (in[1] << 12)) & 0xFFFFFFFFFFFFFULL;
        n[2] = ((in[1] >> 40) | (in[2] << 24)) & 0xFFFFFFFFFFFFFULL;
        n[3] = ((in[2] >> 28) | (in[3] << 36)) & 0xFFFFFFFFFFFFFULL;
        n[4] = (in[3] >> 16);
#ifdef VERIFY_BADNESS
        badness = 1;
#endif
    }

    void SetNeg(const FieldElem &a, int badnessIn) {
#ifdef VERIFY_BADNESS
        assert(a.badness <= badnessIn);
        badness = badnessIn + 1;
#endif
        n[0] = 0xFFFFEFFFFFC2FULL * (badnessIn + 1) - a.n[0];
        n[1] = 0xFFFFFFFFFFFFFULL * (badnessIn + 1) - a.n[1];
        n[2] = 0xFFFFFFFFFFFFFULL * (badnessIn + 1) - a.n[2];
        n[3] = 0xFFFFFFFFFFFFFULL * (badnessIn + 1) - a.n[3];
        n[4] = 0x0FFFFFFFFFFFFULL * (badnessIn + 1) - a.n[4];
    }

    void operator*=(int v) {
#ifdef VERIFY_BADNESS
        badness *= v;
#endif
        n[0] *= v;
        n[1] *= v;
        n[2] *= v;
        n[3] *= v;
        n[4] *= v;
    }

    void operator+=(const FieldElem &a) {
#ifdef VERIFY_BADNESS
        badness += a.badness;
#endif
        n[0] += a.n[0];
        n[1] += a.n[1];
        n[2] += a.n[2];
        n[3] += a.n[3];
        n[4] += a.n[4];
    }

    // precondition: all values in a and b are at most 56 bits, except n[4] is at most 52 bits
    // postcondition: all values are at most 53 bits, except n[4] is at most 49 bits
    void SetMult(const FieldElem &a, const FieldElem &b) {
#ifdef VERIFY_BADNESS
        assert(a.badness <= 8);
        assert(b.badness <= 8);
#endif
        __int128 c = (__int128)a.n[0] * b.n[0];
        uint64_t t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0FFFFFFFFFFFFFE0
        c = c + (__int128)a.n[0] * b.n[1] +
                (__int128)a.n[1] * b.n[0];
        uint64_t t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 20000000000000BF
        c = c + (__int128)a.n[0] * b.n[2] +
                (__int128)a.n[1] * b.n[1] +
                (__int128)a.n[2] * b.n[0];
        uint64_t t2 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 30000000000001A0
        c = c + (__int128)a.n[0] * b.n[3] +
                (__int128)a.n[1] * b.n[2] +
                (__int128)a.n[2] * b.n[1] +
                (__int128)a.n[3] * b.n[0];
        uint64_t t3 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 4000000000000280
        c = c + (__int128)a.n[0] * b.n[4] +
                (__int128)a.n[1] * b.n[3] +
                (__int128)a.n[2] * b.n[2] +
                (__int128)a.n[3] * b.n[1] +
                (__int128)a.n[4] * b.n[0];
        uint64_t t4 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 320000000000037E
        c = c + (__int128)a.n[1] * b.n[4] +
                (__int128)a.n[2] * b.n[3] +
                (__int128)a.n[3] * b.n[2] +
                (__int128)a.n[4] * b.n[1];
        uint64_t t5 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 22000000000002BE
        c = c + (__int128)a.n[2] * b.n[4] +
                (__int128)a.n[3] * b.n[3] +
                (__int128)a.n[4] * b.n[2];
        uint64_t t6 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 12000000000001DE
        c = c + (__int128)a.n[3] * b.n[4] +
                (__int128)a.n[4] * b.n[3];
        uint64_t t7 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 02000000000000FE
        c = c + (__int128)a.n[4] * b.n[4];
        uint64_t t8 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 001000000000001E
        uint64_t t9 = c;
        c = t0 + (__int128)t5 * 0x1000003D10ULL;
        t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t1 + (__int128)t6 * 0x1000003D10ULL;
        t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t2 + (__int128)t7 * 0x1000003D10ULL;
        n[2] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t3 + (__int128)t8 * 0x1000003D10ULL;
        n[3] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t4 + (__int128)t9 * 0x1000003D10ULL;
        n[4] = c & 0x0FFFFFFFFFFFFULL; c = c >> 48; // c max 000001000003D110
        c = t0 + (__int128)c * 0x1000003D1ULL;
        n[0] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 1000008
        n[1] = t1 + c;
#ifdef VERIFY_BADNESS
        badness = 1;
#endif
    }

    // precondition: all values in a and b are at most 56 bits, except n[4] is at most 52 bits
    // postcondition: all values are at most 53 bits, except n[4] is at most 49 bits
    void SetSquare(const FieldElem &a) {
#ifdef VERIFY_BADNESS
        assert(a.badness <= 8);
#endif
        __int128 c = (__int128)a.n[0] * a.n[0];
        uint64_t t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0FFFFFFFFFFFFFE0
        c = c + (__int128)(a.n[0]*2) * a.n[1];
        uint64_t t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 20000000000000BF
        c = c + (__int128)(a.n[0]*2) * a.n[2] +
                (__int128)a.n[1] * a.n[1];
        uint64_t t2 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 30000000000001A0
        c = c + (__int128)(a.n[0]*2) * a.n[3] +
                (__int128)(a.n[1]*2) * a.n[2];
        uint64_t t3 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 4000000000000280
        c = c + (__int128)(a.n[0]*2) * a.n[4] +
                (__int128)(a.n[1]*2) * a.n[3] +
                (__int128)a.n[2] * a.n[2];
        uint64_t t4 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 320000000000037E
        c = c + (__int128)(a.n[1]*2) * a.n[4] +
                (__int128)(a.n[2]*2) * a.n[3];
        uint64_t t5 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 22000000000002BE
        c = c + (__int128)(a.n[2]*2) * a.n[4] +
                (__int128)a.n[3] * a.n[3];
        uint64_t t6 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 12000000000001DE
        c = c + (__int128)(a.n[3]*2) * a.n[4];
        uint64_t t7 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 02000000000000FE
        c = c + (__int128)a.n[4] * a.n[4];
        uint64_t t8 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 001000000000001E
        uint64_t t9 = c;
        c = t0 + (__int128)t5 * 0x1000003D10ULL;
        t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t1 + (__int128)t6 * 0x1000003D10ULL;
        t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t2 + (__int128)t7 * 0x1000003D10ULL;
        n[2] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t3 + (__int128)t8 * 0x1000003D10ULL;
        n[3] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t4 + (__int128)t9 * 0x1000003D10ULL;
        n[4] = c & 0x0FFFFFFFFFFFFULL; c = c >> 48; // c max 000001000003D110
        c = t0 + (__int128)c * 0x1000003D1ULL;
        n[0] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 1000008
        n[1] = t1 + c;
#ifdef VERIFY_BADNESS
        assert(a.badness <= 8);
#endif
    }


    // computes the modular inverse, by computing a^(p-2)
    // TODO: use extmod instead, much faster
    void SetInverse(const FieldElem &a) {
        // calculare a^p, with p={13,27,31}
        FieldElem a2; a2.SetSquare(a);
        FieldElem a3; a3.SetMult(a2,a);
        FieldElem a6; a6.SetSquare(a3);
        FieldElem a7; a7.SetMult(a6,a);
        FieldElem a13; a13.SetMult(a6,a7);
        FieldElem a26; a26.SetSquare(a13);
        FieldElem a27; a27.SetMult(a26,a);
        FieldElem a30; a30.SetMult(a27,a3);
        FieldElem a31; a31.SetMult(a30,a);
        FieldElem x = a;
        for (int i=0; i<44; i++) {
            for (int j=0; j<5; j++)
                x.SetSquare(x);
            x.SetMult(x,a31);
        }
        for (int j=0; j<5; j++)
            x.SetSquare(x);
        x.SetMult(x,a27);
        for (int i=0; i<4; i++) {
            for (int j=0; j<5; j++)
                x.SetSquare(x);
            x.SetMult(x,a31);
        }
        for (int j=0; j<5; j++)
            x.SetSquare(x);
        x.SetMult(x,a);
        for (int j=0; j<5; j++)
            x.SetSquare(x);
        SetMult(x,a13);
    }

    std::string ToString() {
        uint64_t tmp[4];
        Get(tmp);
        std::string ret;
        for (int i=63; i>=0; i--) {
            int val = (tmp[i/16] >> ((i%16)*4)) & 0xF;
            static const char *c = "0123456789ABCDEF";
            ret += c[val];
        }
        return ret;
    }

    void SetHex(const std::string &str) {
        uint64_t tmp[4] = {0,0,0,0};
        static const int cvt[256] = {0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 1, 2, 3, 4, 5, 6,7,8,9,0,0,0,0,0,0,
                                     0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0};
        for (int i=0; i<64; i++) {
            if (str.length() > (63-i))
                tmp[i/16] |= (uint64_t)cvt[(unsigned char)str[63-i]] << ((i%16)*4);
        }
        Set(tmp);
    }
};

template<typename F> class GroupElem {
private:
    bool fInfinity;
    F x;
    F y;
    F z;

public:
    GroupElem<F>() {
    }

    GroupElem<F>(const F &xin, const F &yin) {
        fInfinity = false;
        x = xin;
        y = yin;
        z = F(1);
    }

    bool IsValid() {
        // y^2 = x^3 + 7
        // (Y/Z^3)^2 = (X/Z^2)^3 + 7
        // Y^2 / Z^6 = X^3 / Z^6 + 7
        // Y^2 = X^3 + 7*Z^6
        F y2; y2.SetSquare(y);
        F x3; x3.SetSquare(x); x3.SetMult(x3,x);
        F z2; z2.SetSquare(z);
        F z6; z6.SetSquare(z2); z6.SetMult(z6,z2);
        z6 *= 7;
        x3 += z6;
        return y2 == x3;
    }

    void GetAffine(F &xout, F &yout) {
        z.SetInverse(z);
        F z2;
        z2.SetSquare(z);
        F z3;
        z3.SetMult(z,z2);
        x.SetMult(x,z2);
        y.SetMult(y,z3);
        z = F(1);
        xout = x;
        yout = y;
    }

    std::string ToString() {
        F xt,yt;
        if (fInfinity)
            return "(inf)";
        GetAffine(xt,yt);
        return "(" + xt.ToString() + "," + yt.ToString() + ")";
    }

    void SetDouble(const GroupElem<F> &p) {
        if (p.fInfinity || y.IsZero()) {
            fInfinity = true;
            return;
        }

        F t1,t2,t3,t4,t5;
        z.SetMult(p.y,p.z);
        z *= 2;                // Z' = 2*Y*Z (2)
        t1.SetSquare(p.x);
        t1 *= 3;               // T1 = 3*X^2 (3)
        t2.SetSquare(t1);      // T2 = 9*X^4 (1)
        t3.SetSquare(y);
        t3 *= 2;               // T3 = 2*Y^2 (2)
        t4.SetSquare(t3);
        t4 *= 2;               // T4 = 8*Y^4 (2)
        t3.SetMult(x,t3);      // T3 = 2*X*Y^2 (1)
        x = t3;
        x *= 4;                // X' = 8*X*Y^2 (4)
        x.SetNeg(x,4);         // X' = -8*X*Y^2 (5)
        x += t2;               // X' = 9*X^4 - 8*X*Y^2 (6)
        t2.SetNeg(t2,1);       // T2 = -9*X^4 (2)
        t3 *= 6;               // T3 = 12*X*Y^2 (6)
        t3 += t2;              // T3 = 12*X*Y^2 - 9*X^4 (8)
        y.SetMult(t1,t3);      // Y' = 36*X^3*Y^2 - 27*X^6 (1)
        t2.SetNeg(t4,2);       // T2 = -8*Y^4 (3)
        y += t2;               // Y' = 36*X^3*Y^2 - 27*X^6 - 8*Y^4 (4)
    }

    void SetAdd(const GroupElem<F> &p, const GroupElem<F> &q) {
        if (p.fInfinity) {
            *this = q;
            return;
        }
        if (q.fInfinity) {
            *this = p;
            return;
        }
        fInfinity = false;
        const F &x1 = p.x, &y1 = p.y, &z1 = p.z, &x2 = q.x, &y2 = q.y, &z2 = q.z;
        F z22; z22.SetSquare(z2);
        F z12; z12.SetSquare(z1);
        F u1; u1.SetMult(x1, z22);
        F u2; u2.SetMult(x2, z12);
        F s1; s1.SetMult(y1, z22); s1.SetMult(s1, z2);
        F s2; s2.SetMult(y2, z12); s2.SetMult(s2, z1);
        if (u1 == u2) {
            if (s1 == s2) {
                SetDouble(p);
            } else {
                fInfinity = true;
            }
            return;
        }
        F h; h.SetNeg(u1,1); h += u2;
        F r; r.SetNeg(s1,1); r += s2;
        F r2; r2.SetSquare(r);
        F h2; h2.SetSquare(h);
        F h3; h3.SetMult(h,h2);
        z.SetMult(p.z,q.z); z.SetMult(z, h);
        F t; t.SetMult(u1,h2);
        x = t; x *= 2; x += h3; x.SetNeg(x,3); x += r2;
        y.SetNeg(x,5); y += t; y.SetMult(y,r);
        h3.SetMult(h3,s1); h3.SetNeg(h3,1);
        y += h3;
    }
};

}

using namespace secp256k1;

int main() {
    FieldElem f1,f2;
    f1.SetHex("8b30bbe9ae2a990696b22f670709dff3727fd8bc04d3362c6c7bf458e2846004");
    f2.SetHex("a357ae915c4a65281309edf20504740f0eb3343990216b4f81063cb65f2f7e0f");
    GroupElem<FieldElem> g1(f1,f2);
    printf("%s\n", g1.ToString().c_str());
    GroupElem<FieldElem> p = g1;
    GroupElem<FieldElem> q = p;
    printf("ok %i\n", (int)p.IsValid());
    for (int i=0; i<100000000; i++) {
      p.SetAdd(p,g1);
      q.SetAdd(q,p);
    }
    printf("ok %i\n", (int)q.IsValid());
    printf("%s\n", q.ToString().c_str());
    return 0;
}
