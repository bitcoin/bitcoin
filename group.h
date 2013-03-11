#ifndef _SECP256K1_GROUP_
#define _SECP256K1_GROUP_

#include "field.h"

namespace secp256k1 {

class GroupElemJac;

/** Defines a point on the secp256k1 curve (y^2 = x^3 + 7) */
class GroupElem {
protected:
    bool fInfinity;
    FieldElem x;
    FieldElem y;

public:

    /** Creates the point at infinity */
    GroupElem() {
        fInfinity = true;
    }

    /** Creates the point with given affine coordinates */
    GroupElem(const FieldElem &xin, const FieldElem &yin) {
        fInfinity = false;
        x = xin;
        y = yin;
    }

    /** Checks whether this is the point at infinity */
    bool IsInfinity() const {
        return fInfinity;
    }

    void SetNeg(const GroupElem &p) {
        *this = p;
        y.Normalize();
        y.SetNeg(y, 1);
    }

    void GetX(Context &ctx, FieldElem &xout) {
        xout = x;
    }

    void GetY(Context &ctx, FieldElem &yout) {
        yout = y;
    }

    std::string ToString() const {
        if (fInfinity)
            return "(inf)";
        FieldElem xc = x, yc = y;
        return "(" + xc.ToString() + "," + yc.ToString() + ")";
    }

    void SetJac(Context &ctx, GroupElemJac &jac);

    friend class GroupElemJac;
};

/** Represents a point on the secp256k1 curve, with jacobian coordinates */
class GroupElemJac : private GroupElem {
protected:
    FieldElem z;

public:
    /** Creates the point at infinity */
    GroupElemJac() : GroupElem(), z(1) {}

    /** Creates the point with given affine coordinates */
    GroupElemJac(const FieldElem &xin, const FieldElem &yin) : GroupElem(xin,yin), z(1) {}

    GroupElemJac(const GroupElem &in) : GroupElem(in), z(1) {}

    void SetJac(Context &ctx, GroupElemJac &jac) {
        *this = jac;
    }

    /** Checks whether this is a non-infinite point on the curve */
    bool IsValid() const {
        if (IsInfinity())
            return false;
        // y^2 = x^3 + 7
        // (Y/Z^3)^2 = (X/Z^2)^3 + 7
        // Y^2 / Z^6 = X^3 / Z^6 + 7
        // Y^2 = X^3 + 7*Z^6
        FieldElem y2; y2.SetSquare(y);
        FieldElem x3; x3.SetSquare(x); x3.SetMult(x3,x);
        FieldElem z2; z2.SetSquare(z);
        FieldElem z6; z6.SetSquare(z2); z6.SetMult(z6,z2);
        z6 *= 7;
        x3 += z6;
        return y2 == x3;
    }

    /** Returns the affine coordinates of this point */
    void GetAffine(Context &ctx, GroupElem &aff) {
        z.SetInverse(ctx, z);
        FieldElem z2;
        z2.SetSquare(z);
        FieldElem z3;
        z3.SetMult(z,z2);
        x.SetMult(x,z2);
        y.SetMult(y,z3);
        z = FieldElem(1);
        aff.fInfinity = fInfinity;
        aff.x = x;
        aff.y = y;
    }

    void GetX(Context &ctx, FieldElem &xout) {
        FieldElem zi;
        zi.SetInverse(ctx, z);
        zi.SetSquare(zi);
        xout.SetMult(x, zi);
    }

    bool IsInfinity() const {
        return fInfinity;
    }

    void GetY(Context &ctx, FieldElem &yout) {
        FieldElem zi;
        zi.SetInverse(ctx, z);
        FieldElem zi3; zi3.SetSquare(zi); zi3.SetMult(zi, zi3);
        yout.SetMult(y, zi3);
    }

    void SetNeg(const GroupElemJac &p) {
        *this = p;
        y.Normalize();
        y.SetNeg(y, 1);
    }

    /** Sets this point to have a given X coordinate & given Y oddness */
    void SetCompressed(const FieldElem &xin, bool fOdd) {
        x = xin;
        FieldElem x2; x2.SetSquare(x);
        FieldElem x3; x3.SetMult(x,x2);
        fInfinity = false;
        FieldElem c(7);
        c += x3;
        y.SetSquareRoot(c);
        z = FieldElem(1);
        if (y.IsOdd() != fOdd)
            y.SetNeg(y,1);
    }

