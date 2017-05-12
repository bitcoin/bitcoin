// osrng.cpp - written and placed in the public domain by Wei Dai

// Thanks to Leonard Janke for the suggestion for AutoSeededRandomPool.

#include "pch.h"
#include "config.h"

#ifndef CRYPTOPP_IMPORTS

// Win32 has CryptoAPI and <wincrypt.h>. Windows 10 and Windows Store 10 have CNG and <bcrypt.h>.
//  There's a hole for Windows Phone 8 and Windows Store 8. There is no userland crypto available.
//  Also see http://stackoverflow.com/questions/36974545/random-numbers-for-windows-phone-8-and-windows-store-8
#if defined(CRYPTOPP_WIN32_AVAILABLE) && !defined(OS_RNG_AVAILABLE)
# pragma message("WARNING: Compiling for Windows but an OS RNG is not available. This is likely a Windows Phone 8 or Windows Store 8 app.")
#endif

#if !defined(OS_NO_DEPENDENCE) && defined(OS_RNG_AVAILABLE)

#include "osrng.h"
#include "rng.h"

#ifdef CRYPTOPP_WIN32_AVAILABLE
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
//#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if defined(USE_MS_CRYPTOAPI)
#include <wincrypt.h>
#ifndef CRYPT_NEWKEYSET
# define CRYPT_NEWKEYSET 0x00000008
#endif
#ifndef CRYPT_MACHINE_KEYSET
# define CRYPT_MACHINE_KEYSET 0x00000020
#endif
#elif defined(USE_MS_CNGAPI)
//#include <ntdef.h>
#include <bcrypt.h>
#ifndef BCRYPT_SUCCESS
# define BCRYPT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif
#ifndef STATUS_INVALID_PARAMETER
# define STATUS_INVALID_PARAMETER 0xC000000D
#endif
#ifndef STATUS_INVALID_HANDLE
# define STATUS_INVALID_HANDLE 0xC0000008
#endif
#endif
#endif

#ifdef CRYPTOPP_UNIX_AVAILABLE
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif

NAMESPACE_BEGIN(CryptoPP)

#if defined(NONBLOCKING_RNG_AVAILABLE) || defined(BLOCKING_RNG_AVAILABLE)
OS_RNG_Err::OS_RNG_Err(const std::string &operation)
	: Exception(OTHER_ERROR, "OS_Rng: " + operation + " operation failed with error " +
#ifdef CRYPTOPP_WIN32_AVAILABLE
		"0x" + IntToString(GetLastError(), 16)
#else
		IntToString(errno)
#endif
		)
{
}
#endif

#ifdef NONBLOCKING_RNG_AVAILABLE

#ifdef CRYPTOPP_WIN32_AVAILABLE

#if defined(USE_MS_CNGAPI)
inline DWORD NtStatusToErrorCode(NTSTATUS status)
{
	if (status == STATUS_INVALID_PARAMETER)
		return ERROR_INVALID_PARAMETER;
	else if (status == STATUS_INVALID_HANDLE)
		return ERROR_INVALID_HANDLE;
	else
		return (DWORD)status;
}
#endif

#if defined(UNICODE) || defined(_UNICODE)
# define CRYPTOPP_CONTAINER L"Crypto++ RNG"
#else
# define CRYPTOPP_CONTAINER "Crypto++ RNG"
#endif

