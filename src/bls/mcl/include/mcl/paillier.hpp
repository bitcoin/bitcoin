#pragma once
/**
	@file
	@brief paillier encryption
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <mcl/gmp_util.hpp>

namespace mcl { namespace paillier {

class PublicKey {
	size_t primeBitSize;
	mpz_class g;
	mpz_class n;
	mpz_class n2;
public:
	PublicKey() : primeBitSize(0) {}
	void init(size_t _primeBitSize, const mpz_class& _n)
	{
		primeBitSize = _primeBitSize;
		n = _n;
		g = 1 + _n;
		n2 = _n * _n;
	}
	void enc(mpz_class& c, const mpz_class& m, mcl::fp::RandGen rg = mcl::fp::RandGen()) const
	{
		if (rg.isZero()) rg = mcl::fp::RandGen::get();
		if (primeBitSize == 0) throw cybozu::Exception("paillier:PublicKey:not init");
		mpz_class r;
		mcl::gmp::getRand(r, primeBitSize, rg);
		mpz_class a, b;
		mcl::gmp::powMod(a, g, m, n2);
		mcl::gmp::powMod(b, r, n, n2);
		c = (a * b) % n2;
	}
	/*
		additive homomorphic encryption
		cz = cx + cy
	*/
	void add(mpz_class& cz, mpz_class& cx, mpz_class& cy) const
	{
		cz = (cx * cy) % n2;
	}
};

class SecretKey {
	size_t primeBitSize;
	mpz_class n;
	mpz_class n2;
	mpz_class lambda;
	mpz_class invLambda;
public:
	SecretKey() : primeBitSize(0) {}
	/*
		the size of prime is half of bitSize
	*/
	void init(size_t bitSize, mcl::fp::RandGen rg = mcl::fp::RandGen())
	{
		if (rg.isZero()) rg = mcl::fp::RandGen::get();
		primeBitSize = bitSize / 2;
		mpz_class p, q;
		mcl::gmp::getRandPrime(p, primeBitSize, rg);
		mcl::gmp::getRandPrime(q, primeBitSize, rg);
		lambda = (p - 1) * (q - 1);
		n = p * q;
		n2 = n * n;
		mcl::gmp::invMod(invLambda, lambda, n);
	}
	void getPublicKey(PublicKey& pub) const
	{
		pub.init(primeBitSize, n);
	}
	void dec(mpz_class& m, const mpz_class& c) const
	{
		mpz_class L;
		mcl::gmp::powMod(L, c, lambda, n2);
		L = ((L - 1) / n) % n;
		m = (L * invLambda) % n;
	}
};

} } // mcl::paillier
