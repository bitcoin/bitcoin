#pragma once
/**
	@file
	@brief preset class for BLS12-381 pairing
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#define MCL_MAX_FP_BIT_SIZE 384
#define MCL_MAX_FR_BIT_SIZE 256
#include <mcl/bn.hpp>

namespace mcl { namespace bls12 {
using namespace mcl::bn;
} }
