// regtest.cpp - written and placed in the public domain by Wei Dai

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include "cryptlib.h"
#include "factory.h"
#include "modes.h"
#include "dh.h"
#include "esign.h"
#include "md2.h"
#include "rw.h"
#include "md5.h"
#include "rsa.h"
#include "ripemd.h"
#include "dsa.h"
#include "seal.h"
#include "whrlpool.h"
#include "ttmac.h"
#include "camellia.h"
#include "shacal2.h"
#include "tea.h"
#include "panama.h"
#include "pssr.h"
#include "aes.h"
#include "salsa.h"
#include "chacha.h"
#include "vmac.h"
#include "tiger.h"
#include "md5.h"
#include "sosemanuk.h"
#include "arc4.h"
#include "ccm.h"
#include "gcm.h"
#include "eax.h"
#include "twofish.h"
#include "serpent.h"
#include "cast.h"
#include "rc6.h"
#include "mars.h"
#include "des.h"
#include "idea.h"
#include "rc5.h"
#include "tea.h"
#include "skipjack.h"
#include "cmac.h"
#include "dmac.h"
#include "blowfish.h"
#include "seed.h"
#include "wake.h"
#include "seal.h"
#include "crc.h"
#include "adler32.h"
#include "keccak.h"
#include "sha3.h"
#include "blake2.h"
#include "hkdf.h"

// Aggressive stack checking with VS2005 SP1 and above.
#if (CRYPTOPP_MSC_VERSION >= 1410)
# pragma strict_gs_check (on)
#endif

#if CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

USING_NAMESPACE(CryptoPP)

