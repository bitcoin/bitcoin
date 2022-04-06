#pragma once
/**
	@file
	@brief wrap openssl
	@author MITSUNARI Shigeo(@herumi)
*/

#include <cybozu/exception.hpp>
#ifdef __APPLE__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#if 0 //#ifdef __APPLE__
	#define COMMON_DIGEST_FOR_OPENSSL
	#include <CommonCrypto/CommonDigest.h>
	#include <CommonCrypto/CommonHMAC.h>
	#define SHA1 CC_SHA1
	#define SHA224 CC_SHA224
	#define SHA256 CC_SHA256
	#define SHA384 CC_SHA384
	#define SHA512 CC_SHA512
#else
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#endif
#ifdef _MSC_VER
	#include <cybozu/link_libeay32.hpp>
#endif

namespace cybozu {

namespace crypto {

class Hash {
public:
	enum Name {
		N_SHA1,
		N_SHA224,
		N_SHA256,
		N_SHA384,
		N_SHA512
	};
private:
	Name name_;
	size_t hashSize_;
	union {
		SHA_CTX sha1;
		SHA256_CTX sha256;
		SHA512_CTX sha512;
	} ctx_;
public:
	static inline size_t getSize(Name name)
	{
		switch (name) {
		case N_SHA1:   return SHA_DIGEST_LENGTH;
		case N_SHA224: return SHA224_DIGEST_LENGTH;
		case N_SHA256: return SHA256_DIGEST_LENGTH;
		case N_SHA384: return SHA384_DIGEST_LENGTH;
		case N_SHA512: return SHA512_DIGEST_LENGTH;
		default:
			throw cybozu::Exception("crypto:Hash:getSize") << name;
		}
	}
	static inline const char *getName(Name name)
	{
		switch (name) {
		case N_SHA1:   return "sha1";
		case N_SHA224: return "sha224";
		case N_SHA256: return "sha256";
		case N_SHA384: return "sha384";
		case N_SHA512: return "sha512";
		default:
			throw cybozu::Exception("crypto:Hash:getName") << name;
		}
	}
	static inline Name getName(const std::string& nameStr)
	{
		static const struct {
			const char *nameStr;
			Name name;
		} tbl[] = {
			{ "sha1", N_SHA1 },
			{ "sha224", N_SHA224 },
			{ "sha256", N_SHA256 },
			{ "sha384", N_SHA384 },
			{ "sha512", N_SHA512 },
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			if (nameStr == tbl[i].nameStr) return tbl[i].name;
		}
		throw cybozu::Exception("crypto:Hash:getName") << nameStr;
	}
	explicit Hash(Name name = N_SHA1)
		: name_(name)
		, hashSize_(getSize(name))
	{
		reset();
	}
	void update(const void *buf, size_t bufSize)
	{
		switch (name_) {
		case N_SHA1:   SHA1_Update(&ctx_.sha1, buf, bufSize);     break;
		case N_SHA224: SHA224_Update(&ctx_.sha256, buf, bufSize); break;
		case N_SHA256: SHA256_Update(&ctx_.sha256, buf, bufSize); break;
		case N_SHA384: SHA384_Update(&ctx_.sha512, buf, bufSize); break;
		case N_SHA512: SHA512_Update(&ctx_.sha512, buf, bufSize); break;
		}
	}
	void update(const std::string& buf)
	{
		update(buf.c_str(), buf.size());
	}
	void reset()
	{
		switch (name_) {
		case N_SHA1:   SHA1_Init(&ctx_.sha1);     break;
		case N_SHA224: SHA224_Init(&ctx_.sha256); break;
		case N_SHA256: SHA256_Init(&ctx_.sha256); break;
		case N_SHA384: SHA384_Init(&ctx_.sha512); break;
		case N_SHA512: SHA512_Init(&ctx_.sha512); break;
		default:
			throw cybozu::Exception("crypto:Hash:rset") << name_;
		}
	}
	/*
		md must have hashSize byte
		@note clear inner buffer after calling digest
	*/
	void digest(void *out, const void *buf, size_t bufSize)
	{
		update(buf, bufSize);
		unsigned char *md = reinterpret_cast<unsigned char*>(out);
		switch (name_) {
		case N_SHA1:   SHA1_Final(md, &ctx_.sha1);     break;
		case N_SHA224: SHA224_Final(md, &ctx_.sha256); break;
		case N_SHA256: SHA256_Final(md, &ctx_.sha256); break;
		case N_SHA384: SHA384_Final(md, &ctx_.sha512); break;
		case N_SHA512: SHA512_Final(md, &ctx_.sha512); break;
		default:
			throw cybozu::Exception("crypto:Hash:digest") << name_;
		}
		reset();
	}
	std::string digest(const void *buf, size_t bufSize)
	{
		std::string ret;
		ret.resize(hashSize_);
		digest(&ret[0], buf, bufSize);
		return ret;
	}
	std::string digest(const std::string& buf = "")
	{
		return digest(buf.c_str(), buf.size());
	}
	/*
		out must have necessary size
		@note return written size
	*/
	static inline size_t digest(void *out, Name name, const void *buf, size_t bufSize)
	{
		unsigned char *md = (unsigned char*)out;
		const unsigned char *src = cybozu::cast<const unsigned char *>(buf);
		switch (name) {
		case N_SHA1:   SHA1(src, bufSize, md);   return 160 / 8;
		case N_SHA224: SHA224(src, bufSize, md); return 224 / 8;
		case N_SHA256: SHA256(src, bufSize, md); return 256 / 8;
		case N_SHA384: SHA384(src, bufSize, md); return 384 / 8;
		case N_SHA512: SHA512(src, bufSize, md); return 512 / 8;
		default:
			return 0;
		}
	}
	static inline std::string digest(Name name, const void *buf, size_t bufSize)
	{
		char md[128];
		size_t size = digest(md, name, buf, bufSize);
		if (size == 0) throw cybozu::Exception("crypt:Hash:digest") << name;
		return std::string(md, size);
	}
	static inline std::string digest(Name name, const std::string& buf)
	{
		return digest(name, buf.c_str(), buf.size());
	}
};

class Hmac {
	const EVP_MD *evp_;
public:
	explicit Hmac(Hash::Name name = Hash::N_SHA1)
	{
		switch (name) {
		case Hash::N_SHA1: evp_ = EVP_sha1(); break;
		case Hash::N_SHA224: evp_ = EVP_sha224(); break;
		case Hash::N_SHA256: evp_ = EVP_sha256(); break;
		case Hash::N_SHA384: evp_ = EVP_sha384(); break;
		case Hash::N_SHA512: evp_ = EVP_sha512(); break;
		default:
			throw cybozu::Exception("crypto:Hmac:") << name;
		}
	}
	std::string eval(const std::string& key, const std::string& data)
	{
		std::string out(EVP_MD_size(evp_) + 1, 0);
		unsigned int outLen = 0;
		if (HMAC(evp_, key.c_str(), static_cast<int>(key.size()),
			cybozu::cast<const uint8_t *>(data.c_str()), data.size(), cybozu::cast<uint8_t *>(&out[0]), &outLen)) {
			out.resize(outLen);
			return out;
		}
		throw cybozu::Exception("crypto::Hamc::eval");
	}
};

class Cipher {
	const EVP_CIPHER *cipher_;
	EVP_CIPHER_CTX *ctx_;
public:
	enum Name {
		N_AES128_CBC,
		N_AES192_CBC,
		N_AES256_CBC,
		N_AES128_ECB, // be carefull to use
		N_AES192_ECB, // be carefull to use
		N_AES256_ECB, // be carefull to use
	};
	static inline size_t getSize(Name name)
	{
		switch (name) {
		case N_AES128_CBC: return 128;
		case N_AES192_CBC: return 192;
		case N_AES256_CBC: return 256;
		case N_AES128_ECB: return 128;
		case N_AES192_ECB: return 192;
		case N_AES256_ECB: return 256;
		default:
			throw cybozu::Exception("crypto:Cipher:getSize") << name;
		}
	}
	enum Mode {
		Decoding,
		Encoding
	};
	explicit Cipher(Name name = N_AES128_CBC)
		: cipher_(0)
		, ctx_(0)
	{
		ctx_ = EVP_CIPHER_CTX_new();
		if (ctx_ == 0) throw cybozu::Exception("crypto:Cipher:EVP_CIPHER_CTX_new");
		switch (name) {
		case N_AES128_CBC: cipher_ = EVP_aes_128_cbc(); break;
		case N_AES192_CBC: cipher_ = EVP_aes_192_cbc(); break;
		case N_AES256_CBC: cipher_ = EVP_aes_256_cbc(); break;
		case N_AES128_ECB: cipher_ = EVP_aes_128_ecb(); break;
		case N_AES192_ECB: cipher_ = EVP_aes_192_ecb(); break;
		case N_AES256_ECB: cipher_ = EVP_aes_256_ecb(); break;
		default:
			throw cybozu::Exception("crypto:Cipher:Cipher:name") << (int)name;
		}
	}
	~Cipher()
	{
		if (ctx_) EVP_CIPHER_CTX_free(ctx_);
	}
	/*
		@note don't use padding = true
	*/
	void setup(Mode mode, const std::string& key, const std::string& iv, bool padding = false)
	{
		const int keyLen = static_cast<int>(key.size());
		const int expectedKeyLen = EVP_CIPHER_key_length(cipher_);
		if (keyLen != expectedKeyLen) {
			throw cybozu::Exception("crypto:Cipher:setup:keyLen") << keyLen << expectedKeyLen;
		}

		int ret = EVP_CipherInit_ex(ctx_, cipher_, NULL, cybozu::cast<const uint8_t*>(key.c_str()), cybozu::cast<const uint8_t*>(iv.c_str()), mode == Encoding ? 1 : 0);
		if (ret != 1) {
			throw cybozu::Exception("crypto:Cipher:setup:EVP_CipherInit_ex") << ret;
		}
		ret = EVP_CIPHER_CTX_set_padding(ctx_, padding ? 1 : 0);
		if (ret != 1) {
			throw cybozu::Exception("crypto:Cipher:setup:EVP_CIPHER_CTX_set_padding") << ret;
		}
/*
		const int ivLen = static_cast<int>(iv.size());
		const int expectedIvLen = EVP_CIPHER_CTX_iv_length(&ctx_);
		if (ivLen != expectedIvLen) {
			throw cybozu::Exception("crypto:Cipher:setup:ivLen") << ivLen << expectedIvLen;
		}
*/
	}
	/*
		the size of outBuf must be larger than inBufSize + blockSize
		@retval positive or 0 : writeSize(+blockSize)
		@retval -1 : error
	*/
	int update(char *outBuf, const char *inBuf, int inBufSize)
	{
		int outLen = 0;
		int ret = EVP_CipherUpdate(ctx_, cybozu::cast<uint8_t*>(outBuf), &outLen, cybozu::cast<const uint8_t*>(inBuf), inBufSize);
		if (ret != 1) return -1;
		return outLen;
	}
	/*
		return -1 if padding
		@note don't use
	*/
	int finalize(char *outBuf)
	{
		int outLen = 0;
		int ret = EVP_CipherFinal_ex(ctx_, cybozu::cast<uint8_t*>(outBuf), &outLen);
		if (ret != 1) return -1;
		return outLen;
	}
};

} }	// cybozu::crypto

#ifdef __APPLE__
	#pragma GCC diagnostic pop
#endif
