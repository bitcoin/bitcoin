// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcointalkcoin Core developers
// Copyright (c) 2014-2015 The Groestlcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINTALKCOIN_GROESTLHASH_H
#define BITCOINTALKCOIN_GROESTLHASH_H

#include <amount.h>
#include <uint256.h>
#include <serialize.h>

class ConstBuf {
public:
	const unsigned char *P;
	size_t Size;

	template <typename T>
	ConstBuf(const T pb, const T pe) {
		if (pb == pe) {
			P = 0;
			Size = 0;
		} else {
			P = (unsigned char*)(&pb[0]);
			Size = (pe-pb) * sizeof(pb[0]);
		}
	}

	ConstBuf(const std::vector<unsigned char>& vch) {
		if (vch.empty()) {
			P = 0;
			Size = 0;
		} else {
			P = &vch[0];
			Size = vch.size();
		}
	}
};


uint256 HashGroestl(const ConstBuf& cbuf);

uint256 HashFromTx(const ConstBuf& cbuf);
uint256 HashForSignature(const ConstBuf& cbuf);
inline uint256 HashPow(const ConstBuf& cbuf) { return HashGroestl(cbuf); }
inline uint256 HashMessage(const ConstBuf& cbuf) { return HashGroestl(cbuf); }
inline uint256 HashForAddress(const ConstBuf& cbuf) { return HashGroestl(cbuf); }


class GroestlHasher {
private:
	void *ctx;
public:
	void Finalize(unsigned char hash[32]);
	GroestlHasher& Write(const unsigned char *data, size_t len);
	GroestlHasher();
	GroestlHasher(GroestlHasher&& x);
	~GroestlHasher();
	GroestlHasher& operator=(GroestlHasher&& x);
};

class CGroestlHashWriter
{
private:
	GroestlHasher ctx;

	const int nType;
	const int nVersion;
public:

	CGroestlHashWriter(int nTypeIn, int nVersionIn) : nType(nTypeIn), nVersion(nVersionIn) {}

	int GetType() const { return nType; }
	int GetVersion() const { return nVersion; }

	void write(const char *pch, size_t size) {
		ctx.Write((const unsigned char*)pch, size);
	}

	// invalidates the object
	uint256 GetHash() {
		uint256 result;
		ctx.Finalize((unsigned char*)&result);
		return result;
	}

	template<typename T>
	CGroestlHashWriter& operator<<(const T& obj) {
		// Serialize to this stream
		::Serialize(*this, obj);
		return (*this);
	}
};
#endif
