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

#endif	// CRYPTOPP_PCH_H
