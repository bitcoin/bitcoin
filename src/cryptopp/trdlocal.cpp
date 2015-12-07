// trdlocal.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

// TODO: fix this when more complete C++11 support is cut-in
#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4297)
#endif

#ifndef CRYPTOPP_IMPORTS
#ifdef THREADS_AVAILABLE

#include "trdlocal.h"

#ifdef HAS_WINTHREADS
#include <windows.h>
#endif

NAMESPACE_BEGIN(CryptoPP)

ThreadLocalStorage::Err::Err(const std::string& operation, int error)
	: OS_Error(OTHER_ERROR, "ThreadLocalStorage: " + operation + " operation failed with error 0x" + IntToString(error, 16), operation, error)
{
}

ThreadLocalStorage::ThreadLocalStorage()
{
#ifdef HAS_WINTHREADS
	m_index = TlsAlloc();
	assert(m_index != TLS_OUT_OF_INDEXES);
	if (m_index == TLS_OUT_OF_INDEXES)
		throw Err("TlsAlloc", GetLastError());
#else
	m_index = 0;
	int error = pthread_key_create(&m_index, NULL);
	assert(!error);
	if (error)
		throw Err("pthread_key_create", error);
#endif
}

ThreadLocalStorage::~ThreadLocalStorage() CRYPTOPP_THROW
{
#ifdef CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE
	if (!std::uncaught_exception())
#else
	try
#endif
#ifdef HAS_WINTHREADS
	{
		int rc = TlsFree(m_index);
		assert(rc);
		if (!rc)
			throw Err("TlsFree", GetLastError());
	}
#else
	{
		int error = pthread_key_delete(m_index);
		assert(!error);
		if (error)
			throw Err("pthread_key_delete", error);
	}
#endif
#ifndef CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE
	catch(const Exception&)
	{
	}
#endif
}

void ThreadLocalStorage::SetValue(void *value)
{
#ifdef HAS_WINTHREADS
	if (!TlsSetValue(m_index, value))
		throw Err("TlsSetValue", GetLastError());
#else
	int error = pthread_setspecific(m_index, value);
	if (error)
		throw Err("pthread_key_getspecific", error);
#endif
}

void *ThreadLocalStorage::GetValue() const
{
#ifdef HAS_WINTHREADS
	void *result = TlsGetValue(m_index);
	const DWORD dwRet = GetLastError();

	assert(result || (!result && (dwRet == NO_ERROR)));
	if (!result && dwRet != NO_ERROR)
		throw Err("TlsGetValue", dwRet);
#else
	// Null is a valid return value. Posix does not provide a way to
	//  check for a "good" Null vs a "bad" Null (errno is not set).
	void *result = pthread_getspecific(m_index);
#endif
	return result;
}

NAMESPACE_END

#endif	// #ifdef THREADS_AVAILABLE
#endif
