#include "num.h"
#include "field.h"
#include "group.h"
#include "ecmult.h"
#include "ecdsa.h"

namespace secp256k1 {

bool ParsePubKey(secp256k1_gej_t &elem, const unsigned char *pub, int size) {
    if (size == 33 && (pub[0] == 0x02 || pub[0] == 0x03)) {
        secp256k1_fe_t x;
        secp256k1_fe_set_b32(&x, pub+1);
        secp256k1_gej_set_xo(&elem, &x, pub[0] == 0x03);
    } else if (size == 65 && (pub[0] == 0x04 || pub[0] == 0x06 || pub[0] == 0x07)) {
        secp256k1_fe_t x,y;
        secp256k1_fe_set_b32(&x, pub+1);
        secp256k1_fe_set_b32(&y, pub+33);
        secp256k1_gej_set_xy(&elem, &x, &y);
        if ((pub[0] == 0x06 || pub[0] == 0x07) && secp256k1_fe_is_odd(&y) != (pub[0] == 0x07))
            return false;
    } else {
        return false;
    }
    return secp256k1_gej_is_valid(&elem);
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
    secp256k1_num_set_bin(&r, sig+4, lenr);
    secp256k1_num_set_bin(&s, sig+6+lenr, lens);
    return true;
}

bool Signature::Serialize(unsigned char *sig, int *size) {
    int lenR = (secp256k1_num_bits(&r) + 7)/8;
    if (lenR == 0 || secp256k1_num_get_bit(&r, lenR*8-1))
        lenR++;
    int lenS = (secp256k1_num_bits(&s) + 7)/8;
    if (lenS == 0 || secp256k1_num_get_bit(&s, lenS*8-1))
        lenS++;
    if (*size < 6+lenS+lenR)
        return false;
    *size = 6 + lenS + lenR;
    sig[0] = 0x30;
    sig[1] = 4 + lenS + lenR;
    sig[2] = 0x02;
    sig[3] = lenR;
    secp256k1_num_get_bin(sig+4, lenR, &r);
    sig[4+lenR] = 0x02;
    sig[5+lenR] = lenS;
    secp256k1_num_get_bin(sig+lenR+6, lenS, &s);
    return true;
}

bool Signature::RecomputeR(secp256k1_num_t &r2, const secp256k1_gej_t &pubkey, const secp256k1_num_t &message) const {
    const secp256k1_ge_consts_t &c = *secp256k1_ge_consts;

    if (secp256k1_num_is_neg(&r) || secp256k1_num_is_neg(&s))
        return false;
    if (secp256k1_num_is_zero(&r) || secp256k1_num_is_zero(&s))
        return false;
    if (secp256k1_num_cmp(&r, &c.order) >= 0 || secp256k1_num_cmp(&s, &c.order) >= 0)
        return false;

    bool ret = false;
    secp256k1_num_t sn, u1, u2;
    secp256k1_num_init(&sn);
    secp256k1_num_init(&u1);
    secp256k1_num_init(&u2);
    secp256k1_num_mod_inverse(&sn, &s, &c.order);
    secp256k1_num_mod_mul(&u1, &sn, &message, &c.order);
    secp256k1_num_mod_mul(&u2, &sn, &r, &c.order);
    secp256k1_gej_t pr; secp256k1_ecmult(&pr, &pubkey, &u2, &u1);
    if (!secp256k1_gej_is_infinity(&pr)) {
        secp256k1_fe_t xr; secp256k1_gej_get_x(&xr, &pr);
        secp256k1_fe_normalize(&xr);
        unsigned char xrb[32]; secp256k1_fe_get_b32(xrb, &xr);
        secp256k1_num_set_bin(&r2, xrb, 32);
        secp256k1_num_mod(&r2, &r2, &c.order);
        ret = true;
    }
    secp256k1_num_free(&sn);
    secp256k1_num_free(&u1);
    secp256k1_num_free(&u2);
    return ret;
}

bool Signature::Verify(const secp256k1_gej_t &pubkey, const secp256k1_num_t &message) const {
    secp256k1_num_t r2;
    secp256k1_num_init(&r2);
    bool ret = false;
    ret = RecomputeR(r2, pubkey, message) && secp256k1_num_cmp(&r, &r2) == 0;
    secp256k1_num_free(&r2);
    return ret;
}

bool Signature::Sign(const secp256k1_num_t &seckey, const secp256k1_num_t &message, const secp256k1_num_t &nonce) {
    const secp256k1_ge_consts_t &c = *secp256k1_ge_consts;

    secp256k1_gej_t rp;
    secp256k1_ecmult_gen(&rp, &nonce);
    secp256k1_fe_t rx;
    secp256k1_gej_get_x(&rx, &rp);
    unsigned char b[32];
    secp256k1_fe_normalize(&rx);
    secp256k1_fe_get_b32(b, &rx);
    secp256k1_num_set_bin(&r, b, 32);
    secp256k1_num_mod(&r, &r, &c.order);
    secp256k1_num_t n;
    secp256k1_num_init(&n);
    secp256k1_num_mod_mul(&n, &r, &seckey, &c.order);
    secp256k1_num_add(&n, &n, &message);
    secp256k1_num_mod_inverse(&s, &nonce, &c.order);
    secp256k1_num_mod_mul(&s, &s, &n, &c.order);
    secp256k1_num_free(&n);
    if (secp256k1_num_is_zero(&s))
        return false;
    if (secp256k1_num_is_odd(&s))
        secp256k1_num_sub(&s, &c.order, &s);
    return true;
}

void Signature::SetRS(const secp256k1_num_t &rin, const secp256k1_num_t &sin) {
    secp256k1_num_copy(&r, &rin);
    secp256k1_num_copy(&s, &sin);
}

std::string Signature::ToString() const {
    char rs[65], ss[65];
    int rl = 65, sl = 65;
    secp256k1_num_get_hex(rs, &rl, &r);
    secp256k1_num_get_hex(ss, &sl, &s);
    return "(" + std::string(rs) + "," + std::string(ss) + ")";
}

}
