#ifndef _SECP256K1_FIELD_
#define _SECP256K1_FIELD_

using namespace std;

#include <assert.h>
#include <stdint.h>
#include <string>

#include "num.h"

// #define VERIFY_MAGNITUDE 1

namespace secp256k1 {

/** Implements arithmetic modulo FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F,
 *  represented as 5 uint64_t's in base 2^52. The values are allowed to contain >52 each. In particular,
 *  each FieldElem has a 'magnitude' associated with it. Internally, a magnitude M means each element
 *  is at most M*(2^53-1), except the most significant one, which is limited to M*(2^49-1). All operations
 *  accept any input with magnitude at most M, and have different rules for propagating magnitude to their
 *  output.
 */
class FieldElem {
private:
    // X = sum(i=0..4, elem[i]*2^52)
    uint64_t n[5];
#ifdef VERIFY_MAGNITUDE
    int magnitude;
#endif

public:

    /** Creates a constant field element. Magnitude=1 */
    FieldElem(int x = 0) {
        n[0] = x;
        n[1] = n[2] = n[3] = n[4] = 0;
#ifdef VERIFY_MAGNITUDE
        magnitude = 1;
#endif
    }

    FieldElem(const unsigned char *b32) {
        SetBytes(b32);
    }

    /** Normalizes the internal representation entries. Magnitude=1 */
    void Normalize() {
        uint64_t c;
        if (n[0] > 0xFFFFFFFFFFFFFULL || n[1] > 0xFFFFFFFFFFFFFULL || n[2] > 0xFFFFFFFFFFFFFULL || n[3] > 0xFFFFFFFFFFFFFULL || n[4] > 0xFFFFFFFFFFFFULL) {
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
            n[0] = t0; n[1] = t1; n[2] = t2; n[3] = t3; n[4] = t4;
        }
        if (n[4] == 0xFFFFFFFFFFFFULL && n[3] == 0xFFFFFFFFFFFFFULL && n[2] == 0xFFFFFFFFFFFFFULL && n[1] == 0xFFFFFFFFFFFFF && n[0] >= 0xFFFFEFFFFFC2FULL) {
            n[4] = 0;
            n[3] = 0;
            n[2] = 0;
            n[1] = 0;
            n[0] -= 0xFFFFEFFFFFC2FULL;
        }
#ifdef VERIFY_MAGNITUDE
        magnitude = 1;
#endif
    }

    bool IsZero() {
        Normalize();
        return (n[0] == 0 && n[1] == 0 && n[2] == 0 && n[3] == 0 && n[4] == 0);
    }

    bool friend operator==(FieldElem &a, FieldElem &b) {
        a.Normalize();
        b.Normalize();
        return (a.n[0] == b.n[0] && a.n[1] == b.n[1] && a.n[2] == b.n[2] && a.n[3] == b.n[3] && a.n[4] == b.n[4]);
    }

    /** extract as 32-byte big endian array */
    void GetBytes(unsigned char *o) {
        Normalize();
        for (int i=0; i<32; i++) {
            int c = 0;
            for (int j=0; j<2; j++) {
                int limb = (8*i+4*j)/52;
                int shift = (8*i+4*j)%52;
                c |= ((n[limb] >> shift) & 0xF) << (4 * j);
            }
            o[31-i] = c;
        }
    }

    /** set value of 32-byte big endian array */
    void SetBytes(const unsigned char *in) {
        n[0] = n[1] = n[2] = n[3] = n[4] = 0;
        for (int i=0; i<32; i++) {
            for (int j=0; j<2; j++) {
                int limb = (8*i+4*j)/52;
                int shift = (8*i+4*j)%52;
                n[limb] |= (uint64_t)((in[31-i] >> (4*j)) & 0xF) << shift;
            }
        }
#ifdef VERIFY_MAGNITUDE
        magnitude = 1;
#endif
    }

    /** Set a FieldElem to be the negative of another. Increases magnitude by one. */
    void SetNeg(const FieldElem &a, int magnitudeIn) {
#ifdef VERIFY_MAGNITUDE
        assert(a.magnitude <= magnitudeIn);
        magnitude = magnitudeIn + 1;
#endif
        n[0] = 0xFFFFEFFFFFC2FULL * (magnitudeIn + 1) - a.n[0];
        n[1] = 0xFFFFFFFFFFFFFULL * (magnitudeIn + 1) - a.n[1];
        n[2] = 0xFFFFFFFFFFFFFULL * (magnitudeIn + 1) - a.n[2];
        n[3] = 0xFFFFFFFFFFFFFULL * (magnitudeIn + 1) - a.n[3];
        n[4] = 0x0FFFFFFFFFFFFULL * (magnitudeIn + 1) - a.n[4];
    }

    /** Multiplies this FieldElem with an integer constant. Magnitude is multiplied by v */
    void operator*=(int v) {
#ifdef VERIFY_MAGNITUDE
        magnitude *= v;
#endif
        n[0] *= v;
        n[1] *= v;
        n[2] *= v;
        n[3] *= v;
        n[4] *= v;
    }

    void operator+=(const FieldElem &a) {
#ifdef VERIFY_MAGNITUDE
        magnitude += a.magnitude;
#endif
        n[0] += a.n[0];
        n[1] += a.n[1];
        n[2] += a.n[2];
        n[3] += a.n[3];
        n[4] += a.n[4];
    }