    /** Sets this point to be the EC double of another */
    void SetDouble(const GroupElemJac &p) {
        FieldElem t5 = p.y;
        if (p.fInfinity || t5.IsZero()) {
            fInfinity = true;
            return;
        }

        FieldElem t1,t2,t3,t4;
        z.SetMult(t5,p.z);
        z *= 2;                // Z' = 2*Y*Z (2)
        t1.SetSquare(p.x);
        t1 *= 3;               // T1 = 3*X^2 (3)
        t2.SetSquare(t1);      // T2 = 9*X^4 (1)
        t3.SetSquare(t5);
        t3 *= 2;               // T3 = 2*Y^2 (2)
        t4.SetSquare(t3);
        t4 *= 2;               // T4 = 8*Y^4 (2)
        t3.SetMult(p.x,t3);      // T3 = 2*X*Y^2 (1)
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
        fInfinity = false;
    }

    /** Sets this point to be the EC addition of two others */
    void SetAdd(const GroupElemJac &p, const GroupElemJac &q) {
        if (p.fInfinity) {
            *this = q;
            return;
        }
        if (q.fInfinity) {
            *this = p;
            return;
        }
        fInfinity = false;
        const FieldElem &x1 = p.x, &y1 = p.y, &z1 = p.z, &x2 = q.x, &y2 = q.y, &z2 = q.z;
        FieldElem z22; z22.SetSquare(z2);
        FieldElem z12; z12.SetSquare(z1);
        FieldElem u1; u1.SetMult(x1, z22);
        FieldElem u2; u2.SetMult(x2, z12);
        FieldElem s1; s1.SetMult(y1, z22); s1.SetMult(s1, z2);
        FieldElem s2; s2.SetMult(y2, z12); s2.SetMult(s2, z1);
        if (u1 == u2) {
            if (s1 == s2) {
                SetDouble(p);
            } else {
                fInfinity = true;
            }
            return;
        }
        FieldElem h; h.SetNeg(u1,1); h += u2;
        FieldElem r; r.SetNeg(s1,1); r += s2;
        FieldElem r2; r2.SetSquare(r);
        FieldElem h2; h2.SetSquare(h);
        FieldElem h3; h3.SetMult(h,h2);
        z.SetMult(z1,z2); z.SetMult(z, h);
        FieldElem t; t.SetMult(u1,h2);
        x = t; x *= 2; x += h3; x.SetNeg(x,3); x += r2;
        y.SetNeg(x,5); y += t; y.SetMult(y,r);
        h3.SetMult(h3,s1); h3.SetNeg(h3,1);
        y += h3;
    }

    /** Sets this point to be the EC addition of two others (one of which is in affine coordinates) */
    void SetAdd(const GroupElemJac &p, const GroupElem &q) {
        if (p.fInfinity) {
            x = q.x;
            y = q.y;
            fInfinity = q.fInfinity;
            z = FieldElem(1);
            return;
        }
        if (q.fInfinity) {
            *this = p;
            return;
        }
        fInfinity = false;
        const FieldElem &x1 = p.x, &y1 = p.y, &z1 = p.z, &x2 = q.x, &y2 = q.y;
        FieldElem z12; z12.SetSquare(z1);
        FieldElem u1 = x1; u1.Normalize();
        FieldElem u2; u2.SetMult(x2, z12);
        FieldElem s1 = y1; s1.Normalize();
        FieldElem s2; s2.SetMult(y2, z12); s2.SetMult(s2, z1);
        if (u1 == u2) {
            if (s1 == s2) {
                SetDouble(p);
            } else {
                fInfinity = true;
            }
            return;
        }
        FieldElem h; h.SetNeg(u1,1); h += u2;
        FieldElem r; r.SetNeg(s1,1); r += s2;
        FieldElem r2; r2.SetSquare(r);
        FieldElem h2; h2.SetSquare(h);
        FieldElem h3; h3.SetMult(h,h2);
        z = p.z; z.SetMult(z, h);
        FieldElem t; t.SetMult(u1,h2);
        x = t; x *= 2; x += h3; x.SetNeg(x,3); x += r2;
        y.SetNeg(x,5); y += t; y.SetMult(y,r);
        h3.SetMult(h3,s1); h3.SetNeg(h3,1);
        y += h3;
    }

    std::string ToString() const {
        Context ctx;
        GroupElemJac cop = *this;
        GroupElem aff;
        cop.GetAffine(ctx, aff);
        return aff.ToString();
    }

