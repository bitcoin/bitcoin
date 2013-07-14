// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "impl/num.h"
#include "impl/field.h"
#include "impl/group.h"
#include "impl/ecmult.h"
#include "impl/ecdsa.h"

void secp256k1_start(void) {
    secp256k1_fe_start();
    secp256k1_ge_start();
    secp256k1_ecmult_start();
}

void secp256k1_stop(void) {
    secp256k1_ecmult_stop();
    secp256k1_ge_stop();
    secp256k1_fe_stop();
}

int secp256k1_ecdsa_verify(const unsigned char *msg, int msglen, const unsigned char *sig, int siglen, const unsigned char *pubkey, int pubkeylen) {
    int ret = -3;
    secp256k1_num_t m; 
    secp256k1_num_init(&m);
    secp256k1_ecdsa_sig_t s;
    secp256k1_ecdsa_sig_init(&s);
    secp256k1_ge_t q;
    secp256k1_num_set_bin(&m, msg, msglen);

    if (!secp256k1_ecdsa_pubkey_parse(&q, pubkey, pubkeylen)) {
        ret = -1;
        goto end;
    }
    if (!secp256k1_ecdsa_sig_parse(&s, sig, siglen)) {
        ret = -2;
        goto end;
    }
    if (!secp256k1_ecdsa_sig_verify(&s, &q, &m)) {
        ret = 0;
        goto end;
    }
    ret = 1;
end:
    secp256k1_ecdsa_sig_free(&s);
    secp256k1_num_free(&m);
    return ret;
}

int secp256k1_ecdsa_sign(const unsigned char *message, int messagelen, unsigned char *signature, int *signaturelen, const unsigned char *seckey, const unsigned char *nonce) {
    secp256k1_num_t sec, non, msg;
    secp256k1_num_init(&sec);
    secp256k1_num_init(&non);
    secp256k1_num_init(&msg);
    secp256k1_num_set_bin(&sec, seckey, 32);
    secp256k1_num_set_bin(&non, nonce, 32);
    secp256k1_num_set_bin(&msg, message, messagelen);
    secp256k1_ecdsa_sig_t sig;
    secp256k1_ecdsa_sig_init(&sig);
    int ret = secp256k1_ecdsa_sig_sign(&sig, &sec, &msg, &non, NULL);
    if (ret) {
        secp256k1_ecdsa_sig_serialize(signature, signaturelen, &sig);
    }
    secp256k1_ecdsa_sig_free(&sig);
    secp256k1_num_free(&msg);
    secp256k1_num_free(&non);
    secp256k1_num_free(&sec);
    return ret;
}

int secp256k1_ecdsa_sign_compact(const unsigned char *message, int messagelen, unsigned char *sig64, const unsigned char *seckey, const unsigned char *nonce, int *recid) {
    secp256k1_num_t sec, non, msg;
    secp256k1_num_init(&sec);
    secp256k1_num_init(&non);
    secp256k1_num_init(&msg);
    secp256k1_num_set_bin(&sec, seckey, 32);
    secp256k1_num_set_bin(&non, nonce, 32);
    secp256k1_num_set_bin(&msg, message, messagelen);
    secp256k1_ecdsa_sig_t sig;
    secp256k1_ecdsa_sig_init(&sig);
    int ret = secp256k1_ecdsa_sig_sign(&sig, &sec, &msg, &non, recid);
    if (ret) {
        secp256k1_num_get_bin(sig64, 32, &sig.r);
        secp256k1_num_get_bin(sig64 + 32, 32, &sig.s);
    }
    secp256k1_ecdsa_sig_free(&sig);
    secp256k1_num_free(&msg);
    secp256k1_num_free(&non);
    secp256k1_num_free(&sec);
    return ret;
}

