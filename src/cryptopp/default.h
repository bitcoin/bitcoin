// default.h - written and placed in the public domain by Wei Dai

//! \file default.h
//! \brief Classes for DefaultEncryptor, DefaultDecryptor, DefaultEncryptorWithMAC and DefaultDecryptorWithMAC

#ifndef CRYPTOPP_DEFAULT_H
#define CRYPTOPP_DEFAULT_H

#include "sha.h"
#include "hmac.h"
#include "des.h"
#include "modes.h"
#include "filters.h"
#include "smartptr.h"

NAMESPACE_BEGIN(CryptoPP)

//! \brief Default block cipher for DefaultEncryptor, DefaultDecryptor, DefaultEncryptorWithMAC and DefaultDecryptorWithMAC
typedef DES_EDE2 DefaultBlockCipher;
//! \brief Default hash for use with DefaultEncryptorWithMAC and DefaultDecryptorWithMAC
typedef SHA DefaultHashModule;
//! \brief Default HMAC for use withDefaultEncryptorWithMAC and DefaultDecryptorWithMAC
typedef HMAC<DefaultHashModule> DefaultMAC;

//! \class DefaultEncryptor
//! \brief Password-Based Encryptor using TripleDES
//! \details The class uses 2-key TripleDES (DES_EDE2) for encryption, which only
//!   provides about 80-bits of security.
class DefaultEncryptor : public ProxyFilter
{
public:
	//! \brief Construct a DefaultEncryptor
	//! \param passphrase a C-String password
	//! \param attachment a BufferedTransformation to attach to this object
	DefaultEncryptor(const char *passphrase, BufferedTransformation *attachment = NULL);

	//! \brief Construct a DefaultEncryptor
	//! \param passphrase a byte string password
	//! \param passphraseLength the length of the byte string password
	//! \param attachment a BufferedTransformation to attach to this object
	DefaultEncryptor(const byte *passphrase, size_t passphraseLength, BufferedTransformation *attachment = NULL);

protected:
	void FirstPut(const byte *);
	void LastPut(const byte *inString, size_t length);

private:
	SecByteBlock m_passphrase;
	CBC_Mode<DefaultBlockCipher>::Encryption m_cipher;

} CRYPTOPP_DEPRECATED ("DefaultEncryptor will be changing in the near future because the algorithms are no longer secure");

//! \class DefaultDecryptor
//! \brief Password-Based Decryptor using TripleDES
//! \details The class uses 2-key TripleDES (DES_EDE2) for encryption, which only
//!   provides about 80-bits of security.
class DefaultDecryptor : public ProxyFilter
{
public:
	//! \brief Constructs a DefaultDecryptor
	//! \param passphrase a C-String password
	//! \param attachment a BufferedTransformation to attach to this object
	//! \param throwException a flag specifiying whether an Exception should be thrown on error
	DefaultDecryptor(const char *passphrase, BufferedTransformation *attachment = NULL, bool throwException=true);

	//! \brief Constructs a DefaultDecryptor
	//! \param passphrase a byte string password
	//! \param passphraseLength the length of the byte string password
	//! \param attachment a BufferedTransformation to attach to this object
	//! \param throwException a flag specifiying whether an Exception should be thrown on error
	DefaultDecryptor(const byte *passphrase, size_t passphraseLength, BufferedTransformation *attachment = NULL, bool throwException=true);

	class Err : public Exception
	{
	public:
		Err(const std::string &s)
			: Exception(DATA_INTEGRITY_CHECK_FAILED, s) {}
	};
	class KeyBadErr : public Err {public: KeyBadErr() : Err("DefaultDecryptor: cannot decrypt message with this passphrase") {}};

	enum State {WAITING_FOR_KEYCHECK, KEY_GOOD, KEY_BAD};
	State CurrentState() const {return m_state;}

protected:
	void FirstPut(const byte *inString);
	void LastPut(const byte *inString, size_t length);

	State m_state;

private:
	void CheckKey(const byte *salt, const byte *keyCheck);

	SecByteBlock m_passphrase;
	CBC_Mode<DefaultBlockCipher>::Decryption m_cipher;
	member_ptr<FilterWithBufferedInput> m_decryptor;
	bool m_throwException;

} CRYPTOPP_DEPRECATED ("DefaultDecryptor will be changing in the near future because the algorithms are no longer secure");

