#ifndef CRYPTOPP_WHIRLPOOL_H
#define CRYPTOPP_WHIRLPOOL_H

#include "config.h"
#include "iterhash.h"

NAMESPACE_BEGIN(CryptoPP)

//! <a href="http://www.cryptolounge.org/wiki/Whirlpool">Whirlpool</a>
class Whirlpool : public IteratedHashWithStaticTransform<word64, BigEndian, 64, 64, Whirlpool>
{
public:
	static void InitState(HashWordType *state);
	static void Transform(word64 *digest, const word64 *data);
	void TruncatedFinal(byte *hash, size_t size);
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "Whirlpool";}
};

NAMESPACE_END

#endif
