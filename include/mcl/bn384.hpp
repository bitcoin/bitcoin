#pragma once
/**
	@file
	@brief preset class for 384-bit optimal ate pairing over BN curves
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#define MCL_MAX_FP_BIT_SIZE 384
#include <mcl/bn.hpp>
// #define MCL_MAX_FR_BIT_SIZE 256 // can set if BLS12_381

namespace mcl { namespace bn384 {
using namespace mcl::bn;
} }
