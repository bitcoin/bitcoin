// datatest.cpp - written and placed in the public domain by Wei Dai

#define CRYPTOPP_DEFAULT_NO_DLL
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include "cryptlib.h"
#include "factory.h"
#include "integer.h"
#include "filters.h"
#include "hex.h"
#include "randpool.h"
#include "files.h"
#include "trunhash.h"
#include "queue.h"
#include "smartptr.h"
#include "validate.h"
#include "hkdf.h"
#include "stdcpp.h"
#include <iostream>

// Aggressive stack checking with VS2005 SP1 and above.
#if (CRYPTOPP_MSC_VERSION >= 1410)
# pragma strict_gs_check (on)
#endif

#if defined(__COVERITY__)
extern "C" void __coverity_tainted_data_sanitize__(void *);
#endif

USING_NAMESPACE(CryptoPP)
USING_NAMESPACE(std)

typedef std::map<std::string, std::string> TestData;
static bool s_thorough = false;

class TestFailure : public Exception
{
public:
	TestFailure() : Exception(OTHER_ERROR, "Validation test failed") {}
};

static const TestData *s_currentTestData = NULL;

static void OutputTestData(const TestData &v)
{
	for (TestData::const_iterator i = v.begin(); i != v.end(); ++i)
	{
		cerr << i->first << ": " << i->second << endl;
	}
}

static void SignalTestFailure()
{
	OutputTestData(*s_currentTestData);
	throw TestFailure();
}

static void SignalUnknownAlgorithmError(const std::string& algType)
{
	OutputTestData(*s_currentTestData);
	throw Exception(Exception::OTHER_ERROR, "Unknown algorithm " + algType + " during validation test");
}

static void SignalTestError()
{
	OutputTestData(*s_currentTestData);
	throw Exception(Exception::OTHER_ERROR, "Unexpected error during validation test");
}

bool DataExists(const TestData &data, const char *name)
{
	TestData::const_iterator i = data.find(name);
	return (i != data.end());
}

const std::string & GetRequiredDatum(const TestData &data, const char *name)
{
	TestData::const_iterator i = data.find(name);
	if (i == data.end())
		SignalTestError();
	return i->second;
}

void RandomizedTransfer(BufferedTransformation &source, BufferedTransformation &target, bool finish, const std::string &channel=DEFAULT_CHANNEL)
{
	while (source.MaxRetrievable() > (finish ? 0 : 4096))
	{
		byte buf[4096+64];
		size_t start = GlobalRNG().GenerateWord32(0, 63);
		size_t len = GlobalRNG().GenerateWord32(1, UnsignedMin(4096U, 3*source.MaxRetrievable()/2));
		len = source.Get(buf+start, len);
		target.ChannelPut(channel, buf+start, len);
	}
}

void PutDecodedDatumInto(const TestData &data, const char *name, BufferedTransformation &target)
{
	std::string s1 = GetRequiredDatum(data, name), s2;
	ByteQueue q;

	while (!s1.empty())
	{
		while (s1[0] == ' ')
		{
			s1 = s1.substr(1);
			if (s1.empty())
				goto end;	// avoid invalid read if s1 is empty
		}

		int repeat = 1;
		if (s1[0] == 'r')
		{
			repeat = atoi(s1.c_str()+1);
			s1 = s1.substr(s1.find(' ')+1);
		}

		s2 = ""; // MSVC 6 doesn't have clear();

		if (s1[0] == '\"')
		{
			s2 = s1.substr(1, s1.find('\"', 1)-1);
			s1 = s1.substr(s2.length() + 2);
		}
		else if (s1.substr(0, 2) == "0x")
		{
			StringSource(s1.substr(2, s1.find(' ')), true, new HexDecoder(new StringSink(s2)));
			s1 = s1.substr(STDMIN(s1.find(' '), s1.length()));
		}
		else
		{
			StringSource(s1.substr(0, s1.find(' ')), true, new HexDecoder(new StringSink(s2)));
			s1 = s1.substr(STDMIN(s1.find(' '), s1.length()));
		}

		while (repeat--)
		{
			q.Put((const byte *)s2.data(), s2.size());
			RandomizedTransfer(q, target, false);
		}
	}

end:
	RandomizedTransfer(q, target, true);
}

std::string GetDecodedDatum(const TestData &data, const char *name)
{
	std::string s;
	PutDecodedDatumInto(data, name, StringSink(s).Ref());
	return s;
}

