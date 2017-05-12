#ifndef CRYPTOPP_TIGER_H
#define CRYPTOPP_TIGER_H

#include "config.h"
#include "iterhash.h"

// Clang 3.3 integrated assembler crash on Linux
//  http://github.com/weidai11/cryptopp/issues/264
#if defined(CRYPTOPP_LLVM_CLANG_VERSION) && (CRYPTOPP_LLVM_CLANG_VERSION < 30400)
# define CRYPTOPP_DISABLE_TIGER_ASM
#endif

NAMESPACE_BEGIN(CryptoPP)

/// <a href="http://www.cryptolounge.org/wiki/Tiger">Tiger</a>
class Tiger : public IteratedHashWithStaticTransform<word64, LittleEndian, 64, 24, Tiger>
{
public:
	static void InitState(HashWordType *state);
	static void Transform(word64 *digest, const word64 *data);
	void TruncatedFinal(byte *hash, size_t size);
	CRYPTOPP_CONSTEXPR static const char *StaticAlgorithmName() {return "Tiger";}

protected:
	static const word64 table[4*256+3];
};

NAMESPACE_END

#endif
