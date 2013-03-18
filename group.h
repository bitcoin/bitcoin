#ifndef _SECP256K1_GROUP_
#define _SECP256K1_GROUP_

#include <string>

#include "num.h"
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
    GroupElem();

    /** Creates the point with given affine coordinates */
    GroupElem(const FieldElem &xin, const FieldElem &yin);

    /** Checks whether this is the point at infinity */
    bool IsInfinity() const;

    void SetNeg(const GroupElem &p);

    void GetX(FieldElem &xout);

    void GetY(FieldElem &yout);

    std::string ToString() const;

    void SetJac(GroupElemJac &jac);

    friend class GroupElemJac;
};

/** Represents a point on the secp256k1 curve, with jacobian coordinates */
class GroupElemJac : private GroupElem {
protected:
    FieldElem z;

public:
    /** Creates the point at infinity */
    GroupElemJac();

    /** Creates the point with given affine coordinates */
    GroupElemJac(const FieldElem &xin, const FieldElem &yin);

    GroupElemJac(const GroupElem &in);

    void SetJac(const GroupElemJac &jac);

    void SetAffine(const GroupElem &aff);

    /** Checks whether this is a non-infinite point on the curve */
    bool IsValid() const;

    /** Returns the affine coordinates of this point */
    void GetAffine(GroupElem &aff);

    void GetX(FieldElem &xout);
    void GetY(FieldElem &yout);

    bool IsInfinity() const;

    void SetNeg(const GroupElemJac &p);

    /** Sets this point to have a given X coordinate & given Y oddness */
    void SetCompressed(const FieldElem &xin, bool fOdd);

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
    const FieldElem g_x;
    const FieldElem g_y;

public:
    const Number order;
    const GroupElem g;
    const FieldElem beta;
    const Number lambda, a1b2, b1, a2;

    GroupConstants();
};

const GroupConstants &GetGroupConst();

void SplitExp(const Number &exp, Number &exp1, Number &exp2);

}

#endif
