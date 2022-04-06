#pragma once
/**
	@file
	@brief definition of Op
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#ifdef MCL_DONT_USE_CSPRNG

// nothing

#elif defined(MCL_USE_WEB_CRYPTO_API)
#include <emscripten.h>

namespace mcl {
struct RandomGeneratorJS {
	void read(bool *pb, void *buf, uint32_t byteSize)
	{
		// cf. https://developer.mozilla.org/en-US/docs/Web/API/Crypto/getRandomValues
		if (byteSize > 65536) {
			*pb = false;
			return;
		}
		// use crypto.getRandomValues
		EM_ASM({Module.cryptoGetRandomValues($0, $1)}, buf, byteSize);
		*pb = true;
	}
};
} // mcl

#else
#include <cybozu/random_generator.hpp>
#if 0 // #if CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11
#include <random>
#endif
#endif
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4521)
#endif
namespace mcl { namespace fp {

namespace local {

template<class RG>
uint32_t readWrapper(void *self, void *buf, uint32_t byteSize)
{
	bool b;
	reinterpret_cast<RG*>(self)->read(&b, (uint8_t*)buf, byteSize);
	if (b) return byteSize;
	return 0;
}

#if 0 // #if CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11
template<>
inline uint32_t readWrapper<std::random_device>(void *self, void *buf, uint32_t byteSize)
{
	const uint32_t keep = byteSize;
	std::random_device& rg = *reinterpret_cast<std::random_device*>(self);
	uint8_t *p = reinterpret_cast<uint8_t*>(buf);
	uint32_t v;
	while (byteSize >= 4) {
		v = rg();
		memcpy(p, &v, 4);
		p += 4;
		byteSize -= 4;
	}
	if (byteSize > 0) {
		v = rg();
		memcpy(p, &v, byteSize);
	}
	return keep;
}
#endif
} // local
/*
	wrapper of cryptographically secure pseudo random number generator
*/
class RandGen {
	typedef uint32_t (*readFuncType)(void *self, void *buf, uint32_t byteSize);
	void *self_;
	readFuncType readFunc_;
public:
	RandGen() : self_(0), readFunc_(0) {}
	RandGen(void *self, readFuncType readFunc) : self_(self) , readFunc_(readFunc) {}
	RandGen(const RandGen& rhs) : self_(rhs.self_), readFunc_(rhs.readFunc_) {}
	RandGen(RandGen& rhs) : self_(rhs.self_), readFunc_(rhs.readFunc_) {}
	RandGen& operator=(const RandGen& rhs)
	{
		self_ = rhs.self_;
		readFunc_ = rhs.readFunc_;
		return *this;
	}
	template<class RG>
	RandGen(RG& rg)
		: self_(reinterpret_cast<void*>(&rg))
		, readFunc_(local::readWrapper<RG>)
	{
	}
	void read(bool *pb, void *out, size_t byteSize)
	{
		uint32_t size = readFunc_(self_, out, static_cast<uint32_t>(byteSize));
		*pb = size == byteSize;
	}
#ifdef MCL_DONT_USE_CSPRNG
	bool isZero() const { return false; } /* return false to avoid copying default rg */
#else
	bool isZero() const { return self_ == 0 && readFunc_ == 0; }
#endif
	static RandGen& getDefaultRandGen()
	{
#ifdef MCL_DONT_USE_CSPRNG
		static RandGen wrg;
#elif defined(MCL_USE_WEB_CRYPTO_API)
		static mcl::RandomGeneratorJS rg;
		static RandGen wrg(rg);
#else
		static cybozu::RandomGenerator rg;
		static RandGen wrg(rg);
#endif
		return wrg;
	}
	static RandGen& get()
	{
		static RandGen wrg(getDefaultRandGen());
		return wrg;
	}
	/*
		rg must be thread safe
		rg.read(void *buf, size_t byteSize);
	*/
	static void setRandGen(const RandGen& rg)
	{
		get() = rg;
	}
	/*
		set rand function
		if self and readFunc are NULL then set default rand function
	*/
	static void setRandFunc(void *self, readFuncType readFunc)
	{
		if (self == 0 && readFunc == 0) {
			setRandGen(getDefaultRandGen());
		} else {
			RandGen rg(self, readFunc);
			setRandGen(rg);
		}
	}
};

} } // mcl::fp

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