//! \class DefaultEncryptorWithMAC
//! \brief Password-Based encryptor using TripleDES and HMAC/SHA-1
//! \details DefaultEncryptorWithMAC uses a non-standard mashup function called Mash() to derive key
//!   bits from the password. The class also uses 2-key TripleDES (DES_EDE2) for encryption, which only
//!   provides about 80-bits of security.
//! \details The purpose of the function Mash() is to take an arbitrary length input string and
//!   *deterministically* produce an arbitrary length output string such that (1) it looks random,
//!   (2) no information about the input is deducible from it, and (3) it contains as much entropy
//!   as it can hold, or the amount of entropy in the input string, whichever is smaller.
class DefaultEncryptorWithMAC : public ProxyFilter
{
public:
	//! \brief Constructs a DefaultEncryptorWithMAC
	//! \param passphrase a C-String password
	//! \param attachment a BufferedTransformation to attach to this object
	DefaultEncryptorWithMAC(const char *passphrase, BufferedTransformation *attachment = NULL);

	//! \brief Constructs a DefaultEncryptorWithMAC
	//! \param passphrase a byte string password
	//! \param passphraseLength the length of the byte string password
	//! \param attachment a BufferedTransformation to attach to this object
	DefaultEncryptorWithMAC(const byte *passphrase, size_t passphraseLength, BufferedTransformation *attachment = NULL);

protected:
	void FirstPut(const byte *inString) {CRYPTOPP_UNUSED(inString);}
	void LastPut(const byte *inString, size_t length);

private:
	member_ptr<DefaultMAC> m_mac;

} CRYPTOPP_DEPRECATED ("DefaultEncryptorWithMAC will be changing in the near future because the algorithms are no longer secure");

//! \class DefaultDecryptorWithMAC
//! \brief Password-Based decryptor using TripleDES and HMAC/SHA-1
//! \details DefaultDecryptorWithMAC uses a non-standard mashup function called Mash() to derive key
//!   bits from the password. The class also uses 2-key TripleDES (DES_EDE2) for encryption, which only
//!   provides about 80-bits of security.
//! \details The purpose of the function Mash() is to take an arbitrary length input string and
//!   *deterministically* produce an arbitrary length output string such that (1) it looks random,
//!   (2) no information about the input is deducible from it, and (3) it contains as much entropy
//!   as it can hold, or the amount of entropy in the input string, whichever is smaller.
class DefaultDecryptorWithMAC : public ProxyFilter
{
public:
	//! \class MACBadErr
	//! \brief Excpetion thrown when an incorrect MAC is encountered
	class MACBadErr : public DefaultDecryptor::Err {public: MACBadErr() : DefaultDecryptor::Err("DefaultDecryptorWithMAC: MAC check failed") {}};

	//! \brief Constructs a DefaultDecryptor
	//! \param passphrase a C-String password
	//! \param attachment a BufferedTransformation to attach to this object
	//! \param throwException a flag specifiying whether an Exception should be thrown on error
	DefaultDecryptorWithMAC(const char *passphrase, BufferedTransformation *attachment = NULL, bool throwException=true);

	//! \brief Constructs a DefaultDecryptor
	//! \param passphrase a byte string password
	//! \param passphraseLength the length of the byte string password
	//! \param attachment a BufferedTransformation to attach to this object
	//! \param throwException a flag specifiying whether an Exception should be thrown on error
	DefaultDecryptorWithMAC(const byte *passphrase, size_t passphraseLength, BufferedTransformation *attachment = NULL, bool throwException=true);

	DefaultDecryptor::State CurrentState() const;
	bool CheckLastMAC() const;

protected:
	void FirstPut(const byte *inString) {CRYPTOPP_UNUSED(inString);}
	void LastPut(const byte *inString, size_t length);

private:
	member_ptr<DefaultMAC> m_mac;
	HashVerifier *m_hashVerifier;
	bool m_throwException;

} CRYPTOPP_DEPRECATED ("DefaultDecryptorWithMAC will be changing in the near future because the algorithms are no longer secure");

NAMESPACE_END

#endif