std::string GetOptionalDecodedDatum(const TestData &data, const char *name)
{
	std::string s;
	if (DataExists(data, name))
		PutDecodedDatumInto(data, name, StringSink(s).Ref());
	return s;
}

class TestDataNameValuePairs : public NameValuePairs
{
public:
	TestDataNameValuePairs(const TestData &data) : m_data(data) {}

	virtual bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
	{
		TestData::const_iterator i = m_data.find(name);
		if (i == m_data.end())
		{
			if (std::string(name) == Name::DigestSize() && valueType == typeid(int))
			{
				i = m_data.find("MAC");
				if (i == m_data.end())
					i = m_data.find("Digest");
				if (i == m_data.end())
					return false;

				m_temp.resize(0);
				PutDecodedDatumInto(m_data, i->first.c_str(), StringSink(m_temp).Ref());
				*reinterpret_cast<int *>(pValue) = (int)m_temp.size();
				return true;
			}
			else
				return false;
		}

		const std::string &value = i->second;

		if (valueType == typeid(int))
			*reinterpret_cast<int *>(pValue) = atoi(value.c_str());
		else if (valueType == typeid(Integer))
			*reinterpret_cast<Integer *>(pValue) = Integer((std::string(value) + "h").c_str());
		else if (valueType == typeid(ConstByteArrayParameter))
		{
			m_temp.resize(0);
			PutDecodedDatumInto(m_data, name, StringSink(m_temp).Ref());
			reinterpret_cast<ConstByteArrayParameter *>(pValue)->Assign((const byte *)m_temp.data(), m_temp.size(), false);
		}
		else
			throw ValueTypeMismatch(name, typeid(std::string), valueType);

		return true;
	}

private:
	const TestData &m_data;
	mutable std::string m_temp;
};

void TestKeyPairValidAndConsistent(CryptoMaterial &pub, const CryptoMaterial &priv)
{
	// "!!" converts between bool <-> integral.
	if (!pub.Validate(GlobalRNG(), 2U+!!s_thorough))
		SignalTestFailure();
	if (!priv.Validate(GlobalRNG(), 2U+!!s_thorough))
		SignalTestFailure();

	ByteQueue bq1, bq2;
	pub.Save(bq1);
	pub.AssignFrom(priv);
	pub.Save(bq2);
	if (bq1 != bq2)
		SignalTestFailure();
}

void TestSignatureScheme(TestData &v)
{
	std::string name = GetRequiredDatum(v, "Name");
	std::string test = GetRequiredDatum(v, "Test");

	member_ptr<PK_Signer> signer(ObjectFactoryRegistry<PK_Signer>::Registry().CreateObject(name.c_str()));
	member_ptr<PK_Verifier> verifier(ObjectFactoryRegistry<PK_Verifier>::Registry().CreateObject(name.c_str()));

	TestDataNameValuePairs pairs(v);

	if (test == "GenerateKey")
	{
		signer->AccessPrivateKey().GenerateRandom(GlobalRNG(), pairs);
		verifier->AccessPublicKey().AssignFrom(signer->AccessPrivateKey());
	}
	else
	{
		std::string keyFormat = GetRequiredDatum(v, "KeyFormat");

		if (keyFormat == "DER")
			verifier->AccessMaterial().Load(StringStore(GetDecodedDatum(v, "PublicKey")).Ref());
		else if (keyFormat == "Component")
			verifier->AccessMaterial().AssignFrom(pairs);

		if (test == "Verify" || test == "NotVerify")
		{
			VerifierFilter verifierFilter(*verifier, NULL, VerifierFilter::SIGNATURE_AT_BEGIN);
			PutDecodedDatumInto(v, "Signature", verifierFilter);
			PutDecodedDatumInto(v, "Message", verifierFilter);
			verifierFilter.MessageEnd();
			if (verifierFilter.GetLastResult() == (test == "NotVerify"))
				SignalTestFailure();
			return;
		}
		else if (test == "PublicKeyValid")
		{
			if (!verifier->GetMaterial().Validate(GlobalRNG(), 3))
				SignalTestFailure();
			return;
		}

		if (keyFormat == "DER")
			signer->AccessMaterial().Load(StringStore(GetDecodedDatum(v, "PrivateKey")).Ref());
		else if (keyFormat == "Component")
			signer->AccessMaterial().AssignFrom(pairs);
	}

	if (test == "GenerateKey" || test == "KeyPairValidAndConsistent")
	{
		TestKeyPairValidAndConsistent(verifier->AccessMaterial(), signer->GetMaterial());
		VerifierFilter verifierFilter(*verifier, NULL, VerifierFilter::THROW_EXCEPTION);
		verifierFilter.Put((const byte *)"abc", 3);
		StringSource ss("abc", true, new SignerFilter(GlobalRNG(), *signer, new Redirector(verifierFilter)));
	}
	else if (test == "Sign")
	{
		SignerFilter f(GlobalRNG(), *signer, new HexEncoder(new FileSink(cout)));
		StringSource ss(GetDecodedDatum(v, "Message"), true, new Redirector(f));
		SignalTestFailure();
	}
	else if (test == "DeterministicSign")
	{
		// This test is specialized for RFC 6979. The RFC is a drop-in replacement
		// for DSA and ECDSA, and access to the seed or secret is not needed. If
		// additional determinsitic signatures are added, then the test harness will
		// likely need to be extended.
		string signature;
		SignerFilter f(GlobalRNG(), *signer, new HexEncoder(new StringSink(signature)));
		StringSource ss(GetDecodedDatum(v, "Message"), true, new Redirector(f));
		if (GetDecodedDatum(v, "Signature") != signature)
			SignalTestFailure();
		return;
	}
	else if (test == "RandomSign")
	{
		SignalTestError();
		CRYPTOPP_ASSERT(false);	// TODO: implement
	}
	else
	{
		SignalTestError();
		CRYPTOPP_ASSERT(false);
	}
}

