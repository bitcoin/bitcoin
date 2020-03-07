add_library(secp256k1 STATIC
  src/scalar.h
  src/scalar_4x64.h
  src/scalar_8x32.h
  src/scalar_impl.h
  src/scalar_4x64_impl.h
  src/scalar_8x32_impl.h
  src/group.h
  src/group_impl.h
  src/num_gmp.h
  src/num_gmp_impl.h
  src/ecdsa.h
  src/ecdsa_impl.h
  src/eckey.h
  src/eckey_impl.h
  src/ecmult.h
  src/ecmult_impl.h
  src/ecmult_const.h
  src/ecmult_const_impl.h
  src/ecmult_gen.h
  src/ecmult_gen_impl.h
  src/num.h
  src/num_impl.h
  src/field_10x26.h
  src/field_10x26_impl.h
  src/field_5x52.h
  src/field_5x52_impl.h
  src/field_5x52_int128_impl.h
  src/field_5x52_asm_impl.h
  src/java/org_bitcoin_NativeSecp256k1.h
  src/util.h
  src/testrand.h
  src/testrand_impl.h
  src/hash.h
  src/hash_impl.h
  src/field.h
  src/field_impl.h
  src/bench.h
  contrib/lax_der_parsing.h
  contrib/lax_der_parsing.c
  contrib/lax_der_privatekey_parsing.h
  contrib/lax_der_privatekey_parsing.c
  src/secp256k1.c
)

target_include_directories(secp256k1 PRIVATE . PUBLIC include)

