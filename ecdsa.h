#ifndef _SECP256K1_ECDSA_
#define _SECP256K1_ECDSA_

namespace secp256k1 {

bool ParsePubkey(GroupElemJac &elem, const unsigned char *pub, int size) {
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

class Signature {
private:
    Number r,s;

public:
    bool Verify(Context &ctx, const GroupElemJac &pubkey, const Number &message) {
        const GroupConstants &c = GetGroupConst();

        if (r.IsNeg() || s.IsNeg())
            return false;
        if (r.IsZero() || s.IsZero())
            return false;
        if (r.Compare(c.order) >= 0 || s.Compare(c.order) >= 0)
            return false;

        Context ct(ctx);
        Number sn(ct); sn.SetModInverse(ct, s, c.order);
        Number u1(ct); u1.SetModMul(ct, sn, message, c.order);
        Number u2(ct); u2.SetModMul(ct, sn, r, c.order);
        GroupElemJac pr; ECMult(ct, pr, pubkey, u2, u1);
        if (pr.IsInfinity())
            return false;
        FieldElem xr; pr.GetX(xr);
        unsigned char xrb[32]; xr.GetBytes(xrb);
        Number xrn(ct); xrn.SetBytes(xrb,32); xrn.SetMod(ct,xrn,c.order);
        return xrn.Compare(r) == 0;
    }
};

}

#endif
