// default.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4127 4189)
#endif

#include "cryptlib.h"
#include "filters.h"
#include "smartptr.h"
#include "default.h"
#include "queue.h"

#include <time.h>
#include <memory>

NAMESPACE_BEGIN(CryptoPP)

static const unsigned int MASH_ITERATIONS = 200;
static const unsigned int SALTLENGTH = 8;
static const unsigned int BLOCKSIZE = DefaultBlockCipher::Encryption::BLOCKSIZE;
static const unsigned int KEYLENGTH = DefaultBlockCipher::Encryption::DEFAULT_KEYLENGTH;

// The purpose of this function Mash() is to take an arbitrary length input
// string and *deterministicly* produce an arbitrary length output string such
// that (1) it looks random, (2) no information about the input is
// deducible from it, and (3) it contains as much entropy as it can hold, or
// the amount of entropy in the input string, whichever is smaller.

static void Mash(const byte *in, size_t inLen, byte *out, size_t outLen, int iterations)
{
	if (BytePrecision(outLen) > 2)
		throw InvalidArgument("Mash: output legnth too large");

	size_t bufSize = RoundUpToMultipleOf(outLen, (size_t)DefaultHashModule::DIGESTSIZE);
	byte b[2];
	SecByteBlock buf(bufSize);
	SecByteBlock outBuf(bufSize);
	DefaultHashModule hash;

	unsigned int i;
	for(i=0; i<outLen; i+=DefaultHashModule::DIGESTSIZE)
	{
		b[0] = (byte) (i >> 8);
		b[1] = (byte) i;
		hash.Update(b, 2);
		hash.Update(in, inLen);
		hash.Final(outBuf+i);
	}

	while (iterations-- > 1)
	{
		memcpy(buf, outBuf, bufSize);
		for (i=0; i<bufSize; i+=DefaultHashModule::DIGESTSIZE)
		{
			b[0] = (byte) (i >> 8);
			b[1] = (byte) i;
			hash.Update(b, 2);
			hash.Update(buf, bufSize);
			hash.Final(outBuf+i);
		}
	}

	memcpy(out, outBuf, outLen);
}

static void GenerateKeyIV(const byte *passphrase, size_t passphraseLength, const byte *salt, size_t saltLength, byte *key, byte *IV)
{
	SecByteBlock temp(passphraseLength+saltLength);
	memcpy(temp, passphrase, passphraseLength);
	memcpy(temp+passphraseLength, salt, saltLength);
	SecByteBlock keyIV(KEYLENGTH+BLOCKSIZE);
	Mash(temp, passphraseLength + saltLength, keyIV, KEYLENGTH+BLOCKSIZE, MASH_ITERATIONS);
	memcpy(key, keyIV, KEYLENGTH);
	memcpy(IV, keyIV+KEYLENGTH, BLOCKSIZE);
}

// ********************************************************

DefaultEncryptor::DefaultEncryptor(const char *passphrase, BufferedTransformation *attachment)
	: ProxyFilter(NULL, 0, 0, attachment), m_passphrase((const byte *)passphrase, strlen(passphrase))
{
}

DefaultEncryptor::DefaultEncryptor(const byte *passphrase, size_t passphraseLength, BufferedTransformation *attachment)
	: ProxyFilter(NULL, 0, 0, attachment), m_passphrase(passphrase, passphraseLength)
{
}


void DefaultEncryptor::FirstPut(const byte *)
{
	// VC60 workaround: __LINE__ expansion bug
	CRYPTOPP_COMPILE_ASSERT_INSTANCE(SALTLENGTH <= DefaultHashModule::DIGESTSIZE, 1);
	CRYPTOPP_COMPILE_ASSERT_INSTANCE(BLOCKSIZE <= DefaultHashModule::DIGESTSIZE, 2);

	SecByteBlock salt(DefaultHashModule::DIGESTSIZE), keyCheck(DefaultHashModule::DIGESTSIZE);
	DefaultHashModule hash;

	// use hash(passphrase | time | clock) as salt
	hash.Update(m_passphrase, m_passphrase.size());
	time_t t=time(0);
	hash.Update((byte *)&t, sizeof(t));
	clock_t c=clock();
	hash.Update((byte *)&c, sizeof(c));
	hash.Final(salt);

	// use hash(passphrase | salt) as key check
	hash.Update(m_passphrase, m_passphrase.size());
	hash.Update(salt, SALTLENGTH);
	hash.Final(keyCheck);

	AttachedTransformation()->Put(salt, SALTLENGTH);

	// mash passphrase and salt together into key and IV
	SecByteBlock key(KEYLENGTH);
	SecByteBlock IV(BLOCKSIZE);
	GenerateKeyIV(m_passphrase, m_passphrase.size(), salt, SALTLENGTH, key, IV);

	m_cipher.SetKeyWithIV(key, key.size(), IV);
	SetFilter(new StreamTransformationFilter(m_cipher));

	m_filter->Put(keyCheck, BLOCKSIZE);
}

void DefaultEncryptor::LastPut(const byte *inString, size_t length)
{
	CRYPTOPP_UNUSED(inString); CRYPTOPP_UNUSED(length);
	m_filter->MessageEnd();
}

// ********************************************************

DefaultDecryptor::DefaultDecryptor(const char *p, BufferedTransformation *attachment, bool throwException)
	: ProxyFilter(NULL, SALTLENGTH+BLOCKSIZE, 0, attachment)
	, m_state(WAITING_FOR_KEYCHECK)
	, m_passphrase((const byte *)p, strlen(p))
	, m_throwException(throwException)
{
}

