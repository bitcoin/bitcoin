#pragma once
/**
	@file
	@brief Elliptic curve parameter
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <mcl/curve_type.h>

namespace mcl {

struct EcParam {
	const char *name;
	const char *p;
	const char *a;
	const char *b;
	const char *gx;
	const char *gy;
	const char *n;
	size_t bitSize; // bit length of p
	int curveType;
};

namespace ecparam {

const struct mcl::EcParam secp160k1 = {
	"secp160k1",
	"0xfffffffffffffffffffffffffffffffeffffac73",
	"0",
	"7",
	"0x3b4c382ce37aa192a4019e763036f4f5dd4d7ebb",
	"0x938cf935318fdced6bc28286531733c3f03c4fee",
	"0x100000000000000000001b8fa16dfab9aca16b6b3",
	160,
	MCL_SECP160K1
};
// p=2^160 + 7 (for test)
const struct mcl::EcParam p160_1 = {
	"p160_1",
	"0x10000000000000000000000000000000000000007",
	"10",
	"1343632762150092499701637438970764818528075565078",
	"1",
	"1236612389951462151661156731535316138439983579284",
	"1461501637330902918203683518218126812711137002561",
	161,
	MCL_P160_1
};
const struct mcl::EcParam secp192k1 = {
	"secp192k1",
	"0xfffffffffffffffffffffffffffffffffffffffeffffee37",
	"0",
	"3",
	"0xdb4ff10ec057e9ae26b07d0280b7f4341da5d1b1eae06c7d",
	"0x9b2f2f6d9c5628a7844163d015be86344082aa88d95e2f9d",
	"0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d",
	192,
	MCL_SECP192K1
};
const struct mcl::EcParam secp224k1 = {
	"secp224k1",
	"0xfffffffffffffffffffffffffffffffffffffffffffffffeffffe56d",
	"0",
	"5",
	"0xa1455b334df099df30fc28a169a467e9e47075a90f7e650eb6b7a45c",
	"0x7e089fed7fba344282cafbd6f7e319f7c0b0bd59e2ca4bdb556d61a5",
	"0x10000000000000000000000000001dce8d2ec6184caf0a971769fb1f7",
	224,
	MCL_SECP224K1
};
const struct mcl::EcParam secp256k1 = {
	"secp256k1",
	"0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f",
	"0",
	"7",
	"0x79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798",
	"0x483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8",
	"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141",
	256,
	MCL_SECP256K1
};
const struct mcl::EcParam secp384r1 = {
	"secp384r1",
	"0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff",
	"-3",
	"0xb3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef",
	"0xaa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7",
	"0x3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f",
	"0xffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973",
	384,
	MCL_SECP384R1
};
const struct mcl::EcParam secp521r1 = {
	"secp521r1",
	"0x1ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
	"-3",
	"0x51953eb9618e1c9a1f929a21a0b68540eea2da725b99b315f3b8b489918ef109e156193951ec7e937b1652c0bd3bb1bf073573df883d2c34f1ef451fd46b503f00",
	"0xc6858e06b70404e9cd9e3ecb662395b4429c648139053fb521f828af606b4d3dbaa14b5e77efe75928fe1dc127a2ffa8de3348b3c1856a429bf97e7e31c2e5bd66",
	"0x11839296a789a3bc0045c8a5fb42c7d1bd998f54449579b446817afbd17273e662c97ee72995ef42640c550b9013fad0761353c7086a272c24088be94769fd16650",
	"0x1fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffa51868783bf2f966b7fcc0148f709a5d03bb5c9b8899c47aebb6fb71e91386409",
	521,
	MCL_SECP521R1
};
const struct mcl::EcParam NIST_P192 = {
	"NIST_P192",
	"0xfffffffffffffffffffffffffffffffeffffffffffffffff",
	"-3",
	"0x64210519e59c80e70fa7e9ab72243049feb8deecc146b9b1",
	"0x188da80eb03090f67cbf20eb43a18800f4ff0afd82ff1012",
	"0x07192b95ffc8da78631011ed6b24cdd573f977a11e794811",
	"0xffffffffffffffffffffffff99def836146bc9b1b4d22831",
	192,
	MCL_NIST_P192
};
const struct mcl::EcParam NIST_P224 = {
	"NIST_P224",
	"0xffffffffffffffffffffffffffffffff000000000000000000000001",
	"-3",
	"0xb4050a850c04b3abf54132565044b0b7d7bfd8ba270b39432355ffb4",
	"0xb70e0cbd6bb4bf7f321390b94a03c1d356c21122343280d6115c1d21",
	"0xbd376388b5f723fb4c22dfe6cd4375a05a07476444d5819985007e34",
	"0xffffffffffffffffffffffffffff16a2e0b8f03e13dd29455c5c2a3d",
	224,
	MCL_NIST_P224
};
const struct mcl::EcParam NIST_P256 = {
	"NIST_P256",
	"0xffffffff00000001000000000000000000000000ffffffffffffffffffffffff",
	"-3",
	"0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b",
	"0x6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296",
	"0x4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5",
	"0xffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551",
	256,
	MCL_NIST_P256
};
// same secp384r1
const struct mcl::EcParam NIST_P384 = {
	"NIST_P384",
	"0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff",
	"-3",
	"0xb3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef",
	"0xaa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7",
	"0x3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f",
	"0xffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973",
	384,
	MCL_NIST_P384
};
// same secp521r1
const struct mcl::EcParam NIST_P521 = {
	"NIST_P521",
	"0x1ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
	"-3",
	"0x051953eb9618e1c9a1f929a21a0b68540eea2da725b99b315f3b8b489918ef109e156193951ec7e937b1652c0bd3bb1bf073573df883d2c34f1ef451fd46b503f00",
	"0xc6858e06b70404e9cd9e3ecb662395b4429c648139053fb521f828af606b4d3dbaa14b5e77efe75928fe1dc127a2ffa8de3348b3c1856a429bf97e7e31c2e5bd66",
	"0x11839296a789a3bc0045c8a5fb42c7d1bd998f54449579b446817afbd17273e662c97ee72995ef42640c550b9013fad0761353c7086a272c24088be94769fd16650",
	"0x1fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffa51868783bf2f966b7fcc0148f709a5d03bb5c9b8899c47aebb6fb71e91386409",
	521,
	MCL_NIST_P521
};

} // mcl::ecparam

#ifndef CYBOZU_DONT_USE_STRING
static inline const mcl::EcParam* getEcParam(const std::string& name)
{
	static const mcl::EcParam *tbl[] = {
		&ecparam::p160_1,
		&ecparam::secp160k1,
		&ecparam::secp192k1,
		&ecparam::secp224k1,
		&ecparam::secp256k1,
		&ecparam::secp384r1,
		&ecparam::secp521r1,

		&ecparam::NIST_P192,
		&ecparam::NIST_P224,
		&ecparam::NIST_P256,
		&ecparam::NIST_P384,
		&ecparam::NIST_P521,
	};
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		if (name == tbl[i]->name) return tbl[i];
	}
	throw cybozu::Exception("mcl::getEcParam:not support name") << name;
}
#endif

inline const mcl::EcParam* getEcParam(int curve)
{
	switch (curve) {
	case MCL_SECP192K1: return &ecparam::secp192k1;
	case MCL_SECP224K1: return &ecparam::secp224k1;
	case MCL_SECP256K1: return &ecparam::secp256k1;
	case MCL_SECP384R1: return &ecparam::secp384r1;
	case MCL_SECP521R1: return &ecparam::secp521r1;
	case MCL_NIST_P192: return &ecparam::NIST_P192;
	case MCL_NIST_P224: return &ecparam::NIST_P224;
	case MCL_NIST_P256: return &ecparam::NIST_P256;
	case MCL_SECP160K1: return &ecparam::secp160k1;
	case MCL_P160_1: return &ecparam::p160_1;
	default: return 0;
	}
}

} // mcl

#include <mcl/ec.hpp>