    /** Set this FieldElem to be the multiplication of two others. Magnitude=1 */
    void SetMult(const FieldElem &a, const FieldElem &b) {
#ifdef VERIFY_MAGNITUDE
        assert(a.magnitude <= 8);
        assert(b.magnitude <= 8);
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
#ifdef VERIFY_MAGNITUDE
        magnitude = 1;
#endif
    }

    /** Set this FieldElem to be the square of another. Magnitude=1 */
    void SetSquare(const FieldElem &a) {
#ifdef VERIFY_MAGNITUDE
        assert(a.magnitude <= 8);
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
#ifdef VERIFY_MAGNITUDE
        assert(a.magnitude <= 8);
#endif
    }

    /** Set this to be the (modular) square root of another FieldElem. Magnitude=1 */
    void SetSquareRoot(const FieldElem &a) {
        // calculate a^p, with p={15,780,1022,1023}
        FieldElem a2; a2.SetSquare(a);
        FieldElem a3; a3.SetMult(a2,a);
        FieldElem a6; a6.SetSquare(a3);
        FieldElem a12; a12.SetSquare(a6);
        FieldElem a15; a15.SetMult(a12,a3);
        FieldElem a30; a30.SetSquare(a15);
        FieldElem a60; a60.SetSquare(a30);
        FieldElem a120; a120.SetSquare(a60);
        FieldElem a240; a240.SetSquare(a120);
        FieldElem a255; a255.SetMult(a240,a15);
        FieldElem a510; a510.SetSquare(a255);
        FieldElem a750; a750.SetMult(a510,a240);
        FieldElem a780; a780.SetMult(a750,a30);
        FieldElem a1020; a1020.SetSquare(a510);
        FieldElem a1022; a1022.SetMult(a1020,a2);
        FieldElem a1023; a1023.SetMult(a1022,a);
        FieldElem x = a15;
        for (int i=0; i<21; i++) {
            for (int j=0; j<10; j++) x.SetSquare(x);
            x.SetMult(x,a1023);
        }
        for (int j=0; j<10; j++) x.SetSquare(x);
        x.SetMult(x,a1022);
        for (int i=0; i<2; i++) {
            for (int j=0; j<10; j++) x.SetSquare(x);
            x.SetMult(x,a1023);
        }
        for (int j=0; j<10; j++) x.SetSquare(x);
        SetMult(x,a780);
    }

    bool IsOdd() {
        Normalize();
        return n[0] & 1;
    }

    /** Set this to be the (modular) inverse of another FieldElem. Magnitude=1 */
    void SetInverse(Context &ctx, const FieldElem &a);

    std::string ToString() {
        unsigned char tmp[32];
        GetBytes(tmp);
        std::string ret;
        for (int i=0; i<32; i++) {
            static const char *c = "0123456789ABCDEF";
            ret += c[(tmp[i] >> 4) & 0xF];
            ret += c[(tmp[i]) & 0xF];
        }
        return ret;
    }

    void SetHex(const std::string &str) {
        unsigned char tmp[32] = {};
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
        for (unsigned int i=0; i<32; i++) {
            if (str.length() > i*2)
                tmp[32 - str.length()/2 + i] = (cvt[(unsigned char)str[2*i]] << 4) + cvt[(unsigned char)str[2*i+1]];
        }
        SetBytes(tmp);
    }
};

static const unsigned char field_p_[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                                         0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                                         0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                                         0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFC,0x2F};

class FieldConstants {
private:
    Context ctx;

public:
    const Number field_p;

    FieldConstants() : field_p(ctx, field_p_, sizeof(field_p_)) {}
};

const FieldConstants &GetFieldConst() {
    static const FieldConstants field_const;
    return field_const;
}

void FieldElem::SetInverse(Context &ctx, const FieldElem &a) {
#if 0
    // calculate a^p, with p={45,63,1019,1023}
    FieldElem a2; a2.SetSquare(a);
    FieldElem a3; a3.SetMult(a2,a);
    FieldElem a4; a4.SetSquare(a2);
    FieldElem a5; a5.SetMult(a4,a);
    FieldElem a10; a10.SetSquare(a5);
    FieldElem a11; a11.SetMult(a10,a);
    FieldElem a21; a21.SetMult(a11,a10);
    FieldElem a42; a42.SetSquare(a21);
    FieldElem a45; a45.SetMult(a42,a3);
    FieldElem a63; a63.SetMult(a42,a21);
    FieldElem a126; a126.SetSquare(a63);
    FieldElem a252; a252.SetSquare(a126);
    FieldElem a504; a504.SetSquare(a252);
    FieldElem a1008; a1008.SetSquare(a504);
    FieldElem a1019; a1019.SetMult(a1008,a11);
    FieldElem a1023; a1023.SetMult(a1019,a4);
    FieldElem x = a63;
    for (int i=0; i<21; i++) {
        for (int j=0; j<10; j++) x.SetSquare(x);
        x.SetMult(x,a1023);
    }
    for (int j=0; j<10; j++) x.SetSquare(x);
    x.SetMult(x,a1019);
    for (int i=0; i<2; i++) {
        for (int j=0; j<10; j++) x.SetSquare(x);
        x.SetMult(x,a1023);
    }
    for (int j=0; j<10; j++) x.SetSquare(x);
    SetMult(x,a45);
#else
    unsigned char b[32];
    GetBytes(b);
    {
        const Number &p = GetFieldConst().field_p;
        Context ct(ctx);
        Number n(ct); n.SetBytes(b, 32);
        n.SetModInverse(ct, n, p);
        n.GetBytes(b, 32);
    }
    SetBytes(b);
#endif
}

}

#endif
