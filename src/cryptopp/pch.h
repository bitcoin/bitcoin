// pch.h - written and placed in the public domain by Wei Dai

//! \headerfile pch.h
//! \brief Precompiled header file

#ifndef CRYPTOPP_PCH_H
#define CRYPTOPP_PCH_H

# ifdef CRYPTOPP_GENERATE_X64_MASM
	#include "cpu.h"

# else
	#include "config.h"

	#ifdef USE_PRECOMPILED_HEADERS
		#include "simple.h"
		#include "secblock.h"
		#include "misc.h"
		#include "smartptr.h"
		#include "stdcpp.h"
	#endif
# endif

// Enable file and line numbers, if available.
// #if defined(_MSC_VER) && defined(_DEBUG) && defined(USE_PRECOMPILED_HEADERS)
// # define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
// # define new DEBUG_NEW
// #endif

#endif	// CRYPTOPP_PCH_H