void TestAsymmetricCipher(TestData &v)
{
	std::string name = GetRequiredDatum(v, "Name");
	std::string test = GetRequiredDatum(v, "Test");

	member_ptr<PK_Encryptor> encryptor(ObjectFactoryRegistry<PK_Encryptor>::Registry().CreateObject(name.c_str()));
	member_ptr<PK_Decryptor> decryptor(ObjectFactoryRegistry<PK_Decryptor>::Registry().CreateObject(name.c_str()));

	std::string keyFormat = GetRequiredDatum(v, "KeyFormat");

	if (keyFormat == "DER")
	{
		decryptor->AccessMaterial().Load(StringStore(GetDecodedDatum(v, "PrivateKey")).Ref());
		encryptor->AccessMaterial().Load(StringStore(GetDecodedDatum(v, "PublicKey")).Ref());
	}
	else if (keyFormat == "Component")
	{
		TestDataNameValuePairs pairs(v);
		decryptor->AccessMaterial().AssignFrom(pairs);
		encryptor->AccessMaterial().AssignFrom(pairs);
	}

	if (test == "DecryptMatch")
	{
		std::string decrypted, expected = GetDecodedDatum(v, "Plaintext");
		StringSource ss(GetDecodedDatum(v, "Ciphertext"), true, new PK_DecryptorFilter(GlobalRNG(), *decryptor, new StringSink(decrypted)));
		if (decrypted != expected)
			SignalTestFailure();
	}
	else if (test == "KeyPairValidAndConsistent")
	{
		TestKeyPairValidAndConsistent(encryptor->AccessMaterial(), decryptor->GetMaterial());
	}
	else
	{
		SignalTestError();
		CRYPTOPP_ASSERT(false);
	}
}