DefaultDecryptor::DefaultDecryptor(const byte *passphrase, size_t passphraseLength, BufferedTransformation *attachment, bool throwException)
	: ProxyFilter(NULL, SALTLENGTH+BLOCKSIZE, 0, attachment)
	, m_state(WAITING_FOR_KEYCHECK)
	, m_passphrase(passphrase, passphraseLength)
	, m_throwException(throwException)
{
}

void DefaultDecryptor::FirstPut(const byte *inString)
{
	CheckKey(inString, inString+SALTLENGTH);
}

void DefaultDecryptor::LastPut(const byte *inString, size_t length)
{
	CRYPTOPP_UNUSED(inString); CRYPTOPP_UNUSED(length);
	if (m_filter.get() == NULL)
	{
		m_state = KEY_BAD;
		if (m_throwException)
			throw KeyBadErr();
	}
	else
	{
		m_filter->MessageEnd();
		m_state = WAITING_FOR_KEYCHECK;
	}
}

void DefaultDecryptor::CheckKey(const byte *salt, const byte *keyCheck)
{
	SecByteBlock check(STDMAX((unsigned int)2*BLOCKSIZE, (unsigned int)DefaultHashModule::DIGESTSIZE));

	DefaultHashModule hash;
	hash.Update(m_passphrase, m_passphrase.size());
	hash.Update(salt, SALTLENGTH);
	hash.Final(check);

	SecByteBlock key(KEYLENGTH);
	SecByteBlock IV(BLOCKSIZE);
	GenerateKeyIV(m_passphrase, m_passphrase.size(), salt, SALTLENGTH, key, IV);

	m_cipher.SetKeyWithIV(key, key.size(), IV);
	member_ptr<StreamTransformationFilter> decryptor(new StreamTransformationFilter(m_cipher));

	decryptor->Put(keyCheck, BLOCKSIZE);
	decryptor->ForceNextPut();
	decryptor->Get(check+BLOCKSIZE, BLOCKSIZE);

	SetFilter(decryptor.release());

	if (!VerifyBufsEqual(check, check+BLOCKSIZE, BLOCKSIZE))
	{
		m_state = KEY_BAD;
		if (m_throwException)
			throw KeyBadErr();
	}
	else
		m_state = KEY_GOOD;
}

// ********************************************************

static DefaultMAC * NewDefaultEncryptorMAC(const byte *passphrase, size_t passphraseLength)
{
	size_t macKeyLength = DefaultMAC::StaticGetValidKeyLength(16);
	SecByteBlock macKey(macKeyLength);
	// since the MAC is encrypted there is no reason to mash the passphrase for many iterations
	Mash(passphrase, passphraseLength, macKey, macKeyLength, 1);
	return new DefaultMAC(macKey, macKeyLength);
}

DefaultEncryptorWithMAC::DefaultEncryptorWithMAC(const char *passphrase, BufferedTransformation *attachment)
	: ProxyFilter(NULL, 0, 0, attachment)
	, m_mac(NewDefaultEncryptorMAC((const byte *)passphrase, strlen(passphrase)))
{
	SetFilter(new HashFilter(*m_mac, new DefaultEncryptor(passphrase), true));
}

DefaultEncryptorWithMAC::DefaultEncryptorWithMAC(const byte *passphrase, size_t passphraseLength, BufferedTransformation *attachment)
	: ProxyFilter(NULL, 0, 0, attachment)
	, m_mac(NewDefaultEncryptorMAC(passphrase, passphraseLength))
{
	SetFilter(new HashFilter(*m_mac, new DefaultEncryptor(passphrase, passphraseLength), true));
}

void DefaultEncryptorWithMAC::LastPut(const byte *inString, size_t length)
{
	CRYPTOPP_UNUSED(inString); CRYPTOPP_UNUSED(length);
	m_filter->MessageEnd();
}

// ********************************************************

DefaultDecryptorWithMAC::DefaultDecryptorWithMAC(const char *passphrase, BufferedTransformation *attachment, bool throwException)
	: ProxyFilter(NULL, 0, 0, attachment)
	, m_mac(NewDefaultEncryptorMAC((const byte *)passphrase, strlen(passphrase)))
	, m_throwException(throwException)
{
	SetFilter(new DefaultDecryptor(passphrase, m_hashVerifier=new HashVerifier(*m_mac, NULL, HashVerifier::PUT_MESSAGE), throwException));
}

DefaultDecryptorWithMAC::DefaultDecryptorWithMAC(const byte *passphrase, size_t passphraseLength, BufferedTransformation *attachment, bool throwException)
	: ProxyFilter(NULL, 0, 0, attachment)
	, m_mac(NewDefaultEncryptorMAC(passphrase, passphraseLength))
	, m_throwException(throwException)
{
	SetFilter(new DefaultDecryptor(passphrase, passphraseLength, m_hashVerifier=new HashVerifier(*m_mac, NULL, HashVerifier::PUT_MESSAGE), throwException));
}

DefaultDecryptor::State DefaultDecryptorWithMAC::CurrentState() const
{
	return static_cast<const DefaultDecryptor *>(m_filter.get())->CurrentState();
}

bool DefaultDecryptorWithMAC::CheckLastMAC() const
{
	return m_hashVerifier->GetLastResult();
}

void DefaultDecryptorWithMAC::LastPut(const byte *inString, size_t length)
{
	CRYPTOPP_UNUSED(inString); CRYPTOPP_UNUSED(length);
	m_filter->MessageEnd();
	if (m_throwException && !CheckLastMAC())
		throw MACBadErr();
}

NAMESPACE_END

