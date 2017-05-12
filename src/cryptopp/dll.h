// dll.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile dll.h
//! \brief Functions and definitions required for building the FIPS-140 DLL on Windows

#ifndef CRYPTOPP_DLL_H
#define CRYPTOPP_DLL_H

#if !defined(CRYPTOPP_IMPORTS) && !defined(CRYPTOPP_EXPORTS) && !defined(CRYPTOPP_DEFAULT_NO_DLL)
#ifdef CRYPTOPP_CONFIG_H
#error To use the DLL version of Crypto++, this file must be included before any other Crypto++ header files.
#endif
#define CRYPTOPP_IMPORTS
#endif

#include "aes.h"
#include "cbcmac.h"
#include "ccm.h"
#include "cmac.h"
#include "channels.h"
#include "des.h"
#include "dh.h"
#include "dsa.h"
#include "ec2n.h"
#include "eccrypto.h"
#include "ecp.h"
#include "files.h"
#include "fips140.h"
#include "gcm.h"
#include "hex.h"
#include "hmac.h"
#include "modes.h"
#include "mqueue.h"
#include "nbtheory.h"
#include "osrng.h"
#include "pkcspad.h"
#include "pssr.h"
#include "randpool.h"
#include "rsa.h"
#include "rw.h"
#include "sha.h"
#include "skipjack.h"
#include "trdlocal.h"

#ifdef CRYPTOPP_IMPORTS

#ifdef _DLL
// cause CRT DLL to be initialized before Crypto++ so that we can use malloc and free during DllMain()
#ifdef CRYPTOPP_DEBUG
# pragma comment(lib, "msvcrtd")
# pragma comment(lib, "cryptopp")
#else
# pragma comment(lib, "msvcrt")
# pragma comment(lib, "cryptopp")
#endif
#endif

#endif		// #ifdef CRYPTOPP_IMPORTS

#include <new>	// for new_handler

NAMESPACE_BEGIN(CryptoPP)

#if !(defined(_MSC_VER) && (_MSC_VER < 1300))
using std::new_handler;
#endif

typedef void * (CRYPTOPP_API * PNew)(size_t);
typedef void (CRYPTOPP_API * PDelete)(void *);
typedef void (CRYPTOPP_API * PGetNewAndDelete)(PNew &, PDelete &);
typedef new_handler (CRYPTOPP_API * PSetNewHandler)(new_handler);
typedef void (CRYPTOPP_API * PSetNewAndDelete)(PNew, PDelete, PSetNewHandler);

NAMESPACE_END

#endif