void TestSymmetricCipher(TestData &v, const NameValuePairs &overrideParameters)
{
	std::string name = GetRequiredDatum(v, "Name");
	std::string test = GetRequiredDatum(v, "Test");

	std::string key = GetDecodedDatum(v, "Key");
	std::string plaintext = GetDecodedDatum(v, "Plaintext");

	TestDataNameValuePairs testDataPairs(v);
	CombinedNameValuePairs pairs(overrideParameters, testDataPairs);

	if (test == "Encrypt" || test == "EncryptXorDigest" || test == "Resync" || test == "EncryptionMCT" || test == "DecryptionMCT")
	{
		static member_ptr<SymmetricCipher> encryptor, decryptor;
		static std::string lastName;

		if (name != lastName)
		{
			encryptor.reset(ObjectFactoryRegistry<SymmetricCipher, ENCRYPTION>::Registry().CreateObject(name.c_str()));
			decryptor.reset(ObjectFactoryRegistry<SymmetricCipher, DECRYPTION>::Registry().CreateObject(name.c_str()));
			lastName = name;
		}

		ConstByteArrayParameter iv;
		if (pairs.GetValue(Name::IV(), iv) && iv.size() != encryptor->IVSize())
			SignalTestFailure();

		if (test == "Resync")
		{
			encryptor->Resynchronize(iv.begin(), (int)iv.size());
			decryptor->Resynchronize(iv.begin(), (int)iv.size());
		}
		else
		{
			encryptor->SetKey((const byte *)key.data(), key.size(), pairs);
			decryptor->SetKey((const byte *)key.data(), key.size(), pairs);
		}

		int seek = pairs.GetIntValueWithDefault("Seek", 0);
		if (seek)
		{
			encryptor->Seek(seek);
			decryptor->Seek(seek);
		}

		std::string encrypted, xorDigest, ciphertext, ciphertextXorDigest;
		if (test == "EncryptionMCT" || test == "DecryptionMCT")
		{
			SymmetricCipher *cipher = encryptor.get();
			SecByteBlock buf((byte *)plaintext.data(), plaintext.size()), keybuf((byte *)key.data(), key.size());

			if (test == "DecryptionMCT")
			{
				cipher = decryptor.get();
				ciphertext = GetDecodedDatum(v, "Ciphertext");
				buf.Assign((byte *)ciphertext.data(), ciphertext.size());
			}

			for (int i=0; i<400; i++)
			{
				encrypted.reserve(10000 * plaintext.size());
				for (int j=0; j<10000; j++)
				{
					cipher->ProcessString(buf.begin(), buf.size());
					encrypted.append((char *)buf.begin(), buf.size());
				}

				encrypted.erase(0, encrypted.size() - keybuf.size());
				xorbuf(keybuf.begin(), (const byte *)encrypted.data(), keybuf.size());
				cipher->SetKey(keybuf, keybuf.size());
			}
			encrypted.assign((char *)buf.begin(), buf.size());
			ciphertext = GetDecodedDatum(v, test == "EncryptionMCT" ? "Ciphertext" : "Plaintext");
			if (encrypted != ciphertext)
			{
				std::cout << "incorrectly encrypted: ";
				StringSource xx(encrypted, false, new HexEncoder(new FileSink(std::cout)));
				xx.Pump(256); xx.Flush(false);
				std::cout << "\n";
				SignalTestFailure();
			}
			return;
		}

		StreamTransformationFilter encFilter(*encryptor, new StringSink(encrypted), StreamTransformationFilter::NO_PADDING);
		RandomizedTransfer(StringStore(plaintext).Ref(), encFilter, true);
		encFilter.MessageEnd();
		/*{
			std::string z;
			encryptor->Seek(seek);
			StringSource ss(plaintext, false, new StreamTransformationFilter(*encryptor, new StringSink(z), StreamTransformationFilter::NO_PADDING));
			while (ss.Pump(64)) {}
			ss.PumpAll();
			for (int i=0; i<z.length(); i++)
				CRYPTOPP_ASSERT(encrypted[i] == z[i]);
		}*/
		if (test != "EncryptXorDigest")
			ciphertext = GetDecodedDatum(v, "Ciphertext");
		else
		{
			ciphertextXorDigest = GetDecodedDatum(v, "CiphertextXorDigest");
			xorDigest.append(encrypted, 0, 64);
			for (size_t i=64; i<encrypted.size(); i++)
				xorDigest[i%64] ^= encrypted[i];
		}
		if (test != "EncryptXorDigest" ? encrypted != ciphertext : xorDigest != ciphertextXorDigest)
		{
			std::cout << "incorrectly encrypted: ";
			StringSource xx(encrypted, false, new HexEncoder(new FileSink(std::cout)));
			xx.Pump(2048); xx.Flush(false);
			std::cout << "\n";
			SignalTestFailure();
		}
		std::string decrypted;
		StreamTransformationFilter decFilter(*decryptor, new StringSink(decrypted), StreamTransformationFilter::NO_PADDING);
		RandomizedTransfer(StringStore(encrypted).Ref(), decFilter, true);
		decFilter.MessageEnd();
		if (decrypted != plaintext)
		{
			std::cout << "incorrectly decrypted: ";
			StringSource xx(decrypted, false, new HexEncoder(new FileSink(std::cout)));
			xx.Pump(256); xx.Flush(false);
			std::cout << "\n";
			SignalTestFailure();
		}
	}
	else
	{
		std::cout << "unexpected test name\n";
		SignalTestError();
	}
}

