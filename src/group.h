#ifndef _SECP256K1_GROUP_
#define _SECP256K1_GROUP_

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
void static secp256k1_ge_set_gej(secp256k1_ge_t *r, secp256k1_gej_t *a);

void static secp256k1_gej_set_infinity(secp256k1_gej_t *r);
void static secp256k1_gej_set_xy(secp256k1_gej_t *r, const secp256k1_fe_t *x, const secp256k1_fe_t *y);
void static secp256k1_gej_set_xo(secp256k1_gej_t *r, const secp256k1_fe_t *x, int odd);
void static secp256k1_gej_set_ge(secp256k1_gej_t *r, const secp256k1_ge_t *a);
void static secp256k1_gej_get_x(secp256k1_fe_t *r, const secp256k1_gej_t *a);
void static secp256k1_gej_neg(secp256k1_gej_t *r, const secp256k1_gej_t *a);
int  static secp256k1_gej_is_infinity(const secp256k1_gej_t *a);
int  static secp256k1_gej_is_valid(const secp256k1_gej_t *a);
void static secp256k1_gej_double(secp256k1_gej_t *r, const secp256k1_gej_t *a);
void static secp256k1_gej_add(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_gej_t *b);
void static secp256k1_gej_add_ge(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_ge_t *b);
void static secp256k1_gej_get_hex(char *r, int *rlen, const secp256k1_gej_t *a);
void static secp256k1_gej_mul_lambda(secp256k1_gej_t *r, const secp256k1_gej_t *a);
void static secp256k1_gej_split_exp(secp256k1_num_t *r1, secp256k1_num_t *r2, const secp256k1_num_t *a);

}

#endif
