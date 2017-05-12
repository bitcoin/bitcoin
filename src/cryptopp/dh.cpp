// dh.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "dh.h"

NAMESPACE_BEGIN(CryptoPP)

#if CRYPTOPP_DEBUG && !defined(CRYPTOPP_DOXYGEN_PROCESSING)
void DH_TestInstantiations()
{
	DH dh1;
	DH dh2(NullRNG(), 10);
}
#endif

NAMESPACE_END

#endif