void TestAuthenticatedSymmetricCipher(TestData &v, const NameValuePairs &overrideParameters)
{
	std::string type = GetRequiredDatum(v, "AlgorithmType");
	std::string name = GetRequiredDatum(v, "Name");
	std::string test = GetRequiredDatum(v, "Test");
	std::string key = GetDecodedDatum(v, "Key");

	std::string plaintext = GetOptionalDecodedDatum(v, "Plaintext");
	std::string ciphertext = GetOptionalDecodedDatum(v, "Ciphertext");
	std::string header = GetOptionalDecodedDatum(v, "Header");
	std::string footer = GetOptionalDecodedDatum(v, "Footer");
	std::string mac = GetOptionalDecodedDatum(v, "MAC");

	TestDataNameValuePairs testDataPairs(v);
	CombinedNameValuePairs pairs(overrideParameters, testDataPairs);

	if (test == "Encrypt" || test == "EncryptXorDigest" || test == "NotVerify")
	{
		member_ptr<AuthenticatedSymmetricCipher> asc1, asc2;
		asc1.reset(ObjectFactoryRegistry<AuthenticatedSymmetricCipher, ENCRYPTION>::Registry().CreateObject(name.c_str()));
		asc2.reset(ObjectFactoryRegistry<AuthenticatedSymmetricCipher, DECRYPTION>::Registry().CreateObject(name.c_str()));
		asc1->SetKey((const byte *)key.data(), key.size(), pairs);
		asc2->SetKey((const byte *)key.data(), key.size(), pairs);

		std::string encrypted, decrypted;
		AuthenticatedEncryptionFilter ef(*asc1, new StringSink(encrypted));
		bool macAtBegin = !mac.empty() && !GlobalRNG().GenerateBit();	// test both ways randomly
		AuthenticatedDecryptionFilter df(*asc2, new StringSink(decrypted), macAtBegin ? AuthenticatedDecryptionFilter::MAC_AT_BEGIN : 0);

		if (asc1->NeedsPrespecifiedDataLengths())
		{
			asc1->SpecifyDataLengths(header.size(), plaintext.size(), footer.size());
			asc2->SpecifyDataLengths(header.size(), plaintext.size(), footer.size());
		}

		StringStore sh(header), sp(plaintext), sc(ciphertext), sf(footer), sm(mac);

		if (macAtBegin)
			RandomizedTransfer(sm, df, true);
		sh.CopyTo(df, LWORD_MAX, AAD_CHANNEL);
		RandomizedTransfer(sc, df, true);
		sf.CopyTo(df, LWORD_MAX, AAD_CHANNEL);
		if (!macAtBegin)
			RandomizedTransfer(sm, df, true);
		df.MessageEnd();

		RandomizedTransfer(sh, ef, true, AAD_CHANNEL);
		RandomizedTransfer(sp, ef, true);
		RandomizedTransfer(sf, ef, true, AAD_CHANNEL);
		ef.MessageEnd();

		if (test == "Encrypt" && encrypted != ciphertext+mac)
		{
			std::cout << "incorrectly encrypted: ";
			StringSource xx(encrypted, false, new HexEncoder(new FileSink(std::cout)));
			xx.Pump(2048); xx.Flush(false);
			std::cout << "\n";
			SignalTestFailure();
		}
		if (test == "Encrypt" && decrypted != plaintext)
		{
			std::cout << "incorrectly decrypted: ";
			StringSource xx(decrypted, false, new HexEncoder(new FileSink(std::cout)));
			xx.Pump(256); xx.Flush(false);
			std::cout << "\n";
			SignalTestFailure();
		}

		if (ciphertext.size()+mac.size()-plaintext.size() != asc1->DigestSize())
		{
			std::cout << "bad MAC size\n";
			SignalTestFailure();
		}
		if (df.GetLastResult() != (test == "Encrypt"))
		{
			std::cout << "MAC incorrectly verified\n";
			SignalTestFailure();
		}
	}
	else
	{
		std::cout << "unexpected test name\n";
		SignalTestError();
	}
}

