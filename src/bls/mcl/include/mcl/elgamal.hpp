#pragma once
/**
	@file
	@brief lifted-ElGamal encryption
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause

	original:
	Copyright (c) 2014, National Institute of Advanced Industrial
	Science and Technology All rights reserved.
	This source file is subject to BSD 3-Clause license.
*/
#include <string>
#include <sstream>
#include <cybozu/unordered_map.hpp>
#ifndef CYBOZU_UNORDERED_MAP_STD
#include <map>
#endif
#include <cybozu/exception.hpp>
#include <cybozu/itoa.hpp>
#include <cybozu/atoi.hpp>
#include <mcl/window_method.hpp>

namespace mcl {

template<class _Ec, class Zn>
struct ElgamalT {
	typedef _Ec Ec;
	struct CipherText {
		Ec c1;
		Ec c2;
		CipherText()
		{
			clear();
		}
		/*
			(c1, c2) = (0, 0) is trivial valid ciphertext for m = 0
		*/
		void clear()
		{
			c1.clear();
			c2.clear();
		}
		/*
			add encoded message with encoded message
			input : this = Enc(m1), c = Enc(m2)
			output : this = Enc(m1 + m2)
		*/
		void add(const CipherText& c)
		{
			Ec::add(c1, c1, c.c1);
			Ec::add(c2, c2, c.c2);
		}
		/*
			mul by x
			input : this = Enc(m), x
			output : this = Enc(m x)
		*/
		template<class N>
		void mul(const N& x)
		{
			Ec::mul(c1, c1, x);
			Ec::mul(c2, c2, x);
		}
		/*
			negative encoded message
			input : this = Enc(m)
			output : this = Enc(-m)
		*/
		void neg()
		{
			Ec::neg(c1, c1);
			Ec::neg(c2, c2);
		}
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			c1.load(is, ioMode);
			c2.load(is, ioMode);
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			const char sep = *fp::getIoSeparator(ioMode);
			c1.save(os, ioMode);
			if (sep) cybozu::writeChar(os, sep);
			c2.save(os, ioMode);
		}
		void getStr(std::string& str, int ioMode = 0) const
		{
			str.clear();
			cybozu::StringOutputStream os(str);
			save(os, ioMode);
		}
		std::string getStr(int ioMode = 0) const
		{
			std::string str;
			getStr(str, ioMode);
			return str;
		}
		void setStr(const std::string& str, int ioMode = 0)
		{
			cybozu::StringInputStream is(str);
			load(is, ioMode);
		}
		friend inline std::ostream& operator<<(std::ostream& os, const CipherText& self)
		{
			self.save(os, fp::detectIoMode(Ec::getIoMode(), os));
			return os;
		}
		friend inline std::istream& operator>>(std::istream& is, CipherText& self)
		{
			self.load(is, fp::detectIoMode(Ec::getIoMode(), is));
			return is;
		}
		// obsolete
		std::string toStr() const { return getStr(); }
		void fromStr(const std::string& str) { setStr(str); }
	};
	/*
		Zero Knowledge Proof
		cipher text with ZKP to ensure m = 0 or 1
		http://dx.doi.org/10.1587/transfun.E96.A.1156
	*/
	struct Zkp {
		Zn c[2], s[2];
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			c[0].load(is, ioMode);
			c[1].load(is, ioMode);
			s[0].load(is, ioMode);
			s[1].load(is, ioMode);
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			const char sep = *fp::getIoSeparator(ioMode);
			c[0].save(os, ioMode);
			if (sep) cybozu::writeChar(os, sep);
			c[1].save(os, ioMode);
			if (sep) cybozu::writeChar(os, sep);
			s[0].save(os, ioMode);
			if (sep) cybozu::writeChar(os, sep);
			s[1].save(os, ioMode);
		}
		void getStr(std::string& str, int ioMode = 0) const
		{
			str.clear();
			cybozu::StringOutputStream os(str);
			save(os, ioMode);
		}
		std::string getStr(int ioMode = 0) const
		{
			std::string str;
			getStr(str, ioMode);
			return str;
		}
		void setStr(const std::string& str, int ioMode = 0)
		{
			cybozu::StringInputStream is(str);
			load(is, ioMode);
		}
		friend inline std::ostream& operator<<(std::ostream& os, const Zkp& self)
		{
			self.save(os, fp::detectIoMode(Ec::getIoMode(), os));
			return os;
		}
		friend inline std::istream& operator>>(std::istream& is, Zkp& self)
		{
			self.load(is, fp::detectIoMode(Ec::getIoMode(), is));
			return is;
		}
		// obsolete
		std::string toStr() const { return getStr(); }
		void fromStr(const std::string& str) { setStr(str); }
	};

	class PublicKey {
		size_t bitSize;
		Ec g;
		Ec h;
		bool enableWindowMethod_;
		fp::WindowMethod<Ec> wm_g;
		fp::WindowMethod<Ec> wm_h;
		template<class N>
		void mulDispatch(Ec& z, const Ec& x, const N& n, const fp::WindowMethod<Ec>& pw) const
		{
			if (enableWindowMethod_) {
				pw.mul(z, n);
			} else {
				Ec::mul(z, x, n);
			}
		}
		template<class N>
		void mulG(Ec& z, const N& n) const { mulDispatch(z, g, n, wm_g); }
		template<class N>
		void mulH(Ec& z, const N& n) const { mulDispatch(z, h, n, wm_h); }
	public:
		PublicKey()
			: bitSize(0)
			, enableWindowMethod_(false)
		{
		}
		void enableWindowMethod(size_t winSize = 10)
		{
			wm_g.init(g, bitSize, winSize);
			wm_h.init(h, bitSize, winSize);
			enableWindowMethod_ = true;
		}
		const Ec& getG() const { return g; }
		void init(size_t bitSize, const Ec& g, const Ec& h)
		{
			this->bitSize = bitSize;
			this->g = g;
			this->h = h;
			enableWindowMethod_ = false;
			enableWindowMethod();
		}
		/*
			encode message
			input : m
			output : c = (c1, c2) = (g^u, h^u g^m)
		*/
		void enc(CipherText& c, const Zn& m, fp::RandGen rg = fp::RandGen()) const
		{
			Zn u;
			u.setRand(rg);
			mulG(c.c1, u);
			mulH(c.c2, u);
			Ec t;
			mulG(t, m);
			Ec::add(c.c2, c.c2, t);
		}
		/*
			encode message
			input : m = 0 or 1
			output : c (c1, c2), zkp
		*/
		void encWithZkp(CipherText& c, Zkp& zkp, int m, fp::RandGen rg = fp::RandGen()) const
		{
			if (m != 0 && m != 1) {
				throw cybozu::Exception("elgamal:PublicKey:encWithZkp") << m;
			}
			Zn u;
			u.setRand(rg);
			mulG(c.c1, u);
			mulH(c.c2, u);
			Ec t1, t2;
			Ec R1[2], R2[2];
			zkp.c[1-m].setRand(rg);
			zkp.s[1-m].setRand(rg);
			mulG(t1, zkp.s[1-m]);
			Ec::mul(t2, c.c1, zkp.c[1-m]);
			Ec::sub(R1[1-m], t1, t2);
			mulH(t1, zkp.s[1-m]);
			if (m) {
				Ec::add(c.c2, c.c2, g);
				Ec::mul(t2, c.c2, zkp.c[0]);
			} else {
				Ec::sub(t2, c.c2, g);
				Ec::mul(t2, t2, zkp.c[1]);
			}
			Ec::sub(R2[1-m], t1, t2);
			Zn r;
			r.setRand(rg);
			mulG(R1[m], r);
			mulH(R2[m], r);
			std::ostringstream os;
			os << R1[0] << R2[0] << R1[1] << R2[1] << c.c1 << c.c2 << g << h;
			Zn cc;
			cc.setHashOf(os.str());
			zkp.c[m] = cc - zkp.c[1-m];
			zkp.s[m] = r + zkp.c[m] * u;
		}
		/*
			verify cipher text with ZKP
		*/
		bool verify(const CipherText& c, const Zkp& zkp) const
		{
			Ec R1[2], R2[2];
			Ec t1, t2;
			mulG(t1, zkp.s[0]);
			Ec::mul(t2, c.c1, zkp.c[0]);
			Ec::sub(R1[0], t1, t2);
			mulH(t1, zkp.s[0]);
			Ec::mul(t2, c.c2, zkp.c[0]);
			Ec::sub(R2[0], t1, t2);
			mulG(t1, zkp.s[1]);
			Ec::mul(t2, c.c1, zkp.c[1]);
			Ec::sub(R1[1], t1, t2);
			mulH(t1, zkp.s[1]);
			Ec::sub(t2, c.c2, g);
			Ec::mul(t2, t2, zkp.c[1]);
			Ec::sub(R2[1], t1, t2);
			std::ostringstream os;
			os << R1[0] << R2[0] << R1[1] << R2[1] << c.c1 << c.c2 << g << h;
			Zn cc;
			cc.setHashOf(os.str());
			return cc == zkp.c[0] + zkp.c[1];
		}
		/*
			rerandomize encoded message
			input : c = (c1, c2)
			output : c = (c1 g^v, c2 h^v)
		*/
		void rerandomize(CipherText& c, fp::RandGen rg = fp::RandGen()) const
		{
			Zn v;
			v.setRand(rg);
			Ec t;
			mulG(t, v);
			Ec::add(c.c1, c.c1, t);
			mulH(t, v);
			Ec::add(c.c2, c.c2, t);
		}
		/*
			add encoded message with plain message
			input : c = Enc(m1) = (c1, c2), m2
			ouput : c = Enc(m1 + m2) = (c1, c2 g^m2)
		*/
		template<class N>
		void add(CipherText& c, const N& m) const
		{
			Ec fm;
			mulG(fm, m);
			Ec::add(c.c2, c.c2, fm);
		}
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			std::string s;
			mcl::fp::local::loadWord(s, is);
			bitSize = cybozu::atoi(s);
			g.load(is, ioMode);
			h.load(is, ioMode);
			init(bitSize, g, h);
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			std::string s = cybozu::itoa(bitSize);
			cybozu::write(os, s.c_str(), s.size());
			cybozu::writeChar(os, ' ');

			const char sep = *fp::getIoSeparator(ioMode);
			if (sep) cybozu::writeChar(os, sep);
			g.save(os, ioMode);
			if (sep) cybozu::writeChar(os, sep);
			h.save(os, ioMode);
			if (sep) cybozu::writeChar(os, sep);
		}
		void getStr(std::string& str, int ioMode = 0) const
		{
			str.clear();
			cybozu::StringOutputStream os(str);
			save(os, ioMode);
		}
		std::string getStr(int ioMode = 0) const
		{
			std::string str;
			getStr(str, ioMode);
			return str;
		}
		void setStr(const std::string& str, int ioMode = 0)
		{
			cybozu::StringInputStream is(str);
			load(is, ioMode);
		}
		friend inline std::ostream& operator<<(std::ostream& os, const PublicKey& self)
		{
			self.save(os, fp::detectIoMode(Ec::getIoMode(), os));
			return os;
		}
		friend inline std::istream& operator>>(std::istream& is, PublicKey& self)
		{
			self.load(is, fp::detectIoMode(Ec::getIoMode(), is));
			return is;
		}
		// obsolete
		std::string toStr() const { return getStr(); }
		void fromStr(const std::string& str) { setStr(str); }
	};
	/*
		create table g^i for i in [rangeMin, rangeMax]
	*/
	struct PowerCache {
#if (CYBOZU_CPP_VERSION > CYBOZU_CPP_VERSION_CPP03)
		typedef CYBOZU_NAMESPACE_STD::unordered_map<Ec, int> Cache;
#else
		typedef std::map<Ec, int> Cache;
#endif
		Cache cache;
		void init(const Ec& g, int rangeMin, int rangeMax)
		{
			if (rangeMin > rangeMax) throw cybozu::Exception("mcl:ElgamalT:PowerCache:bad range") << rangeMin << rangeMax;
			Ec x;
			x.clear();
			cache[x] = 0;
			for (int i = 1; i <= rangeMax; i++) {
				Ec::add(x, x, g);
				cache[x] = i;
			}
			Ec nf;
			Ec::neg(nf, g);
			x.clear();
			for (int i = -1; i >= rangeMin; i--) {
				Ec::add(x, x, nf);
				cache[x] = i;
			}
		}
		/*
			return m such that g^m = y
		*/
		int getExponent(const Ec& y, bool *b = 0) const
		{
			typename Cache::const_iterator i = cache.find(y);
			if (i == cache.end()) {
				if (b) {
					*b = false;
					return 0;
				}
				throw cybozu::Exception("Elgamal:PowerCache:getExponent:not found") << y;
			}
			if (b) *b = true;
			return i->second;
		}
		void clear()
		{
			cache.clear();
		}
		bool isEmpty() const
		{
			return cache.empty();
		}
	};
	class PrivateKey {
		PublicKey pub;
		Zn z;
		PowerCache cache;
	public:
		/*
			init
			input : g
			output : (h, z)
			Ec = <g>
			h = g^z
		*/
		void init(const Ec& g, size_t bitSize, fp::RandGen rg = fp::RandGen())
		{
			Ec h;
			z.setRand(rg);
			Ec::mul(h, g, z);
			pub.init(bitSize, g, h);
		}
		const PublicKey& getPublicKey() const { return pub; }
		/*
			decode message by brute-force attack
			input : c = (c1, c2)
			output : m
			M = c2 / c1^z
			find m such that M = g^m and |m| < limit
			@memo 7sec@core i3 for m = 1e6
		*/
		void dec(Zn& m, const CipherText& c, int limit = 100000) const
		{
			const Ec& g = pub.getG();
			Ec c1z;
			Ec::mul(c1z, c.c1, z);
			if (c1z == c.c2) {
				m = 0;
				return;
			}
			Ec t1(c1z);
			Ec t2(c.c2);
			for (int i = 1; i < limit; i++) {
				Ec::add(t1, t1, g);
				if (t1 == c.c2) {
					m = i;
					return;
				}
				Ec::add(t2, t2, g);
				if (t2 == c1z) {
					m = -i;
					return;
				}
			}
			throw cybozu::Exception("elgamal:PrivateKey:dec:overflow");
		}
		/*
			powgm = c2 / c1^z = g^m
		*/
		void getPowerg(Ec& powgm, const CipherText& c) const
		{
			Ec c1z;
			Ec::mul(c1z, c.c1, z);
			Ec::sub(powgm, c.c2, c1z);
		}
		/*
			set range of message to decode quickly
		*/
		void setCache(int rangeMin, int rangeMax)
		{
			cache.init(pub.getG(), rangeMin, rangeMax);
		}
		/*
			clear cache
		*/
		void clearCache()
		{
			cache.clear();
		}
		/*
			decode message by lookup table if !cache.isEmpty()
			                  brute-force attack otherwise
			input : c = (c1, c2)
			        b : set false if not found
			return m
		*/
		int dec(const CipherText& c, bool *b = 0) const
		{
			Ec powgm;
			getPowerg(powgm, c);
			return cache.getExponent(powgm, b);
		}
		/*
			check whether c is encrypted zero message
		*/
		bool isZeroMessage(const CipherText& c) const
		{
			Ec c1z;
			Ec::mul(c1z, c.c1, z);
			return c.c2 == c1z;
		}
		template<class InputStream>
		void load(InputStream& is, int ioMode = IoSerialize)
		{
			pub.load(is, ioMode);
			z.load(is, ioMode);
		}
		template<class OutputStream>
		void save(OutputStream& os, int ioMode = IoSerialize) const
		{
			const char sep = *fp::getIoSeparator(ioMode);
			pub.save(os, ioMode);
			if (sep) cybozu::writeChar(os, sep);
			z.save(os, ioMode);
		}
		void getStr(std::string& str, int ioMode = 0) const
		{
			str.clear();
			cybozu::StringOutputStream os(str);
			save(os, ioMode);
		}
		std::string getStr(int ioMode = 0) const
		{
			std::string str;
			getStr(str, ioMode);
			return str;
		}
		void setStr(const std::string& str, int ioMode = 0)
		{
			cybozu::StringInputStream is(str);
			load(is, ioMode);
		}
		friend inline std::ostream& operator<<(std::ostream& os, const PrivateKey& self)
		{
			self.save(os, fp::detectIoMode(Ec::getIoMode(), os));
			return os;
		}
		friend inline std::istream& operator>>(std::istream& is, PrivateKey& self)
		{
			self.load(is, fp::detectIoMode(Ec::getIoMode(), is));
			return is;
		}
		std::string toStr() const { return getStr(); }
		void fromStr(const std::string& str) { setStr(str); }
	};
};

} // mcl
