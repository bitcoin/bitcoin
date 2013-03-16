#ifndef _SECP256K1_FIELD_
#define _SECP256K1_FIELD_

using namespace std;

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
    // X = sum(i=0..4, elem[i]*2^52) mod n
    uint64_t n[5];
#ifdef VERIFY_MAGNITUDE
    int magnitude;
#endif

public:

    /** Creates a constant field element. Magnitude=1 */
    FieldElem(int x = 0);

    FieldElem(const unsigned char *b32);

    /** Normalizes the internal representation entries. Magnitude=1 */
    void Normalize();

    bool IsZero();

    bool friend operator==(FieldElem &a, FieldElem &b);

    /** extract as 32-byte big endian array */
    void GetBytes(unsigned char *o);

    /** set value of 32-byte big endian array */
    void SetBytes(const unsigned char *in);

    /** Set a FieldElem to be the negative of another. Increases magnitude by one. */
    void SetNeg(const FieldElem &a, int magnitudeIn);

    /** Multiplies this FieldElem with an integer constant. Magnitude is multiplied by v */
    void operator*=(int v);

    void operator+=(const FieldElem &a);

    /** Set this FieldElem to be the multiplication of two others. Magnitude=1 */
    void SetMult(const FieldElem &a, const FieldElem &b);

    /** Set this FieldElem to be the square of another. Magnitude=1 */
    void SetSquare(const FieldElem &a);

    /** Set this to be the (modular) square root of another FieldElem. Magnitude=1 */
    void SetSquareRoot(const FieldElem &a);

    bool IsOdd();

    /** Set this to be the (modular) inverse of another FieldElem. Magnitude=1 */
    void SetInverse(FieldElem &a);

    std::string ToString();

    void SetHex(const std::string &str);
};

class FieldConstants {
public:
    const Number field_p;

    FieldConstants();
};

const FieldConstants &GetFieldConst();

}

#endif