void RegisterFactories()
{
	static bool s_registered = false;
	if (s_registered)
		return;

	RegisterDefaultFactoryFor<SimpleKeyAgreementDomain, DH>();
	RegisterDefaultFactoryFor<HashTransformation, CRC32>();
	RegisterDefaultFactoryFor<HashTransformation, CRC32C>();
	RegisterDefaultFactoryFor<HashTransformation, Adler32>();
	RegisterDefaultFactoryFor<HashTransformation, Weak::MD5>();
	RegisterDefaultFactoryFor<HashTransformation, SHA1>();
	RegisterDefaultFactoryFor<HashTransformation, SHA224>();
	RegisterDefaultFactoryFor<HashTransformation, SHA256>();
	RegisterDefaultFactoryFor<HashTransformation, SHA384>();
	RegisterDefaultFactoryFor<HashTransformation, SHA512>();
	RegisterDefaultFactoryFor<HashTransformation, Whirlpool>();
	RegisterDefaultFactoryFor<HashTransformation, Tiger>();
	RegisterDefaultFactoryFor<HashTransformation, RIPEMD160>();
	RegisterDefaultFactoryFor<HashTransformation, RIPEMD320>();
	RegisterDefaultFactoryFor<HashTransformation, RIPEMD128>();
	RegisterDefaultFactoryFor<HashTransformation, RIPEMD256>();
	RegisterDefaultFactoryFor<HashTransformation, Weak::PanamaHash<LittleEndian> >();
	RegisterDefaultFactoryFor<HashTransformation, Weak::PanamaHash<BigEndian> >();
	RegisterDefaultFactoryFor<HashTransformation, Keccak_224>();
	RegisterDefaultFactoryFor<HashTransformation, Keccak_256>();
	RegisterDefaultFactoryFor<HashTransformation, Keccak_384>();
	RegisterDefaultFactoryFor<HashTransformation, Keccak_512>();
	RegisterDefaultFactoryFor<HashTransformation, SHA3_224>();
	RegisterDefaultFactoryFor<HashTransformation, SHA3_256>();
	RegisterDefaultFactoryFor<HashTransformation, SHA3_384>();
	RegisterDefaultFactoryFor<HashTransformation, SHA3_512>();
	RegisterDefaultFactoryFor<HashTransformation, BLAKE2s>();
	RegisterDefaultFactoryFor<HashTransformation, BLAKE2b>();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, HMAC<Weak::MD5> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, HMAC<SHA1> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, HMAC<RIPEMD160> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, HMAC<SHA224> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, HMAC<SHA256> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, HMAC<SHA384> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, HMAC<SHA512> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, TTMAC>();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, VMAC<AES> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, VMAC<AES, 64> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, Weak::PanamaMAC<LittleEndian> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, Weak::PanamaMAC<BigEndian> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, CMAC<AES> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, DMAC<AES> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, CMAC<DES_EDE3> >();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, BLAKE2s>();
	RegisterDefaultFactoryFor<MessageAuthenticationCode, BLAKE2b>();
	RegisterAsymmetricCipherDefaultFactories<RSAES<OAEP<SHA1> > >("RSA/OAEP-MGF1(SHA-1)");
	RegisterAsymmetricCipherDefaultFactories<DLIES<> >("DLIES(NoCofactorMultiplication, KDF2(SHA-1), XOR, HMAC(SHA-1), DHAES)");
	RegisterSignatureSchemeDefaultFactories<DSA>();
	RegisterSignatureSchemeDefaultFactories<DSA2<SHA224> >();
	RegisterSignatureSchemeDefaultFactories<DSA2<SHA256> >();
	RegisterSignatureSchemeDefaultFactories<DSA2<SHA384> >();
	RegisterSignatureSchemeDefaultFactories<DSA2<SHA512> >();
	RegisterSignatureSchemeDefaultFactories<NR<SHA1> >("NR(1363)/EMSA1(SHA-1)");
	RegisterSignatureSchemeDefaultFactories<GDSA<SHA1> >("DSA-1363/EMSA1(SHA-1)");
	RegisterSignatureSchemeDefaultFactories<RSASS<PKCS1v15, Weak::MD2> >("RSA/PKCS1-1.5(MD2)");
	RegisterSignatureSchemeDefaultFactories<RSASS<PKCS1v15, SHA1> >("RSA/PKCS1-1.5(SHA-1)");
	RegisterSignatureSchemeDefaultFactories<ESIGN<SHA1> >("ESIGN/EMSA5-MGF1(SHA-1)");
	RegisterSignatureSchemeDefaultFactories<RWSS<P1363_EMSA2, SHA1> >("RW/EMSA2(SHA-1)");
	RegisterSignatureSchemeDefaultFactories<RSASS<PSS, SHA1> >("RSA/PSS-MGF1(SHA-1)");
	RegisterSymmetricCipherDefaultFactories<SEAL<> >();
	RegisterSymmetricCipherDefaultFactories<ECB_Mode<SHACAL2> >();
	RegisterSymmetricCipherDefaultFactories<ECB_Mode<Camellia> >();
	RegisterSymmetricCipherDefaultFactories<ECB_Mode<TEA> >();
	RegisterSymmetricCipherDefaultFactories<ECB_Mode<XTEA> >();
	RegisterSymmetricCipherDefaultFactories<PanamaCipher<LittleEndian> >();
	RegisterSymmetricCipherDefaultFactories<PanamaCipher<BigEndian> >();
	RegisterSymmetricCipherDefaultFactories<ECB_Mode<AES> >();
	RegisterSymmetricCipherDefaultFactories<CBC_Mode<AES> >();
	RegisterSymmetricCipherDefaultFactories<CFB_Mode<AES> >();
	RegisterSymmetricCipherDefaultFactories<OFB_Mode<AES> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<AES> >();
	RegisterSymmetricCipherDefaultFactories<Salsa20>();
	RegisterSymmetricCipherDefaultFactories<XSalsa20>();
	RegisterSymmetricCipherDefaultFactories<ChaCha8>();
	RegisterSymmetricCipherDefaultFactories<ChaCha12>();
	RegisterSymmetricCipherDefaultFactories<ChaCha20>();
	RegisterSymmetricCipherDefaultFactories<Sosemanuk>();
	RegisterSymmetricCipherDefaultFactories<Weak::MARC4>();
	RegisterSymmetricCipherDefaultFactories<WAKE_OFB<LittleEndian> >();
	RegisterSymmetricCipherDefaultFactories<WAKE_OFB<BigEndian> >();
	RegisterSymmetricCipherDefaultFactories<SEAL<LittleEndian> >();
	RegisterAuthenticatedSymmetricCipherDefaultFactories<CCM<AES> >();
	RegisterAuthenticatedSymmetricCipherDefaultFactories<GCM<AES> >();
	RegisterAuthenticatedSymmetricCipherDefaultFactories<EAX<AES> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<Camellia> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<Twofish> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<Serpent> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<CAST256> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<RC6> >();
	RegisterSymmetricCipherDefaultFactories<ECB_Mode<MARS> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<MARS> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<SHACAL2> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<DES> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<DES_XEX3> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<DES_EDE3> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<IDEA> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<RC5> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<TEA> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<XTEA> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<CAST128> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<SKIPJACK> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<Blowfish> >();
	RegisterSymmetricCipherDefaultFactories<ECB_Mode<SEED> >();
	RegisterSymmetricCipherDefaultFactories<CTR_Mode<SEED> >();
	RegisterDefaultFactoryFor<KeyDerivationFunction, HKDF<SHA1> >();
	RegisterDefaultFactoryFor<KeyDerivationFunction, HKDF<SHA256> >();
	RegisterDefaultFactoryFor<KeyDerivationFunction, HKDF<SHA512> >();
	RegisterDefaultFactoryFor<KeyDerivationFunction, HKDF<Whirlpool> >();

	s_registered = true;
}