void TestDigestOrMAC(TestData &v, bool testDigest)
{
	std::string name = GetRequiredDatum(v, "Name");
	std::string test = GetRequiredDatum(v, "Test");
	const char *digestName = testDigest ? "Digest" : "MAC";

	member_ptr<MessageAuthenticationCode> mac;
	member_ptr<HashTransformation> hash;
	HashTransformation *pHash = NULL;

	TestDataNameValuePairs pairs(v);

	if (testDigest)
	{
		hash.reset(ObjectFactoryRegistry<HashTransformation>::Registry().CreateObject(name.c_str()));
		pHash = hash.get();
	}
	else
	{
		mac.reset(ObjectFactoryRegistry<MessageAuthenticationCode>::Registry().CreateObject(name.c_str()));
		pHash = mac.get();
		std::string key = GetDecodedDatum(v, "Key");
		mac->SetKey((const byte *)key.c_str(), key.size(), pairs);
	}

	if (test == "Verify" || test == "VerifyTruncated" || test == "NotVerify")
	{
		int digestSize = -1;
		if (test == "VerifyTruncated")
			digestSize = pairs.GetIntValueWithDefault(Name::DigestSize(), digestSize);
		HashVerificationFilter verifierFilter(*pHash, NULL, HashVerificationFilter::HASH_AT_BEGIN, digestSize);
		PutDecodedDatumInto(v, digestName, verifierFilter);
		PutDecodedDatumInto(v, "Message", verifierFilter);
		verifierFilter.MessageEnd();
		if (verifierFilter.GetLastResult() == (test == "NotVerify"))
			SignalTestFailure();
	}
	else
	{
		SignalTestError();
		CRYPTOPP_ASSERT(false);
	}
}

void TestKeyDerivationFunction(TestData &v)
{
	std::string name = GetRequiredDatum(v, "Name");
	std::string test = GetRequiredDatum(v, "Test");

	if(test == "Skip") return;
	CRYPTOPP_ASSERT(test == "Verify");

	std::string key = GetDecodedDatum(v, "Key");
	std::string salt = GetDecodedDatum(v, "Salt");
	std::string info = GetDecodedDatum(v, "Info");
	std::string derived = GetDecodedDatum(v, "DerivedKey");
	std::string t = GetDecodedDatum(v, "DerivedKeyLength");

	TestDataNameValuePairs pairs(v);
	unsigned int length = pairs.GetIntValueWithDefault(Name::DerivedKeyLength(), (int)derived.size());

	member_ptr<KeyDerivationFunction> kdf;
	kdf.reset(ObjectFactoryRegistry<KeyDerivationFunction>::Registry().CreateObject(name.c_str()));

	std::string calc; calc.resize(length);
	unsigned int ret = kdf->DeriveKey(reinterpret_cast<byte*>(&calc[0]), calc.size(),
		reinterpret_cast<const byte*>(key.data()), key.size(),
		reinterpret_cast<const byte*>(salt.data()), salt.size(),
		reinterpret_cast<const byte*>(info.data()), info.size());

	if(calc != derived || ret != length)
		SignalTestFailure();
}

bool GetField(std::istream &is, std::string &name, std::string &value)
{
	name.resize(0);		// GCC workaround: 2.95.3 doesn't have clear()
	is >> name;

	if (name.empty())
		return false;

	if (name[name.size()-1] != ':')
	{
		char c;
		is >> skipws >> c;
		if (c != ':')
			SignalTestError();
	}
	else
		name.erase(name.size()-1);

	while (is.peek() == ' ')
		is.ignore(1);

	// VC60 workaround: getline bug
	char buffer[128];
	value.resize(0);	// GCC workaround: 2.95.3 doesn't have clear()
	bool continueLine, space = false;

	do
	{
		do
		{
			is.get(buffer, sizeof(buffer));
			value += buffer;
			if (buffer[0] == ' ')
				space = true;
		}
		while (buffer[0] != 0);
		is.clear();
		is.ignore();

		if (!value.empty() && value[value.size()-1] == '\r')
			value.resize(value.size()-1);

		if (!value.empty() && value[value.size()-1] == '\\')
		{
			value.resize(value.size()-1);
			continueLine = true;
		}
		else
			continueLine = false;

		std::string::size_type i = value.find('#');
		if (i != std::string::npos)
			value.erase(i);
	}
	while (continueLine);

	// Strip intermediate spaces for some values.
	if (space && (name == "Modulus" || name == "SubgroupOrder" || name == "SubgroupGenerator" ||
		name == "PublicElement" || name == "PrivateExponent" || name == "Signature"))
	{
		string temp;
		temp.reserve(value.size());

		std::string::const_iterator it;
		for(it = value.begin(); it != value.end(); it++)
		{
			if(*it != ' ')
				temp.push_back(*it);
		}

		std::swap(temp, value);
	}

	return true;
}

