#pragma once
/**
	@file
	@brief constant macro
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <cybozu/inttype.hpp>

#ifndef MCL_SIZEOF_UNIT
	#if defined(CYBOZU_OS_BIT) && (CYBOZU_OS_BIT == 32)
		#define MCL_SIZEOF_UNIT 4
	#else
		#define MCL_SIZEOF_UNIT 8
	#endif
#endif

#ifndef MCL_MAX_BIT_SIZE
	#define MCL_MAX_BIT_SIZE 512
#endif

namespace mcl {

namespace fp {

#if MCL_SIZEOF_UNIT == 8
typedef uint64_t Unit;
#else
typedef uint32_t Unit;
#endif
#define MCL_UNIT_BIT_SIZE (MCL_SIZEOF_UNIT * 8)

} } // mcl::fp
