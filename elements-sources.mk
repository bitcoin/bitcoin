# - All Variables ending in _HEADERS or _SOURCES confuse automake, so the
#     _INT postfix is applied.
# - The %reldir% is the relative path from the Makefile.am.

ELEMENTS_SIMPLICITY_INCLUDE_DIR_INT = %reldir%/include

ELEMENTS_SIMPLICITY_DIST_HEADERS_INT =
ELEMENTS_SIMPLICITY_DIST_HEADERS_INT += %reldir%/include/simplicity/errorCodes.h
ELEMENTS_SIMPLICITY_DIST_HEADERS_INT += %reldir%/include/simplicity/elements/cmr.h
ELEMENTS_SIMPLICITY_DIST_HEADERS_INT += %reldir%/include/simplicity/elements/env.h
ELEMENTS_SIMPLICITY_DIST_HEADERS_INT += %reldir%/include/simplicity/elements/exec.h

ELEMENTS_SIMPLICITY_LIB_SOURCES_INT =
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/bitstream.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/dag.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/deserialize.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/eval.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/frame.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/jets-secp256k1.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/jets.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/rsort.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/sha256.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/type.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/typeInference.c

ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/elements/cmr.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/elements/env.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/elements/exec.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/elements/elementsJets.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/elements/ops.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/elements/primitive.c
ELEMENTS_SIMPLICITY_LIB_SOURCES_INT += %reldir%/elements/txEnv.c

ELEMENTS_SIMPLICITY_LIB_HEADERS_INT =
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/bitstream.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/bitstring.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/bounded.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/dag.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/decodeCoreJets.inc
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/deserialize.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/eval.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/frame.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/jets.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/limitations.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/precomputed.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/rsort.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/sha256.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/sha256_x86.inc
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/simplicity_alloc.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/simplicity_assert.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/taptweak.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/type.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/typeInference.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/uword.h

ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/assumptions.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/eckey.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/eckey_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/ecmult.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/ecmult_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/extrakeys.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/extrakeys_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/field.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/field_5x52.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/field_5x52_asm_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/field_5x52_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/field_5x52_int128_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/field_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/generator.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/generator_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/group.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/group_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/int128.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/int128_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/int128_native.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/int128_native_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/int128_struct.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/int128_struct_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/modinv64.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/modinv64_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/precomputed_ecmult.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/scalar.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/scalar_4x64.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/scalar_4x64_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/scalar_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/schnorrsig.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/schnorrsig_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/secp256k1.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/secp256k1_impl.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/secp256k1/util.h

ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/elements/decodeElementsJets.inc
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/elements/elementsJets.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/elements/ops.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/elements/primitive.h
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/elements/primitiveEnumJet.inc
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/elements/primitiveEnumTy.inc
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/elements/primitiveInitTy.inc
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/elements/primitiveJetNode.inc
ELEMENTS_SIMPLICITY_LIB_HEADERS_INT += %reldir%/elements/txEnv.h
