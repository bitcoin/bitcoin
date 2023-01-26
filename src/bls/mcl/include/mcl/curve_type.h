#pragma once
/**
	@file
	@brief curve type
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/

enum {
	MCL_BN254 = 0,
	MCL_BN381_1 = 1,
	MCL_BN381_2 = 2,
	MCL_BN462 = 3,
	MCL_BN_SNARK1 = 4,
	MCL_BLS12_381 = 5,
	MCL_BN160 = 6,
	MCL_BLS12_461 = 7,
	MCL_BLS12_377 = 8,

	/*
		for only G1
		the size of curve must be <= MCLBN_FP_UNIT_SIZE
	*/
	MCL_EC_BEGIN = 100,
	MCL_SECP192K1 = MCL_EC_BEGIN,
	MCL_SECP224K1 = 101,
	MCL_SECP256K1 = 102,
	MCL_SECP384R1 = 103,
	MCL_SECP521R1 = 104,
	MCL_NIST_P192 = 105,
	MCL_NIST_P224 = 106,
	MCL_NIST_P256 = 107,
	MCL_SECP160K1 = 108,
	MCL_P160_1 = 109,
	MCL_EC_END = MCL_P160_1 + 1,
	MCL_NIST_P384 = MCL_SECP384R1,
	MCL_NIST_P521 = MCL_SECP521R1
};

/*
	remark : if irtf-cfrg-hash-to-curve is compeletely fixed, then
	MCL_MAP_TO_MODE_WB19, MCL_MAP_TO_MODE_HASH_TO_CURVE_0? will be removed and
	only MCL_MAP_TO_MODE_HASH_TO_CURVE will be available.
*/
enum {
	MCL_MAP_TO_MODE_ORIGINAL, // see MapTo::calcBN
	MCL_MAP_TO_MODE_TRY_AND_INC, // try-and-incremental-x
	MCL_MAP_TO_MODE_ETH2, // (deprecated) old eth2.0 spec
	MCL_MAP_TO_MODE_WB19, // (deprecated) used in new eth2.0 spec
	MCL_MAP_TO_MODE_HASH_TO_CURVE_05 = MCL_MAP_TO_MODE_WB19, // (deprecated) draft-irtf-cfrg-hash-to-curve-05
	MCL_MAP_TO_MODE_HASH_TO_CURVE_06, // (deprecated) draft-irtf-cfrg-hash-to-curve-06
	MCL_MAP_TO_MODE_HASH_TO_CURVE_07 = 5, /* don't change this value! */ // draft-irtf-cfrg-hash-to-curve-07
	MCL_MAP_TO_MODE_HASH_TO_CURVE = MCL_MAP_TO_MODE_HASH_TO_CURVE_07, // the latset version
	MCL_MAP_TO_MODE_ETH2_LEGACY // backwards compatible version of MCL_MAP_TO_MODE_ETH2 with commit 730c50d4eaff1e0d685a92ac8c896e873749471b
};

#ifdef __cplusplus

#include <string.h>
#include <assert.h>

namespace mcl {

struct CurveParam {
	/*
		y^2 = x^3 + b
		i^2 = -1
		xi = xi_a + i
		v^3 = xi
		w^2 = v
	*/
	const char *z;
	int b; // y^2 = x^3 + b
	int xi_a; // xi = xi_a + i
	/*
		BN254, BN381 : Dtype
		BLS12-381 : Mtype
	*/
	bool isMtype;
	int curveType; // same in curve_type.h
	bool operator==(const CurveParam& rhs) const
	{
		return strcmp(z, rhs.z) == 0 && b == rhs.b && xi_a == rhs.xi_a && isMtype == rhs.isMtype;
	}
	bool operator!=(const CurveParam& rhs) const { return !operator==(rhs); }
};

const CurveParam BN254 = { "-0x4080000000000001", 2, 1, false, MCL_BN254 }; // -(2^62 + 2^55 + 1)
// provisional(experimental) param with maxBitSize = 384
const CurveParam BN381_1 = { "-0x400011000000000000000001", 2, 1, false, MCL_BN381_1 }; // -(2^94 + 2^76 + 2^72 + 1) // A Family of Implementation-Friendly BN Elliptic Curves
const CurveParam BN381_2 = { "-0x400040090001000000000001", 2, 1, false, MCL_BN381_2 }; // -(2^94 + 2^78 + 2^67 + 2^64 + 2^48 + 1) // used in relic-toolkit
const CurveParam BN462 = { "0x4001fffffffffffffffffffffbfff", 5, 2, false, MCL_BN462 }; // 2^114 + 2^101 - 2^14 - 1 // https://eprint.iacr.org/2017/334
const CurveParam BN_SNARK1 = { "4965661367192848881", 3, 9, false, MCL_BN_SNARK1 };
const CurveParam BLS12_381 = { "-0xd201000000010000", 4, 1, true, MCL_BLS12_381 };
const CurveParam BN160 = { "0x4000000031", 3, 4, false, MCL_BN160 };
const CurveParam BLS12_461 = { "-0x1ffffffbfffe00000000", 4, 1, true, MCL_BLS12_461 };
const CurveParam BLS12_377 = { "0x8508c00000000001", 4, 1, true, MCL_BLS12_377 }; // not supported yet

#ifdef __clang__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wreturn-type-c-linkage"
#endif
inline const CurveParam* getCurveParam(int type)
{
	switch (type) {
	case MCL_BN254: return &mcl::BN254;
	case MCL_BN381_1: return &mcl::BN381_1;
	case MCL_BN381_2: return &mcl::BN381_2;
	case MCL_BN462: return &mcl::BN462;
	case MCL_BN_SNARK1: return &mcl::BN_SNARK1;
	case MCL_BLS12_381: return &mcl::BLS12_381;
	case MCL_BN160: return &mcl::BN160;
	default:
		return 0;
	}
}
#ifdef __clang__
	#pragma GCC diagnostic pop
#endif

} // mcl
#endif
