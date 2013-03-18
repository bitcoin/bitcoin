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

bool Signature::Serialize(unsigned char *sig, int *size) {
    int lenR = (r.GetBits() + 7)/8;
    if (lenR == 0 || r.CheckBit(lenR*8-1))
        lenR++;
    int lenS = (s.GetBits() + 7)/8;
    if (lenS == 0 || s.CheckBit(lenS*8-1))
        lenS++;
    if (*size < 6+lenS+lenR)
        return false;
    *size = 6 + lenS + lenR;
    sig[0] = 0x30;
    sig[1] = 4 + lenS + lenR;
    sig[2] = 0x02;
    sig[3] = lenR;
    r.GetBytes(sig+4, lenR);
    sig[4+lenR] = 0x02;
    sig[5+lenR] = lenS;
    s.GetBytes(sig+6, lenS);
    return true;
}

bool Signature::RecomputeR(Number &r2, const GroupElemJac &pubkey, const Number &message) const {
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

bool Signature::Verify(const GroupElemJac &pubkey, const Number &message) const {
    Number r2;
    if (!RecomputeR(r2, pubkey, message))
        return false;
    return r2.Compare(r) == 0;
}

bool Signature::Sign(const Number &seckey, const Number &message, const Number &nonce) {
    const GroupConstants &c = GetGroupConst();

    GroupElemJac rp;
    ECMultBase(rp, nonce);
    FieldElem rx;
    rp.GetX(rx);
    unsigned char b[32];
    rx.GetBytes(b);
    r.SetBytes(b, 32);
    r.SetMod(r, c.order);
    Number n;
    n.SetModMul(r, seckey, c.order);
    n.SetAdd(message, n);
    s.SetModInverse(nonce, c.order);
    s.SetModMul(s, n, c.order);
    if (s.IsZero())
        return false;
    if (s.IsOdd())
        s.SetSub(c.order, s);
    return true;
}

void Signature::SetRS(const Number &rin, const Number &sin) {
    r = rin;
    s = sin;
}

std::string Signature::ToString() const {
    return "(" + r.ToString() + "," + s.ToString() + ")";
}

}
