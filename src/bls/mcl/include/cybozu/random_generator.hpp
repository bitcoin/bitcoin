#pragma once
/**
	@file
	@brief pseudrandom generator
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/

#ifndef CYBOZU_DONT_USE_EXCEPTION
#include <cybozu/exception.hpp>
#endif
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincrypt.h>
#ifdef _MSC_VER
#pragma comment (lib, "advapi32.lib")
#endif
#else
#include <sys/types.h>
#include <fcntl.h>
#endif

namespace cybozu {

class RandomGenerator {
	RandomGenerator(const RandomGenerator&);
	void operator=(const RandomGenerator&);
public:
#ifdef _WIN32
	RandomGenerator()
		: prov_(0)
	{
		DWORD flagTbl[] = { CRYPT_VERIFYCONTEXT | CRYPT_SILENT, 0, CRYPT_MACHINE_KEYSET };
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(flagTbl); i++) {
			if (CryptAcquireContext(&prov_, NULL, NULL, PROV_RSA_FULL, flagTbl[i]) != 0) return;
		}
#ifdef CYBOZU_DONT_USE_EXCEPTION
		prov_ = 0;
#else
		throw cybozu::Exception("randomgenerator");
#endif
	}
	bool read_inner(void *buf, size_t byteSize)
	{
		if (prov_ == 0) return false;
		return CryptGenRandom(prov_, static_cast<DWORD>(byteSize), static_cast<BYTE*>(buf)) != 0;
	}
	~RandomGenerator()
	{
		if (prov_) {
			CryptReleaseContext(prov_, 0);
		}
	}
	/*
		fill buf[0..bufNum-1] with random data
		@note bufNum is not byte size
	*/
	template<class T>
	void read(bool *pb, T *buf, size_t bufNum)
	{
		const size_t byteSize = sizeof(T) * bufNum;
		*pb = read_inner(buf, byteSize);
	}
private:
	HCRYPTPROV prov_;
#else
	RandomGenerator()
		: fp_(::fopen("/dev/urandom", "rb"))
	{
#ifndef CYBOZU_DONT_USE_EXCEPTION
		if (!fp_) throw cybozu::Exception("randomgenerator");
#endif
	}
	~RandomGenerator()
	{
		if (fp_) ::fclose(fp_);
	}
	/*
		fill buf[0..bufNum-1] with random data
		@note bufNum is not byte size
	*/
	template<class T>
	void read(bool *pb, T *buf, size_t bufNum)
	{
		if (fp_ == 0) {
			*pb = false;
			return;
		}
		const size_t byteSize = sizeof(T) * bufNum;
		*pb = ::fread(buf, 1, (int)byteSize, fp_) == byteSize;
	}
private:
	FILE *fp_;
#endif
#ifndef CYBOZU_DONT_USE_EXCEPTION
public:
	template<class T>
	void read(T *buf, size_t bufNum)
	{
		bool b;
		read(&b, buf, bufNum);
		if (!b) throw cybozu::Exception("RandomGenerator:read") << bufNum;
	}
	uint32_t get32()
	{
		uint32_t ret;
		read(&ret, 1);
		return ret;
	}
	uint64_t get64()
	{
		uint64_t ret;
		read(&ret, 1);
		return ret;
	}
	uint32_t operator()()
	{
		return get32();
	}
#endif
};

template<class T, class RG>
void shuffle(T* v, size_t n, RG& rg)
{
	if (n <= 1) return;
	for (size_t i = 0; i < n - 1; i++) {
		size_t r = i + size_t(rg.get64() % (n - i));
		using namespace std;
		swap(v[i], v[r]);
	}
}

template<class V, class RG>
void shuffle(V& v, RG& rg)
{
	shuffle(v.data(), v.size(), rg);
}

} // cybozu