void OutputPair(const NameValuePairs &v, const char *name)
{
	Integer x;
	bool b = v.GetValue(name, x);
	CRYPTOPP_UNUSED(b); CRYPTOPP_ASSERT(b);
	cout << name << ": \\\n    ";
	x.Encode(HexEncoder(new FileSink(cout), false, 64, "\\\n    ").Ref(), x.MinEncodedSize());
	cout << endl;
}

void OutputNameValuePairs(const NameValuePairs &v)
{
	std::string names = v.GetValueNames();
	string::size_type i = 0;
	while (i < names.size())
	{
		string::size_type j = names.find_first_of (';', i);

		if (j == string::npos)
			return;
		else
		{
			std::string name = names.substr(i, j-i);
			if (name.find(':') == string::npos)
				OutputPair(v, name.c_str());
		}

		i = j + 1;
	}
}

void TestDataFile(std::string filename, const NameValuePairs &overrideParameters, unsigned int &totalTests, unsigned int &failedTests)
{
	static const std::string dataDirectory(CRYPTOPP_DATA_DIR);
	if (!dataDirectory.empty())
	{
		if(dataDirectory != filename.substr(0, dataDirectory.length()))
			filename.insert(0, dataDirectory);
	}

	std::ifstream file(filename.c_str());
	if (!file.good())
		throw Exception(Exception::OTHER_ERROR, "Can not open file " + filename + " for reading");
	TestData v;
	s_currentTestData = &v;
	std::string name, value, lastAlgName;

	while (file)
	{
		while (file.peek() == '#')
			file.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

		if (file.peek() == '\n' || file.peek() == '\r')
			v.clear();

		if (!GetField(file, name, value))
			break;
		v[name] = value;

		if (name == "Test" && (s_thorough || v["SlowTest"] != "1"))
		{
			bool failed = true;
			std::string algType = GetRequiredDatum(v, "AlgorithmType");

			if (lastAlgName != GetRequiredDatum(v, "Name"))
			{
				lastAlgName = GetRequiredDatum(v, "Name");
				cout << "\nTesting " << algType.c_str() << " algorithm " << lastAlgName.c_str() << ".\n";
			}

			try
			{
				if (algType == "Signature")
					TestSignatureScheme(v);
				else if (algType == "SymmetricCipher")
					TestSymmetricCipher(v, overrideParameters);
				else if (algType == "AuthenticatedSymmetricCipher")
					TestAuthenticatedSymmetricCipher(v, overrideParameters);
				else if (algType == "AsymmetricCipher")
					TestAsymmetricCipher(v);
				else if (algType == "MessageDigest")
					TestDigestOrMAC(v, true);
				else if (algType == "MAC")
					TestDigestOrMAC(v, false);
				else if (algType == "KDF")
					TestKeyDerivationFunction(v);
				else if (algType == "FileList")
					TestDataFile(GetRequiredDatum(v, "Test"), g_nullNameValuePairs, totalTests, failedTests);
				else
					SignalUnknownAlgorithmError(algType);
				failed = false;
			}
			catch (const TestFailure &)
			{
				cout << "\nTest failed.\n";
			}
			catch (const CryptoPP::Exception &e)
			{
				cout << "\nCryptoPP::Exception caught: " << e.what() << endl;
			}
			catch (const std::exception &e)
			{
				cout << "\nstd::exception caught: " << e.what() << endl;
			}

			if (failed)
			{
				cout << "Skipping to next test.\n";
				failedTests++;
			}
			else
				cout << "." << flush;

			totalTests++;
		}
	}
}

bool RunTestDataFile(const char *filename, const NameValuePairs &overrideParameters, bool thorough)
{
	s_thorough = thorough;
	unsigned int totalTests = 0, failedTests = 0;
	TestDataFile((filename ? filename : ""), overrideParameters, totalTests, failedTests);
	cout << dec << "\nTests complete. Total tests = " << totalTests << ". Failed tests = " << failedTests << "." << endl;
	if (failedTests != 0)
		cout << "SOME TESTS FAILED!\n";
	return failedTests == 0;
}