int secp256k1_ecdsa_recover_compact(const unsigned char *msg, int msglen, const unsigned char *sig64, unsigned char *pubkey, int *pubkeylen, int compressed, int recid) {
    int ret = 0;
    secp256k1_num_t m; 
    secp256k1_num_init(&m);
    secp256k1_ecdsa_sig_t sig;
    secp256k1_ecdsa_sig_init(&sig);
    secp256k1_num_set_bin(&sig.r, sig64, 32);
    secp256k1_num_set_bin(&sig.s, sig64 + 32, 32);
    secp256k1_num_set_bin(&m, msg, msglen);

    secp256k1_ge_t q;
    if (secp256k1_ecdsa_sig_recover(&sig, &q, &m, recid)) {
        secp256k1_ecdsa_pubkey_serialize(&q, pubkey, pubkeylen, compressed);
        ret = 1;
    }
    secp256k1_ecdsa_sig_free(&sig);
    secp256k1_num_free(&m);
    return ret;
}

int secp256k1_ecdsa_seckey_verify(const unsigned char *seckey) {
    secp256k1_num_t sec;
    secp256k1_num_init(&sec);
    secp256k1_num_set_bin(&sec, seckey, 32);
    int ret = !secp256k1_num_is_zero(&sec) &&
              (secp256k1_num_cmp(&sec, &secp256k1_ge_consts->order) < 0);
    secp256k1_num_free(&sec);
    return ret;
}

int secp256k1_ecdsa_pubkey_verify(const unsigned char *pubkey, int pubkeylen) {
    secp256k1_ge_t q;
    return secp256k1_ecdsa_pubkey_parse(&q, pubkey, pubkeylen);
}

int secp256k1_ecdsa_pubkey_create(unsigned char *pubkey, int *pubkeylen, const unsigned char *seckey, int compressed) {
    secp256k1_num_t sec;
    secp256k1_num_init(&sec);
    secp256k1_num_set_bin(&sec, seckey, 32);
    secp256k1_gej_t pj;
    secp256k1_ecmult_gen(&pj, &sec);
    secp256k1_ge_t p;
    secp256k1_ge_set_gej(&p, &pj);
    secp256k1_ecdsa_pubkey_serialize(&p, pubkey, pubkeylen, compressed);
    return 1;
}

int secp256k1_ecdsa_pubkey_decompress(unsigned char *pubkey, int *pubkeylen) {
    secp256k1_ge_t p;
    if (!secp256k1_ecdsa_pubkey_parse(&p, pubkey, *pubkeylen))
        return 0;
    secp256k1_ecdsa_pubkey_serialize(&p, pubkey, pubkeylen, 0);
    return 1;
}

int secp256k1_ecdsa_privkey_tweak_add(unsigned char *seckey, const unsigned char *tweak) {
    int ret = 1;
    secp256k1_num_t term;
    secp256k1_num_init(&term);
    secp256k1_num_set_bin(&term, tweak, 32);
    if (secp256k1_num_cmp(&term, &secp256k1_ge_consts->order) >= 0)
        ret = 0;
    secp256k1_num_t sec;
    secp256k1_num_init(&sec);
    if (ret) {
        secp256k1_num_set_bin(&sec, seckey, 32);
        secp256k1_num_add(&sec, &sec, &term);
        secp256k1_num_mod(&sec, &secp256k1_ge_consts->order);
        if (secp256k1_num_is_zero(&sec))
            ret = 0;
    }
    if (ret)
        secp256k1_num_get_bin(seckey, 32, &sec);
    secp256k1_num_free(&sec);
    secp256k1_num_free(&term);
    return ret;
}

