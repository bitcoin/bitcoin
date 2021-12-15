/***********************************************************************
 * Copyright (c) 2013, 2014, 2015 Thomas Daede, Cory Fields            *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

/* Autotools creates libsecp256k1-config.h, of which ECMULT_GEN_PREC_BITS is needed.
   ifndef guard so downstream users can define their own if they do not use autotools. */
#if !defined(ECMULT_GEN_PREC_BITS)
#include "libsecp256k1-config.h"
#endif

/* We can't require the precomputed tables when creating them. */
#undef USE_ECMULT_STATIC_PRECOMPUTATION

/* In principle we could use ASM, but this yields only a minor speedup in
   build time and it's very complicated. In particular when cross-compiling, we'd
   need to build the ASM for the build and the host machine. */
#undef USE_EXTERNAL_ASM
#undef USE_ASM_X86_64

#include "../include/secp256k1.h"
#include "assumptions.h"
#include "util.h"
#include "field_impl.h"
#include "scalar_impl.h"
#include "group_impl.h"
#include "ecmult_gen_impl.h"

static void default_error_callback_fn(const char* str, void* data) {
    (void)data;
    fprintf(stderr, "[libsecp256k1] internal consistency check failed: %s\n", str);
    abort();
}

static const secp256k1_callback default_error_callback = {
    default_error_callback_fn,
    NULL
};

int main(int argc, char **argv) {
    secp256k1_ecmult_gen_context ctx;
    void *prealloc, *base;
    int inner;
    int outer;
    FILE* fp;

    (void)argc;
    (void)argv;

    fp = fopen("src/ecmult_static_context.h","w");
    if (fp == NULL) {
        fprintf(stderr, "Could not open src/ecmult_static_context.h for writing!\n");
        return -1;
    }

    fprintf(fp, "#ifndef SECP256K1_ECMULT_STATIC_CONTEXT_H\n");
    fprintf(fp, "#define SECP256K1_ECMULT_STATIC_CONTEXT_H\n");
    fprintf(fp, "#include \"src/group.h\"\n");
    fprintf(fp, "#define SC SECP256K1_GE_STORAGE_CONST\n");
    fprintf(fp, "#if ECMULT_GEN_PREC_N != %d || ECMULT_GEN_PREC_G != %d\n", ECMULT_GEN_PREC_N, ECMULT_GEN_PREC_G);
    fprintf(fp, "   #error configuration mismatch, invalid ECMULT_GEN_PREC_N, ECMULT_GEN_PREC_G. Try deleting ecmult_static_context.h before the build.\n");
    fprintf(fp, "#endif\n");
    fprintf(fp, "static const secp256k1_ge_storage secp256k1_ecmult_static_context[ECMULT_GEN_PREC_N][ECMULT_GEN_PREC_G] = {\n");

    base = checked_malloc(&default_error_callback, SECP256K1_ECMULT_GEN_CONTEXT_PREALLOCATED_SIZE);
    prealloc = base;
    secp256k1_ecmult_gen_context_init(&ctx);
    secp256k1_ecmult_gen_context_build(&ctx, &prealloc);
    for(outer = 0; outer != ECMULT_GEN_PREC_N; outer++) {
        fprintf(fp,"{\n");
        for(inner = 0; inner != ECMULT_GEN_PREC_G; inner++) {
            fprintf(fp,"    SC(%uu, %uu, %uu, %uu, %uu, %uu, %uu, %uu, %uu, %uu, %uu, %uu, %uu, %uu, %uu, %uu)", SECP256K1_GE_STORAGE_CONST_GET((*ctx.prec)[outer][inner]));
            if (inner != ECMULT_GEN_PREC_G - 1) {
                fprintf(fp,",\n");
            } else {
                fprintf(fp,"\n");
            }
        }
        if (outer != ECMULT_GEN_PREC_N - 1) {
            fprintf(fp,"},\n");
        } else {
            fprintf(fp,"}\n");
        }
    }
    fprintf(fp,"};\n");
    secp256k1_ecmult_gen_context_clear(&ctx);
    free(base);

    fprintf(fp, "#undef SC\n");
    fprintf(fp, "#endif\n");
    fclose(fp);

    return 0;
}
