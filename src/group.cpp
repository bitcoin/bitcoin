#include <string>

#include "num.h"
#include "field.h"
#include "group.h"

namespace secp256k1 {

GroupElem::GroupElem() {
    fInfinity = true;
}

GroupElem::GroupElem(const secp256k1_fe_t &xin, const secp256k1_fe_t &yin) {
    fInfinity = false;
    x = xin;
    y = yin;
}

bool GroupElem::IsInfinity() const {
    return fInfinity;
}

void GroupElem::SetNeg(const GroupElem &p) {
    *this = p;
    secp256k1_fe_normalize(&y);
    secp256k1_fe_negate(&y, &y, 1);
}

void GroupElem::GetX(secp256k1_fe_t &xout) {
    xout = x;
}

void GroupElem::GetY(secp256k1_fe_t &yout) {
    yout = y;
}

std::string GroupElem::ToString() const {
    if (fInfinity)
        return "(inf)";
    secp256k1_fe_t xc = x, yc = y;
    char xo[65], yo[65];
    int xl = 65, yl = 65;
    secp256k1_fe_get_hex(xo, &xl, &xc);
    secp256k1_fe_get_hex(yo, &yl, &yc);
    return "(" + std::string(xo) + "," + std::string(yo) + ")";
}

GroupElemJac::GroupElemJac() : GroupElem() {
    secp256k1_fe_set_int(&z, 1);
}

GroupElemJac::GroupElemJac(const secp256k1_fe_t &xin, const secp256k1_fe_t &yin) : GroupElem(xin,yin) {
    secp256k1_fe_set_int(&z, 1);
}

GroupElemJac::GroupElemJac(const GroupElem &in) : GroupElem(in) {
    secp256k1_fe_set_int(&z, 1);
}

void GroupElemJac::SetJac(const GroupElemJac &jac) {
    *this = jac;
}

void GroupElemJac::SetAffine(const GroupElem &aff) {
    fInfinity = aff.fInfinity;
    x = aff.x;
    y = aff.y;
    secp256k1_fe_set_int(&z, 1);
}

bool GroupElemJac::IsValid() const {
    if (IsInfinity())
        return false;
    // y^2 = x^3 + 7
    // (Y/Z^3)^2 = (X/Z^2)^3 + 7
    // Y^2 / Z^6 = X^3 / Z^6 + 7
    // Y^2 = X^3 + 7*Z^6
    secp256k1_fe_t y2; secp256k1_fe_sqr(&y2, &y);
    secp256k1_fe_t x3; secp256k1_fe_sqr(&x3, &x); secp256k1_fe_mul(&x3, &x3, &x);
    secp256k1_fe_t z2; secp256k1_fe_sqr(&z2, &z);
    secp256k1_fe_t z6; secp256k1_fe_sqr(&z6, &z2); secp256k1_fe_mul(&z6, &z6, &z2);
    secp256k1_fe_mul_int(&z6, 7);
    secp256k1_fe_add(&x3, &z6);
    secp256k1_fe_normalize(&y2);
    secp256k1_fe_normalize(&x3);
    return secp256k1_fe_equal(&y2, &x3);
}

void GroupElemJac::GetAffine(GroupElem &aff) {
    secp256k1_fe_inv_var(&z, &z);
    secp256k1_fe_t z2; secp256k1_fe_sqr(&z2, &z);
    secp256k1_fe_t z3; secp256k1_fe_mul(&z3, &z, &z2);
    secp256k1_fe_mul(&x, &x, &z2);
    secp256k1_fe_mul(&y, &y, &z3);
    secp256k1_fe_set_int(&z, 1);
    aff.fInfinity = fInfinity;
    aff.x = x;
    aff.y = y;
}

void GroupElemJac::GetX(secp256k1_fe_t &xout) {
    secp256k1_fe_t zi2; secp256k1_fe_inv_var(&zi2, &z); secp256k1_fe_sqr(&zi2, &zi2);
    secp256k1_fe_mul(&xout, &x, &zi2);
}

void GroupElemJac::GetY(secp256k1_fe_t &yout) {
    secp256k1_fe_t zi; secp256k1_fe_inv_var(&zi, &z); 
    secp256k1_fe_t zi3; secp256k1_fe_sqr(&zi3, &zi); secp256k1_fe_mul(&zi3, &zi, &zi3);
    secp256k1_fe_mul(&yout, &y, &zi3);
}

bool GroupElemJac::IsInfinity() const {
    return fInfinity;
}


void GroupElemJac::SetNeg(const GroupElemJac &p) {
    *this = p;
    secp256k1_fe_normalize(&y);
    secp256k1_fe_negate(&y, &y, 1);
}

void GroupElemJac::SetCompressed(const secp256k1_fe_t &xin, bool fOdd) {
    x = xin;
    secp256k1_fe_t x2; secp256k1_fe_sqr(&x2, &x);
    secp256k1_fe_t x3; secp256k1_fe_mul(&x3, &x, &x2);
    fInfinity = false;
    secp256k1_fe_t c; secp256k1_fe_set_int(&c, 7);
    secp256k1_fe_add(&c, &x3);
    secp256k1_fe_sqrt(&y, &c);
    secp256k1_fe_set_int(&z, 1);
    secp256k1_fe_normalize(&y);
    if (secp256k1_fe_is_odd(&y) != fOdd)
        secp256k1_fe_negate(&y, &y, 1);
}

void GroupElemJac::SetDouble(const GroupElemJac &p) {
    secp256k1_fe_t t5 = p.y;
    secp256k1_fe_normalize(&t5);
    if (p.fInfinity || secp256k1_fe_is_zero(&t5)) {
        fInfinity = true;
        return;
    }

    secp256k1_fe_t t1,t2,t3,t4;
    secp256k1_fe_mul(&z, &t5, &p.z);
    secp256k1_fe_mul_int(&z, 2);      // Z' = 2*Y*Z (2)
    secp256k1_fe_sqr(&t1, &p.x);
    secp256k1_fe_mul_int(&t1, 3);     // T1 = 3*X^2 (3)
    secp256k1_fe_sqr(&t2, &t1);       // T2 = 9*X^4 (1)
    secp256k1_fe_sqr(&t3, &t5);
    secp256k1_fe_mul_int(&t3, 2);     // T3 = 2*Y^2 (2)
    secp256k1_fe_sqr(&t4, &t3);
    secp256k1_fe_mul_int(&t4, 2);     // T4 = 8*Y^4 (2)
    secp256k1_fe_mul(&t3, &p.x, &t3); // T3 = 2*X*Y^2 (1)
    x = t3;
    secp256k1_fe_mul_int(&x, 4);      // X' = 8*X*Y^2 (4)
    secp256k1_fe_negate(&x, &x, 4);   // X' = -8*X*Y^2 (5)
    secp256k1_fe_add(&x, &t2);        // X' = 9*X^4 - 8*X*Y^2 (6)
    secp256k1_fe_negate(&t2, &t2, 1); // T2 = -9*X^4 (2)
    secp256k1_fe_mul_int(&t3, 6);     // T3 = 12*X*Y^2 (6)
    secp256k1_fe_add(&t3, &t2);       // T3 = 12*X*Y^2 - 9*X^4 (8)
    secp256k1_fe_mul(&y, &t1, &t3);   // Y' = 36*X^3*Y^2 - 27*X^6 (1)
    secp256k1_fe_negate(&t2, &t4, 2); // T2 = -8*Y^4 (3)
    secp256k1_fe_add(&y, &t2);        // Y' = 36*X^3*Y^2 - 27*X^6 - 8*Y^4 (4)
    fInfinity = false;
}

void GroupElemJac::SetAdd(const GroupElemJac &p, const GroupElemJac &q) {
    if (p.fInfinity) {
        *this = q;
        return;
    }
    if (q.fInfinity) {
        *this = p;
        return;
    }
    fInfinity = false;
    const secp256k1_fe_t &x1 = p.x, &y1 = p.y, &z1 = p.z, &x2 = q.x, &y2 = q.y, &z2 = q.z;
    secp256k1_fe_t z22; secp256k1_fe_sqr(&z22, &z2);
    secp256k1_fe_t z12; secp256k1_fe_sqr(&z12, &z1);
    secp256k1_fe_t u1; secp256k1_fe_mul(&u1, &x1, &z22);
    secp256k1_fe_t u2; secp256k1_fe_mul(&u2, &x2, &z12);
    secp256k1_fe_t s1; secp256k1_fe_mul(&s1, &y1, &z22); secp256k1_fe_mul(&s1, &s1, &z2);
    secp256k1_fe_t s2; secp256k1_fe_mul(&s2, &y2, &z12); secp256k1_fe_mul(&s2, &s2, &z1);
    secp256k1_fe_normalize(&u1);
    secp256k1_fe_normalize(&u2);
    if (secp256k1_fe_equal(&u1, &u2)) {
        secp256k1_fe_normalize(&s1);
        secp256k1_fe_normalize(&s2);
        if (secp256k1_fe_equal(&s1, &s2)) {
            SetDouble(p);
        } else {
            fInfinity = true;
        }
        return;
    }
    secp256k1_fe_t h; secp256k1_fe_negate(&h, &u1, 1); secp256k1_fe_add(&h, &u2);
    secp256k1_fe_t r; secp256k1_fe_negate(&r, &s1, 1); secp256k1_fe_add(&r, &s2);
    secp256k1_fe_t r2; secp256k1_fe_sqr(&r2, &r);
    secp256k1_fe_t h2; secp256k1_fe_sqr(&h2, &h);
    secp256k1_fe_t h3; secp256k1_fe_mul(&h3, &h, &h2);
    secp256k1_fe_mul(&z, &z1, &z2); secp256k1_fe_mul(&z, &z, &h);
    secp256k1_fe_t t; secp256k1_fe_mul(&t, &u1, &h2);
    x = t; secp256k1_fe_mul_int(&x, 2); secp256k1_fe_add(&x, &h3); secp256k1_fe_negate(&x, &x, 3); secp256k1_fe_add(&x, &r2);
    secp256k1_fe_negate(&y, &x, 5); secp256k1_fe_add(&y, &t); secp256k1_fe_mul(&y, &y, &r);
    secp256k1_fe_mul(&h3, &h3, &s1); secp256k1_fe_negate(&h3, &h3, 1);
    secp256k1_fe_add(&y, &h3);
}

void GroupElemJac::SetAdd(const GroupElemJac &p, const GroupElem &q) {
    if (p.fInfinity) {
        x = q.x;
        y = q.y;
        fInfinity = q.fInfinity;
        secp256k1_fe_set_int(&z, 1);
        return;
    }
    if (q.fInfinity) {
        *this = p;
        return;
    }
    fInfinity = false;
    const secp256k1_fe_t &x1 = p.x, &y1 = p.y, &z1 = p.z, &x2 = q.x, &y2 = q.y;
    secp256k1_fe_t z12; secp256k1_fe_sqr(&z12, &z1);
    secp256k1_fe_t u1 = x1; secp256k1_fe_normalize(&u1);
    secp256k1_fe_t u2; secp256k1_fe_mul(&u2, &x2, &z12);
    secp256k1_fe_t s1 = y1; secp256k1_fe_normalize(&s1); 
    secp256k1_fe_t s2; secp256k1_fe_mul(&s2, &y2, &z12); secp256k1_fe_mul(&s2, &s2, &z1);
    secp256k1_fe_normalize(&u1);
    secp256k1_fe_normalize(&u2);
    if (secp256k1_fe_equal(&u1, &u2)) {
        secp256k1_fe_normalize(&s1);
        secp256k1_fe_normalize(&s2);
        if (secp256k1_fe_equal(&s1, &s2)) {
            SetDouble(p);
        } else {
            fInfinity = true;
        }
        return;
    }
    secp256k1_fe_t h; secp256k1_fe_negate(&h, &u1, 1); secp256k1_fe_add(&h, &u2);
    secp256k1_fe_t r; secp256k1_fe_negate(&r, &s1, 1); secp256k1_fe_add(&r, &s2);
    secp256k1_fe_t r2; secp256k1_fe_sqr(&r2, &r);
    secp256k1_fe_t h2; secp256k1_fe_sqr(&h2, &h);
    secp256k1_fe_t h3; secp256k1_fe_mul(&h3, &h, &h2);
    z = p.z; secp256k1_fe_mul(&z, &z, &h);
    secp256k1_fe_t t; secp256k1_fe_mul(&t, &u1, &h2);
    x = t; secp256k1_fe_mul_int(&x, 2); secp256k1_fe_add(&x, &h3); secp256k1_fe_negate(&x, &x, 3); secp256k1_fe_add(&x, &r2);
    secp256k1_fe_negate(&y, &x, 5); secp256k1_fe_add(&y, &t); secp256k1_fe_mul(&y, &y, &r);
    secp256k1_fe_mul(&h3, &h3, &s1); secp256k1_fe_negate(&h3, &h3, 1);
    secp256k1_fe_add(&y, &h3);
}

std::string GroupElemJac::ToString() const {
    GroupElemJac cop = *this;
    GroupElem aff;
    cop.GetAffine(aff);
    return aff.ToString();
}

void GroupElem::SetJac(GroupElemJac &jac) {
    jac.GetAffine(*this);
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

GroupConstants::GroupConstants() {
    secp256k1_num_init(&order);
    secp256k1_num_init(&lambda);
    secp256k1_num_init(&a1b2);
    secp256k1_num_init(&b1);
    secp256k1_num_init(&a2);

    secp256k1_fe_set_b32(&g_x, g_x_);
    secp256k1_fe_set_b32(&g_y, g_y_);
    secp256k1_fe_set_b32(&beta, beta_);

    g = GroupElem(g_x, g_y);

    secp256k1_num_set_bin(&order, order_, sizeof(order_));
    secp256k1_num_set_bin(&lambda, lambda_, sizeof(lambda_));
    secp256k1_num_set_bin(&a1b2, a1b2_, sizeof(a1b2_));
    secp256k1_num_set_bin(&b1, b1_, sizeof(b1_));
    secp256k1_num_set_bin(&a2, a2_, sizeof(a2_));
}

GroupConstants::~GroupConstants() {
    secp256k1_num_free(&order);
    secp256k1_num_free(&lambda);
    secp256k1_num_free(&a1b2);
    secp256k1_num_free(&b1);
    secp256k1_num_free(&a2);
}

const GroupConstants &GetGroupConst() {
    static const GroupConstants group_const;
    return group_const;
}

void GroupElemJac::SetMulLambda(const GroupElemJac &p) {
    const secp256k1_fe_t &beta = GetGroupConst().beta;
    *this = p;
    secp256k1_fe_mul(&x, &x, &beta);
}

void SplitExp(const secp256k1_num_t &exp, secp256k1_num_t &exp1, secp256k1_num_t &exp2) {
    const GroupConstants &c = GetGroupConst();
    secp256k1_num_t bnc1, bnc2, bnt1, bnt2, bnn2;

    secp256k1_num_init(&bnc1);
    secp256k1_num_init(&bnc2);
    secp256k1_num_init(&bnt1);
    secp256k1_num_init(&bnt2);
    secp256k1_num_init(&bnn2);

    secp256k1_num_copy(&bnn2, &c.order);
    secp256k1_num_shift(&bnn2, 1);

    secp256k1_num_mul(&bnc1, &exp, &c.a1b2);
    secp256k1_num_add(&bnc1, &bnc1, &bnn2);
    secp256k1_num_div(&bnc1, &bnc1, &c.order);

    secp256k1_num_mul(&bnc2, &exp, &c.b1);
    secp256k1_num_add(&bnc2, &bnc2, &bnn2);
    secp256k1_num_div(&bnc2, &bnc2, &c.order);

    secp256k1_num_mul(&bnt1, &bnc1, &c.a1b2);
    secp256k1_num_mul(&bnt2, &bnc2, &c.a2);
    secp256k1_num_add(&bnt1, &bnt1, &bnt2);
    secp256k1_num_sub(&exp1, &exp, &bnt1);
    secp256k1_num_mul(&bnt1, &bnc1, &c.b1);
    secp256k1_num_mul(&bnt2, &bnc2, &c.a1b2);
    secp256k1_num_sub(&exp2, &bnt1, &bnt2);

    secp256k1_num_free(&bnc1);
    secp256k1_num_free(&bnc2);
    secp256k1_num_free(&bnt1);
    secp256k1_num_free(&bnt2);
    secp256k1_num_free(&bnn2);
}

}