int secp256k1_ecdsa_pubkey_tweak_add(unsigned char *pubkey, int pubkeylen, const unsigned char *tweak) {
    int ret = 1;
    secp256k1_num_t term;
    secp256k1_num_init(&term);
    secp256k1_num_set_bin(&term, tweak, 32);
    if (secp256k1_num_cmp(&term, &secp256k1_ge_consts->order) >= 0)
        ret = 0;
    secp256k1_ge_t p;
    if (ret) {
        if (!secp256k1_ecdsa_pubkey_parse(&p, pubkey, pubkeylen))
            ret = 0;
    }
    if (ret) {
        secp256k1_gej_t pt;
        secp256k1_ecmult_gen(&pt, &term);
        secp256k1_gej_add_ge(&pt, &pt, &p);
        if (secp256k1_gej_is_infinity(&pt))
            ret = 0;
        secp256k1_ge_set_gej(&p, &pt);
        int oldlen = pubkeylen;
        secp256k1_ecdsa_pubkey_serialize(&p, pubkey, &pubkeylen, oldlen <= 33);
        assert(pubkeylen == oldlen);
    }
    secp256k1_num_free(&term);
    return ret;
}

int secp256k1_ecdsa_privkey_tweak_mul(unsigned char *seckey, const unsigned char *tweak) {
    int ret = 1;
    secp256k1_num_t factor;
    secp256k1_num_init(&factor);
    secp256k1_num_set_bin(&factor, tweak, 32);
    if (secp256k1_num_is_zero(&factor))
        ret = 0;
    if (secp256k1_num_cmp(&factor, &secp256k1_ge_consts->order) >= 0)
        ret = 0;
    secp256k1_num_t sec;
    secp256k1_num_init(&sec);
    if (ret) {
        secp256k1_num_set_bin(&sec, seckey, 32);
        secp256k1_num_mod_mul(&sec, &sec, &factor, &secp256k1_ge_consts->order);
    }
    if (ret)
        secp256k1_num_get_bin(seckey, 32, &sec);
    secp256k1_num_free(&sec);
    secp256k1_num_free(&factor);
    return ret;
}

int secp256k1_ecdsa_pubkey_tweak_mul(unsigned char *pubkey, int pubkeylen, const unsigned char *tweak) {
    int ret = 1;
    secp256k1_num_t factor;
    secp256k1_num_init(&factor);
    secp256k1_num_set_bin(&factor, tweak, 32);
    if (secp256k1_num_is_zero(&factor))
        ret = 0;
    if (secp256k1_num_cmp(&factor, &secp256k1_ge_consts->order) >= 0)
        ret = 0;
    secp256k1_ge_t p;
    if (ret) {
        if (!secp256k1_ecdsa_pubkey_parse(&p, pubkey, pubkeylen))
            ret = 0;
    }
    if (ret) {
        secp256k1_num_t zero;
        secp256k1_num_init(&zero);
        secp256k1_num_set_int(&zero, 0);
        secp256k1_gej_t pt;
        secp256k1_gej_set_ge(&pt, &p);
        secp256k1_ecmult(&pt, &pt, &factor, &zero);
        secp256k1_num_free(&zero);
        secp256k1_ge_set_gej(&p, &pt);
        int oldlen = pubkeylen;
        secp256k1_ecdsa_pubkey_serialize(&p, pubkey, &pubkeylen, oldlen <= 33);
        assert(pubkeylen == oldlen);
    }
    secp256k1_num_free(&factor);
    return ret;
}

int secp256k1_ecdsa_privkey_export(const unsigned char *seckey, unsigned char *privkey, int *privkeylen, int compressed) {
    secp256k1_num_t key;
    secp256k1_num_init(&key);
    secp256k1_num_set_bin(&key, seckey, 32);
    int ret = secp256k1_ecdsa_privkey_serialize(privkey, privkeylen, &key, compressed);
    secp256k1_num_free(&key);
    return ret;
}

int secp256k1_ecdsa_privkey_import(unsigned char *seckey, const unsigned char *privkey, int privkeylen) {
    secp256k1_num_t key;
    secp256k1_num_init(&key);
    int ret = secp256k1_ecdsa_privkey_parse(&key, privkey, privkeylen);
    if (ret)
        secp256k1_num_get_bin(seckey, 32, &key);
    secp256k1_num_free(&key);
    return ret;
}