MicrosoftCryptoProvider::MicrosoftCryptoProvider() : m_hProvider(0)
{
#if defined(USE_MS_CRYPTOAPI)
	// See http://support.microsoft.com/en-us/kb/238187 for CRYPT_NEWKEYSET fallback strategy
	if (!CryptAcquireContext(&m_hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		const DWORD firstErr = GetLastError();
		if (!CryptAcquireContext(&m_hProvider, CRYPTOPP_CONTAINER, 0, PROV_RSA_FULL, CRYPT_NEWKEYSET /*user*/) &&
		    !CryptAcquireContext(&m_hProvider, CRYPTOPP_CONTAINER, 0, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_NEWKEYSET))
		{
			// Set original error with original code
			SetLastError(firstErr);
			throw OS_RNG_Err("CryptAcquireContext");
		}
	}
#elif defined(USE_MS_CNGAPI)
	NTSTATUS ret = BCryptOpenAlgorithmProvider(&m_hProvider, BCRYPT_RNG_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
	if (!(BCRYPT_SUCCESS(ret)))
	{
		// Hack... OS_RNG_Err calls GetLastError()
		SetLastError(NtStatusToErrorCode(ret));
		throw OS_RNG_Err("BCryptOpenAlgorithmProvider");
	}
#endif
}

MicrosoftCryptoProvider::~MicrosoftCryptoProvider()
{
#if defined(USE_MS_CRYPTOAPI)
	if (m_hProvider)
		CryptReleaseContext(m_hProvider, 0);
#elif defined(USE_MS_CNGAPI)
	if (m_hProvider)
		BCryptCloseAlgorithmProvider(m_hProvider, 0);
#endif
}

#endif  // CRYPTOPP_WIN32_AVAILABLE

NonblockingRng::NonblockingRng()
{
#ifndef CRYPTOPP_WIN32_AVAILABLE
	m_fd = open("/dev/urandom",O_RDONLY);
	if (m_fd == -1)
		throw OS_RNG_Err("open /dev/urandom");
#endif
}

NonblockingRng::~NonblockingRng()
{
#ifndef CRYPTOPP_WIN32_AVAILABLE
	close(m_fd);
#endif
}

void NonblockingRng::GenerateBlock(byte *output, size_t size)
{
#ifdef CRYPTOPP_WIN32_AVAILABLE
	// Acquiring a provider is expensive. Do it once and retain the reference.
	static const MicrosoftCryptoProvider &hProvider = Singleton<MicrosoftCryptoProvider>().Ref();
# if defined(USE_MS_CRYPTOAPI)
	if (!CryptGenRandom(hProvider.GetProviderHandle(), (DWORD)size, output))
		throw OS_RNG_Err("CryptGenRandom");
# elif defined(USE_MS_CNGAPI)
	NTSTATUS ret = BCryptGenRandom(hProvider.GetProviderHandle(), output, (ULONG)size, 0);
	if (!(BCRYPT_SUCCESS(ret)))
	{
		// Hack... OS_RNG_Err calls GetLastError()
		SetLastError(NtStatusToErrorCode(ret));
		throw OS_RNG_Err("BCryptGenRandom");
	}
# endif
#else
	while (size)
	{
		ssize_t len = read(m_fd, output, size);
		if (len < 0)
		{
			// /dev/urandom reads CAN give EAGAIN errors! (maybe EINTR as well)
			if (errno != EINTR && errno != EAGAIN)
				throw OS_RNG_Err("read /dev/urandom");

			continue;
		}

		output += len;
		size -= len;
	}
#endif  // CRYPTOPP_WIN32_AVAILABLE
}

#endif  // NONBLOCKING_RNG_AVAILABLE

// *************************************************************

#ifdef BLOCKING_RNG_AVAILABLE

#ifndef CRYPTOPP_BLOCKING_RNG_FILENAME
#ifdef __OpenBSD__
#define CRYPTOPP_BLOCKING_RNG_FILENAME "/dev/srandom"
#else
#define CRYPTOPP_BLOCKING_RNG_FILENAME "/dev/random"
#endif
#endif

BlockingRng::BlockingRng()
{
	m_fd = open(CRYPTOPP_BLOCKING_RNG_FILENAME,O_RDONLY);
	if (m_fd == -1)
		throw OS_RNG_Err("open " CRYPTOPP_BLOCKING_RNG_FILENAME);
}

BlockingRng::~BlockingRng()
{
	close(m_fd);
}

void BlockingRng::GenerateBlock(byte *output, size_t size)
{
	while (size)
	{
		// on some systems /dev/random will block until all bytes
		// are available, on others it returns immediately
		ssize_t len = read(m_fd, output, size);
		if (len < 0)
		{
			// /dev/random reads CAN give EAGAIN errors! (maybe EINTR as well)
			if (errno != EINTR && errno != EAGAIN)
				throw OS_RNG_Err("read " CRYPTOPP_BLOCKING_RNG_FILENAME);

			continue;
		}

		size -= len;
		output += len;
		if (size)
			sleep(1);
	}
}

#endif  // BLOCKING_RNG_AVAILABLE

// *************************************************************

void OS_GenerateRandomBlock(bool blocking, byte *output, size_t size)
{
#ifdef NONBLOCKING_RNG_AVAILABLE
	if (blocking)
#endif
	{
#ifdef BLOCKING_RNG_AVAILABLE
		BlockingRng rng;
		rng.GenerateBlock(output, size);
#endif
	}

#ifdef BLOCKING_RNG_AVAILABLE
	if (!blocking)
#endif
	{
#ifdef NONBLOCKING_RNG_AVAILABLE
		NonblockingRng rng;
		rng.GenerateBlock(output, size);
#endif
	}
}

void AutoSeededRandomPool::Reseed(bool blocking, unsigned int seedSize)
{
	SecByteBlock seed(seedSize);
	OS_GenerateRandomBlock(blocking, seed, seedSize);
	IncorporateEntropy(seed, seedSize);
}

NAMESPACE_END

#endif  // OS_RNG_AVAILABLE

#endif  // CRYPTOPP_IMPORTS
