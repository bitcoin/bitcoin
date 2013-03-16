#include "num.h"
#include "field.h"
#include "group.h"
#include "ecmult.h"
#include "ecdsa.h"

namespace secp256k1 {

bool ParsePubKey(GroupElemJac &elem, const unsigned char *pub, int size) {
    if (size == 33 && (pub[0] == 0x02 || pub[0] == 0x03)) {
        FieldElem x;
        x.SetBytes(pub+1);
        elem.SetCompressed(x, pub[0] == 0x03);
    } else if (size == 65 && (pub[0] == 0x04 || pub[0] == 0x06 || pub[0] == 0x07)) {
        FieldElem x,y;
        x.SetBytes(pub+1);
        y.SetBytes(pub+33);
        elem = GroupElem(x,y);
        if ((pub[0] == 0x06 || pub[0] == 0x07) && y.IsOdd() != (pub[0] == 0x07))
            return false;
    } else {
        return false;
    }
    return elem.IsValid();
}

bool Signature::Parse(const unsigned char *sig, int size) {
    if (sig[0] != 0x30) return false;
    int lenr = sig[3];
    if (5+lenr >= size) return false;
    int lens = sig[lenr+5];
    if (sig[1] != lenr+lens+4) return false;
    if (lenr+lens+6 > size) return false;
    if (sig[2] != 0x02) return false;
    if (lenr == 0) return false;
    if (sig[lenr+4] != 0x02) return false;
    if (lens == 0) return false;
    r.SetBytes(sig+4, lenr);
    s.SetBytes(sig+6+lenr, lens);
    return true;
}

bool Signature::RecomputeR(Number &r2, const GroupElemJac &pubkey, const Number &message) {
    const GroupConstants &c = GetGroupConst();

    if (r.IsNeg() || s.IsNeg())
        return false;
    if (r.IsZero() || s.IsZero())
        return false;
    if (r.Compare(c.order) >= 0 || s.Compare(c.order) >= 0)
        return false;

    Number sn, u1, u2;
    sn.SetModInverse(s, c.order);
    u1.SetModMul(sn, message, c.order);
    u2.SetModMul(sn, r, c.order);
    GroupElemJac pr; ECMult(pr, pubkey, u2, u1);
    if (pr.IsInfinity())
        return false;
    FieldElem xr; pr.GetX(xr);
    unsigned char xrb[32]; xr.GetBytes(xrb);
    r2.SetBytes(xrb,32); r2.SetMod(r2,c.order);
    return true;
}

bool Signature::Verify(const GroupElemJac &pubkey, const Number &message) {
    Number r2;
    if (!RecomputeR(r2, pubkey, message))
        return false;
    return r2.Compare(r) == 0;
}

void Signature::SetRS(const Number &rin, const Number &sin) {
    r = rin;
    s = sin;
}

std::string Signature::ToString() const {
    return "(" + r.ToString() + "," + s.ToString() + ")";
}

}
