#pragma once
/**
	@file
	@brief XorShift

	@author MITSUNARI Shigeo(@herumi)
	@author MITSUNARI Shigeo
*/
#include <cybozu/inttype.hpp>
#include <assert.h>

namespace cybozu {

namespace xorshift_local {

/*
	U is uint32_t or uint64_t
*/
template<class U, class Gen>
void read_local(void *p, size_t n, Gen& gen, U (Gen::*f)())
{
	uint8_t *dst = static_cast<uint8_t*>(p);
	const size_t uSize = sizeof(U);
	assert(uSize == 4 || uSize == 8);
	union ua {
		U u;
		uint8_t a[uSize];
	};

	while (n >= uSize) {
		ua ua;
		ua.u = (gen.*f)();
		for (size_t i = 0; i < uSize; i++) {
			dst[i] = ua.a[i];
		}
		dst += uSize;
		n -= uSize;
	}
	assert(n < uSize);
	if (n > 0) {
		ua ua;
		ua.u = (gen.*f)();
		for (size_t i = 0; i < n; i++) {
			dst[i] = ua.a[i];
		}
	}
}

} // xorshift_local

class XorShift {
	uint32_t x_, y_, z_, w_;
public:
	explicit XorShift(uint32_t x = 0, uint32_t y = 0, uint32_t z = 0, uint32_t w = 0)
	{
		init(x, y, z, w);
	}
	void init(uint32_t x = 0, uint32_t y = 0, uint32_t z = 0, uint32_t w = 0)
	{
		x_ = x ? x : 123456789;
		y_ = y ? y : 362436069;
		z_ = z ? z : 521288629;
		w_ = w ? w : 88675123;
	}
	uint32_t get32()
	{
		unsigned int t = x_ ^ (x_ << 11);
		x_ = y_; y_ = z_; z_ = w_;
		return w_ = (w_ ^ (w_ >> 19)) ^ (t ^ (t >> 8));
	}
	uint32_t operator()() { return get32(); }
	uint64_t get64()
	{
		uint32_t a = get32();
		uint32_t b = get32();
		return (uint64_t(a) << 32) | b;
	}
	template<class T>
	void read(bool *pb, T *p, size_t n)
	{
		xorshift_local::read_local(p, n * sizeof(T), *this, &XorShift::get32);
		*pb = true;
	}
	template<class T>
	size_t read(T *p, size_t n)
	{
		bool b;
		read(&b, p, n);
		(void)b;
		return n;
	}
};

// see http://xorshift.di.unimi.it/xorshift128plus.c
class XorShift128Plus {
	uint64_t s_[2];
	static const uint64_t seed0 = 123456789;
	static const uint64_t seed1 = 987654321;
public:
	explicit XorShift128Plus(uint64_t s0 = seed0, uint64_t s1 = seed1)
	{
		init(s0, s1);
	}
	void init(uint64_t s0 = seed0, uint64_t s1 = seed1)
	{
		s_[0] = s0;
		s_[1] = s1;
	}
	uint32_t get32()
	{
		return static_cast<uint32_t>(get64());
	}
	uint64_t operator()() { return get64(); }
	uint64_t get64()
	{
		uint64_t s1 = s_[0];
		const uint64_t s0 = s_[1];
		s_[0] = s0;
		s1 ^= s1 << 23;
		s_[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
		return s_[1] + s0;
	}
	template<class T>
	void read(bool *pb, T *p, size_t n)
	{
		xorshift_local::read_local(p, n * sizeof(T), *this, &XorShift128Plus::get64);
		*pb = true;
	}
	template<class T>
	size_t read(T *p, size_t n)
	{
		bool b;
		read(&b, p, n);
		(void)b;
		return n;
	}
};

// see http://xoroshiro.di.unimi.it/xoroshiro128plus.c
class Xoroshiro128Plus {
	uint64_t s_[2];
	static const uint64_t seed0 = 123456789;
	static const uint64_t seed1 = 987654321;
	uint64_t rotl(uint64_t x, unsigned int k) const
	{
		return (x << k) | (x >> (64 - k));
	}
public:
	explicit Xoroshiro128Plus(uint64_t s0 = seed0, uint64_t s1 = seed1)
	{
		init(s0, s1);
	}
	void init(uint64_t s0 = seed0, uint64_t s1 = seed1)
	{
		s_[0] = s0;
		s_[1] = s1;
	}
	uint32_t get32()
	{
		return static_cast<uint32_t>(get64());
	}
	uint64_t operator()() { return get64(); }
	uint64_t get64()
	{
		uint64_t s0 = s_[0];
		uint64_t s1 = s_[1];
		uint64_t result = s0 + s1;
		s1 ^= s0;
		s_[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14);
		s_[1] = rotl(s1, 36);
		return result;
	}
	template<class T>
	void read(bool *pb, T *p, size_t n)
	{
		xorshift_local::read_local(p, n * sizeof(T), *this, &Xoroshiro128Plus::get64);
		*pb = true;
	}
	template<class T>
	size_t read(T *p, size_t n)
	{
		bool b;
		read(&b, p, n);
		(void)b;
		return n;
	}
};

} // cybozu
