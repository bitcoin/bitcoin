#ifndef _SECP256K1_GROUP_
#define _SECP256K1_GROUP_

#include <string>

#include "num.h"
#include "field.h"

extern "C" {

typedef struct {
    secp256k1_fe_t x;
    secp256k1_fe_t y;
    int infinity;
} secp256k1_ge_t;

typedef struct {
    secp256k1_fe_t x;
    secp256k1_fe_t y;
    secp256k1_fe_t z;
    int infinity;
} secp256k1_gej_t;

typedef struct {
    secp256k1_num_t order;
    secp256k1_ge_t g;
    secp256k1_fe_t beta;
    secp256k1_num_t lambda, a1b2, b1, a2;
} secp256k1_ge_consts_t;

static secp256k1_ge_consts_t *secp256k1_ge_consts = NULL;

void static secp256k1_ge_start(void);
void static secp256k1_ge_stop(void);
void static secp256k1_ge_set_infinity(secp256k1_ge_t *r);
void static secp256k1_ge_set_xy(secp256k1_ge_t *r, const secp256k1_fe_t *x, const secp256k1_fe_t *y);
int  static secp256k1_ge_is_infinity(const secp256k1_ge_t *a);
void static secp256k1_ge_neg(secp256k1_ge_t *r, const secp256k1_ge_t *a);
void static secp256k1_ge_get_hex(char *r, int *rlen, const secp256k1_ge_t *a);
void static secp256k1_ge_set_gej(secp256k1_ge_t *r, const secp256k1_gej_t *a);

void static secp256k1_gej_set_infinity(secp256k1_gej_t *r);
void static secp256k1_gej_set_xy(secp256k1_gej_t *r, const secp256k1_fe_t *x, const secp256k1_fe_t *y);
void static secp256k1_gej_set_xo(secp256k1_gej_t *r, const secp256k1_fe_t *x, int compressed);
void static secp256k1_gej_set_ge(secp256k1_gej_t *r, const secp256k1_ge_t *a);
void static secp256k1_gej_get_x(secp256k1_fe_t *r, const secp256k1_gej_t *a);
void static secp256k1_gej_neg(secp256k1_gej_t *r, const secp256k1_gej_t *a);
int  static secp256k1_gej_is_infinity(const secp256k1_gej_t *a);
void static secp256k1_gej_double(secp256k1_gej_t *r, const secp256k1_gej_t *a);
void static secp256k1_gej_add(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_gej_t *b);
void static secp256k1_gej_add_ge(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_ge_t *b);
void static secp256k1_gej_get_hex(char *r, int *rlen, const secp256k1_gej_t *a);
void static secp256k1_gej_mul_lambda(secp256k1_gej_t *r, const secp256k1_gej_t *a);
void static secp256k1_gej_split_exp(secp256k1_num_t *r1, secp256k1_num_t *r2, const secp256k1_num_t *a);

}

namespace secp256k1 {

class GroupElemJac;

/** Defines a point on the secp256k1 curve (y^2 = x^3 + 7) */
class GroupElem {
protected:
    bool fInfinity;
    secp256k1_fe_t x;
    secp256k1_fe_t y;

public:

    /** Creates the point at infinity */
    GroupElem();

    /** Creates the point with given affine coordinates */
    GroupElem(const secp256k1_fe_t &xin, const secp256k1_fe_t &yin);

    /** Checks whether this is the point at infinity */
    bool IsInfinity() const;

    void SetNeg(const GroupElem &p);

    void GetX(secp256k1_fe_t &xout);

    void GetY(secp256k1_fe_t &yout);

    std::string ToString() const;

    void SetJac(GroupElemJac &jac);

    friend class GroupElemJac;
};

/** Represents a point on the secp256k1 curve, with jacobian coordinates */
class GroupElemJac : private GroupElem {
protected:
    secp256k1_fe_t z;

public:
    /** Creates the point at infinity */
    GroupElemJac();

    /** Creates the point with given affine coordinates */
    GroupElemJac(const secp256k1_fe_t &xin, const secp256k1_fe_t &yin);

    GroupElemJac(const GroupElem &in);

    void SetJac(const GroupElemJac &jac);

    void SetAffine(const GroupElem &aff);

    /** Checks whether this is a non-infinite point on the curve */
    bool IsValid() const;

    /** Returns the affine coordinates of this point */
    void GetAffine(GroupElem &aff);

    void GetX(secp256k1_fe_t &xout);
    void GetY(secp256k1_fe_t &yout);

    bool IsInfinity() const;

    void SetNeg(const GroupElemJac &p);

    /** Sets this point to have a given X coordinate & given Y oddness */
    void SetCompressed(const secp256k1_fe_t &xin, bool fOdd);

    /** Sets this point to be the EC double of another */
    void SetDouble(const GroupElemJac &p);

    /** Sets this point to be the EC addition of two others */
    void SetAdd(const GroupElemJac &p, const GroupElemJac &q);

    /** Sets this point to be the EC addition of two others (one of which is in affine coordinates) */
    void SetAdd(const GroupElemJac &p, const GroupElem &q);

    std::string ToString() const;

    void SetMulLambda(const GroupElemJac &p);
};

class GroupConstants {
private:
    secp256k1_fe_t g_x;
    secp256k1_fe_t g_y;

public:
    secp256k1_num_t order;
    GroupElem g;
    secp256k1_fe_t beta;
    secp256k1_num_t lambda, a1b2, b1, a2;

    GroupConstants();
    ~GroupConstants();
};

const GroupConstants &GetGroupConst();

void SplitExp(const secp256k1_num_t &exp, secp256k1_num_t &exp1, secp256k1_num_t &exp2);

}

#endif
