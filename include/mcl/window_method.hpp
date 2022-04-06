#pragma once
/**
	@file
	@brief window method
	@author MITSUNARI Shigeo(@herumi)
*/
#include <mcl/array.hpp>
#include <mcl/util.hpp>
#include <mcl/op.hpp>
#include <assert.h>

namespace mcl { namespace fp {

template<class Ec>
class WindowMethod {
public:
	size_t bitSize_;
	size_t winSize_;
	mcl::Array<Ec> tbl_;
	WindowMethod(const Ec& x, size_t bitSize, size_t winSize)
	{
		init(x, bitSize, winSize);
	}
	WindowMethod()
		: bitSize_(0)
		, winSize_(0)
	{
	}
	/*
		@param x [in] base index
		@param bitSize [in] exponent bit length
		@param winSize [in] window size
	*/
	void init(bool *pb, const Ec& x, size_t bitSize, size_t winSize)
	{
		bitSize_ = bitSize;
		winSize_ = winSize;
		const size_t tblNum = (bitSize + winSize - 1) / winSize;
		const size_t r = size_t(1) << winSize;
		*pb = tbl_.resize(tblNum * r);
		if (!*pb) return;
		Ec t(x);
		for (size_t i = 0; i < tblNum; i++) {
			Ec* w = &tbl_[i * r];
			w[0].clear();
			for (size_t d = 1; d < r; d *= 2) {
				for (size_t j = 0; j < d; j++) {
					Ec::add(w[j + d], w[j], t);
				}
				Ec::dbl(t, t);
			}
			for (size_t j = 0; j < r; j++) {
				w[j].normalize();
			}
		}
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	void init(const Ec& x, size_t bitSize, size_t winSize)
	{
		bool b;
		init(&b, x, bitSize, winSize);
		if (!b) throw cybozu::Exception("mcl:WindowMethod:init") << bitSize << winSize;
	}
#endif
	/*
		@param z [out] x multiplied by y
		@param y [in] exponent
	*/
	template<class tag2, size_t maxBitSize2, template<class tag2_, size_t maxBitSize2_> class FpT>
	void mul(Ec& z, const FpT<tag2, maxBitSize2>& y) const
	{
		fp::Block b;
		y.getBlock(b);
		powArray(z, b.p, b.n, false);
	}
	void mul(Ec& z, int64_t y) const
	{
#if MCL_SIZEOF_UNIT == 8
		Unit u = fp::abs_(y);
		powArray(z, &u, 1, y < 0);
#else
		uint64_t ua = fp::abs_(y);
		Unit u[2] = { uint32_t(ua), uint32_t(ua >> 32) };
		size_t un = u[1] ? 2 : 1;
		powArray(z, u, un, y < 0);
#endif
	}
	void mul(Ec& z, const mpz_class& y) const
	{
		powArray(z, gmp::getUnit(y), gmp::getUnitSize(y), y < 0);
	}
	void powArray(Ec& z, const Unit* y, size_t n, bool isNegative) const
	{
		z.clear();
		while (n > 0) {
			if (y[n - 1]) break;
			n--;
		}
		if (n == 0) return;
		assert((n << winSize_) <= tbl_.size());
		if ((n << winSize_) > tbl_.size()) return;
		assert(y[n - 1]);
		size_t i = 0;
		BitIterator<Unit> ai(y, n);
		do {
			Unit v = ai.getNext(winSize_);
			if (v) {
				Ec::add(z, z, tbl_[(i << winSize_) + v]);
			}
			i++;
		} while (ai.hasNext());
		if (isNegative) {
			Ec::neg(z, z);
		}
	}
};

} } // mcl::fp