    void SetMulLambda(const GroupElemJac &p);
};

void GroupElem::SetJac(Context &ctx, GroupElemJac &jac) {
    jac.GetAffine(ctx, *this);
}

static const unsigned char order_[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                                       0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
                                       0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
                                       0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x41};

static const unsigned char g_x_[] = {0x79,0xBE,0x66,0x7E,0xF9,0xDC,0xBB,0xAC,
                                     0x55,0xA0,0x62,0x95,0xCE,0x87,0x0B,0x07,
                                     0x02,0x9B,0xFC,0xDB,0x2D,0xCE,0x28,0xD9,
                                     0x59,0xF2,0x81,0x5B,0x16,0xF8,0x17,0x98};

static const unsigned char g_y_[] = {0x48,0x3A,0xDA,0x77,0x26,0xA3,0xC4,0x65,
                                     0x5D,0xA4,0xFB,0xFC,0x0E,0x11,0x08,0xA8,
                                     0xFD,0x17,0xB4,0x48,0xA6,0x85,0x54,0x19,
                                     0x9C,0x47,0xD0,0x8F,0xFB,0x10,0xD4,0xB8};

// properties of secp256k1's efficiently computable endomorphism
static const unsigned char lambda_[] = {0x53,0x63,0xad,0x4c,0xc0,0x5c,0x30,0xe0,
                                        0xa5,0x26,0x1c,0x02,0x88,0x12,0x64,0x5a,
                                        0x12,0x2e,0x22,0xea,0x20,0x81,0x66,0x78,
                                        0xdf,0x02,0x96,0x7c,0x1b,0x23,0xbd,0x72};
static const unsigned char beta_[] =   {0x7a,0xe9,0x6a,0x2b,0x65,0x7c,0x07,0x10,
                                        0x6e,0x64,0x47,0x9e,0xac,0x34,0x34,0xe9,
                                        0x9c,0xf0,0x49,0x75,0x12,0xf5,0x89,0x95,
                                        0xc1,0x39,0x6c,0x28,0x71,0x95,0x01,0xee};
static const unsigned char a1b2_[] =   {0x30,0x86,0xd2,0x21,0xa7,0xd4,0x6b,0xcd,
                                        0xe8,0x6c,0x90,0xe4,0x92,0x84,0xeb,0x15};
static const unsigned char b1_[]   =   {0xe4,0x43,0x7e,0xd6,0x01,0x0e,0x88,0x28,
                                        0x6f,0x54,0x7f,0xa9,0x0a,0xbf,0xe4,0xc3};
static const unsigned char a2_[]   =   {0x01,
                                        0x14,0xca,0x50,0xf7,0xa8,0xe2,0xf3,0xf6,
                                        0x57,0xc1,0x10,0x8d,0x9d,0x44,0xcf,0xd8};
class GroupConstants {
private:
    Context ctx;
    const FieldElem g_x;
    const FieldElem g_y;

public:
    const Number order;
    const GroupElem g;
    const FieldElem beta;
    const Number lambda, a1b2, b1, a2;

    GroupConstants() : g_x(g_x_), g_y(g_y_),
                       order(ctx, order_, sizeof(order_)),
                       g(g_x,g_y),
                       beta(beta_),
                       lambda(ctx, lambda_, sizeof(lambda_)),
                       a1b2(ctx, a1b2_, sizeof(a1b2_)),
                       b1(ctx, b1_, sizeof(b1_)),
                       a2(ctx, a2_, sizeof(a2_)) {}
};

const GroupConstants &GetGroupConst() {
    static const GroupConstants group_const;
    return group_const;
}

void GroupElemJac::SetMulLambda(const GroupElemJac &p) {
    FieldElem beta = GetGroupConst().beta;
    *this = p;
    x.SetMult(x, beta);
}

void SplitExp(Context &ctx, const Number &exp, Number &exp1, Number &exp2) {
    const GroupConstants &c = GetGroupConst();
    Context ct(ctx);
    Number bnc1(ct), bnc2(ct), bnt1(ct), bnt2(ct), bnn2(ct);
    bnn2.SetNumber(c.order);
    bnn2.Shift1();

    bnc1.SetMult(ct, exp, c.a1b2);
    bnc1.SetAdd(ct, bnc1, bnn2);
    bnc1.SetDiv(ct, bnc1, c.order);

    bnc2.SetMult(ct, exp, c.b1);
    bnc2.SetAdd(ct, bnc2, bnn2);
    bnc2.SetDiv(ct, bnc2, c.order);

    bnt1.SetMult(ct, bnc1, c.a1b2);
    bnt2.SetMult(ct, bnc2, c.a2);
    bnt1.SetAdd(ct, bnt1, bnt2);
    exp1.SetSub(ct, exp, bnt1);
    bnt1.SetMult(ct, bnc1, c.b1);
    bnt2.SetMult(ct, bnc2, c.a1b2);
    exp2.SetSub(ct, bnt1, bnt2);
}

}

#endif
