// fipsalgt.cpp - written and placed in the public domain by Wei Dai

// This file implements the various algorithm tests needed to pass FIPS 140 validation.
// They're preserved here (commented out) in case Crypto++ needs to be revalidated.

#if 0
#ifndef CRYPTOPP_IMPORTS
#define CRYPTOPP_DEFAULT_NO_DLL
#endif

#include "dll.h"
#include "cryptlib.h"
#include "smartptr.h"
#include "filters.h"
#include "oids.h"

USING_NAMESPACE(CryptoPP)
USING_NAMESPACE(std)

class LineBreakParser : public AutoSignaling<Bufferless<Filter> >
{
public:
	LineBreakParser(BufferedTransformation *attachment=NULL, byte lineEnd='\n')
		: m_lineEnd(lineEnd) {Detach(attachment);}

	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking)
	{
		if (!blocking)
			throw BlockingInputOnly("LineBreakParser");

		unsigned int i, last = 0;
		for (i=0; i<length; i++)
		{
			if (begin[i] == m_lineEnd)
			{
				AttachedTransformation()->Put2(begin+last, i-last, GetAutoSignalPropagation(), blocking);
				last = i+1;
			}
		}
		if (last != i)
			AttachedTransformation()->Put2(begin+last, i-last, 0, blocking);

		if (messageEnd && GetAutoSignalPropagation())
		{
			AttachedTransformation()->MessageEnd(GetAutoSignalPropagation()-1, blocking);
			AttachedTransformation()->MessageSeriesEnd(GetAutoSignalPropagation()-1, blocking);
		}

		return 0;
	}

private:
	byte m_lineEnd;
};

class TestDataParser : public Unflushable<FilterWithInputQueue>
{
public:
	enum DataType {OTHER, COUNT, KEY_T, IV, INPUT, OUTPUT};

	TestDataParser(std::string algorithm, std::string test, std::string mode, unsigned int feedbackSize, bool encrypt, BufferedTransformation *attachment)
		: m_algorithm(algorithm), m_test(test), m_mode(mode), m_feedbackSize(feedbackSize)
		, m_firstLine(true), m_blankLineTransition(0)
	{
		Detach(attachment);

		m_typeToName[COUNT] = "COUNT";

		m_nameToType["COUNT"] = COUNT;
		m_nameToType["KEY"] = KEY_T;
		m_nameToType["KEYs"] = KEY_T;
		m_nameToType["key"] = KEY_T;
		m_nameToType["Key"] = KEY_T;
		m_nameToType["IV"] = IV;
		m_nameToType["IV1"] = IV;
		m_nameToType["CV"] = IV;
		m_nameToType["CV1"] = IV;
		m_nameToType["IB"] = IV;
		m_nameToType["TEXT"] = INPUT;
		m_nameToType["RESULT"] = OUTPUT;
		m_nameToType["Msg"] = INPUT;
		m_nameToType["Seed"] = INPUT;
		m_nameToType["V"] = INPUT;
		m_nameToType["DT"] = IV;
		SetEncrypt(encrypt);

		if (m_algorithm == "DSA" || m_algorithm == "ECDSA")
		{
			if (m_test == "PKV")
				m_trigger = "Qy";
			else if (m_test == "KeyPair")
				m_trigger = "N";
			else if (m_test == "SigGen")
				m_trigger = "Msg";
			else if (m_test == "SigVer")
				m_trigger = "S";
			else if (m_test == "PQGGen")
				m_trigger = "N";
			else if (m_test == "PQGVer")
				m_trigger = "H";
		}
		else if (m_algorithm == "HMAC")
			m_trigger = "Msg";
		else if (m_algorithm == "SHA")
			m_trigger = (m_test == "MONTE") ? "Seed" : "Msg";
		else if (m_algorithm == "RNG")
			m_trigger = "V";
		else if (m_algorithm == "RSA")
			m_trigger = (m_test == "Ver") ? "S" : "Msg";
	}

	void SetEncrypt(bool encrypt)
	{
		m_encrypt = encrypt;
		if (encrypt)
		{
			m_nameToType["PLAINTEXT"] = INPUT;
			m_nameToType["CIPHERTEXT"] = OUTPUT;
			m_nameToType["PT"] = INPUT;
			m_nameToType["CT"] = OUTPUT;
		}
		else
		{
			m_nameToType["PLAINTEXT"] = OUTPUT;
			m_nameToType["CIPHERTEXT"] = INPUT;
			m_nameToType["PT"] = OUTPUT;
			m_nameToType["CT"] = INPUT;
		}

		if (m_algorithm == "AES" || m_algorithm == "TDES")
		{
			if (encrypt)
			{
				m_trigger = "PLAINTEXT";
				m_typeToName[OUTPUT] = "CIPHERTEXT";
			}
			else
			{
				m_trigger = "CIPHERTEXT";
				m_typeToName[OUTPUT] = "PLAINTEXT";
			}
			m_count = 0;
		}
	}

protected:
	void OutputData(std::string &output, const std::string &key, const std::string &data)
	{
		output += key;
		output += "= ";
		output += data;
		output += "\n";
	}

	void OutputData(std::string &output, const std::string &key, int data)
	{
		OutputData(output, key, IntToString(data));
	}

	void OutputData(std::string &output, const std::string &key, const SecByteBlock &data)
	{
		output += key;
		output += "= ";
		HexEncoder(new StringSink(output), false).Put(data, data.size());
		output += "\n";
	}

	void OutputData(std::string &output, const std::string &key, const Integer &data, int size=-1)
	{
		SecByteBlock s(size < 0 ? data.MinEncodedSize() : size);
		data.Encode(s, s.size());
		OutputData(output, key, s);
	}

	void OutputData(std::string &output, const std::string &key, const PolynomialMod2 &data, int size=-1)
	{
		SecByteBlock s(size < 0 ? data.MinEncodedSize() : size);
		data.Encode(s, s.size());
		OutputData(output, key, s);
	}

	void OutputData(std::string &output, DataType t, const std::string &data)
	{
		if (m_algorithm == "SKIPJACK")
		{
			if (m_test == "KAT")
			{
				if (t == OUTPUT)
					output = m_line + data + "\n";
			}
			else
			{
				if (t != COUNT)
				{
					output += m_typeToName[t];
					output += "=";
				}
				output += data;
				output += t == OUTPUT ? "\n" : "  ";
			}
		}
		else if (m_algorithm == "TDES" && t == KEY_T && m_typeToName[KEY_T].empty())
		{
			output += "KEY1 = ";
			output += data.substr(0, 16);
			output += "\nKEY2 = ";
			output += data.size() > 16 ? data.substr(16, 16) : data.substr(0, 16);
			output += "\nKEY3 = ";
			output += data.size() > 32 ? data.substr(32, 16) : data.substr(0, 16);
			output += "\n";
		}
		else
		{
			output += m_typeToName[t];
			output += " = ";
			output += data;
			output += "\n";
		}
	}

	void OutputData(std::string &output, DataType t, int i)
	{
		OutputData(output, t, IntToString(i));
	}

	void OutputData(std::string &output, DataType t, const SecByteBlock &data)
	{
		std::string hexData;
		StringSource(data.begin(), data.size(), true, new HexEncoder(new StringSink(hexData), false));
		OutputData(output, t, hexData);
	}

	void OutputGivenData(std::string &output, DataType t, bool optional = false)
	{
		if (m_data.find(m_typeToName[t]) == m_data.end())
		{
			if (optional)
				return;
			throw Exception(Exception::OTHER_ERROR, "TestDataParser: key not found: " + m_typeToName[t]);
		}

		OutputData(output, t, m_data[m_typeToName[t]]);
	}

	template <class T>
		BlockCipher * NewBT(T *)
	{
		if (!m_encrypt && (m_mode == "ECB" || m_mode == "CBC"))
			return new typename T::Decryption;
		else
			return new typename T::Encryption;
	}

	template <class T>
		SymmetricCipher * NewMode(T *, BlockCipher &bt, const byte *iv)
	{
		if (!m_encrypt)
			return new typename T::Decryption(bt, iv, m_feedbackSize/8);
		else
			return new typename T::Encryption(bt, iv, m_feedbackSize/8);
	}

	static inline void Xor(SecByteBlock &z, const SecByteBlock &x, const SecByteBlock &y)
	{
		CRYPTOPP_ASSERT(x.size() == y.size());
		z.resize(x.size());
		xorbuf(z, x, y, x.size());
	}

	SecByteBlock UpdateKey(SecByteBlock key, const SecByteBlock *text)
	{
		unsigned int innerCount = (m_algorithm == "AES") ? 1000 : 10000;
		int keySize = key.size(), blockSize = text[0].size();
		SecByteBlock x(keySize);
		for (int k=0; k<keySize;)
		{
			int pos = innerCount * blockSize - keySize + k;
			memcpy(x + k, text[pos / blockSize] + pos % blockSize, blockSize - pos % blockSize);
			k += blockSize - pos % blockSize;
		}

		if (m_algorithm == "TDES" || m_algorithm == "DES")
		{
			for (int i=0; i<keySize; i+=8)
			{
				xorbuf(key+i, x+keySize-8-i, 8);
				DES::CorrectKeyParityBits(key+i);
			}
		}
		else
			xorbuf(key, x, keySize);

		return key;
	}

	static inline void AssignLeftMostBits(SecByteBlock &z, const SecByteBlock &x, unsigned int K)
	{
		z.Assign(x, K/8);
	}

	template <class EC>
	void EC_KeyPair(string &output, int n, const OID &oid)
	{
		DL_GroupParameters_EC<EC> params(oid);
		for (int i=0; i<n; i++)
		{
			DL_PrivateKey_EC<EC> priv;
			DL_PublicKey_EC<EC> pub;
			priv.Initialize(m_rng, params);
			priv.MakePublicKey(pub);

			OutputData(output, "d ", priv.GetPrivateExponent());
			OutputData(output, "Qx ", pub.GetPublicElement().x, params.GetCurve().GetField().MaxElementByteLength());
			OutputData(output, "Qy ", pub.GetPublicElement().y, params.GetCurve().GetField().MaxElementByteLength());
		}
	}

	template <class EC>
	void EC_SigGen(string &output, const OID &oid)
	{
		DL_GroupParameters_EC<EC> params(oid);
		typename ECDSA<EC, SHA1>::PrivateKey priv;
		typename ECDSA<EC, SHA1>::PublicKey pub;
		priv.Initialize(m_rng, params);
		priv.MakePublicKey(pub);

		typename ECDSA<EC, SHA1>::Signer signer(priv);
		SecByteBlock sig(signer.SignatureLength());
		StringSource(m_data["Msg"], true, new HexDecoder(new SignerFilter(m_rng, signer, new ArraySink(sig, sig.size()))));
		SecByteBlock R(sig, sig.size()/2), S(sig+sig.size()/2, sig.size()/2);

		OutputData(output, "Qx ", pub.GetPublicElement().x, params.GetCurve().GetField().MaxElementByteLength());
		OutputData(output, "Qy ", pub.GetPublicElement().y, params.GetCurve().GetField().MaxElementByteLength());
		OutputData(output, "R ", R);
		OutputData(output, "S ", S);
	}

	template <class EC>
	void EC_SigVer(string &output, const OID &oid)
	{
		SecByteBlock x(DecodeHex(m_data["Qx"]));
		SecByteBlock y(DecodeHex(m_data["Qy"]));
		Integer r((m_data["R"]+"h").c_str());
		Integer s((m_data["S"]+"h").c_str());

		typename EC::FieldElement Qx(x, x.size());
		typename EC::FieldElement Qy(y, y.size());
		typename EC::Element Q(Qx, Qy);

		DL_GroupParameters_EC<EC> params(oid);
		typename ECDSA<EC, SHA1>::PublicKey pub;
		pub.Initialize(params, Q);
		typename ECDSA<EC, SHA1>::Verifier verifier(pub);

		SecByteBlock sig(verifier.SignatureLength());
		r.Encode(sig, sig.size()/2);
		s.Encode(sig+sig.size()/2, sig.size()/2);

		SignatureVerificationFilter filter(verifier);
		filter.Put(sig, sig.size());
		StringSource(m_data["Msg"], true, new HexDecoder(new Redirector(filter, Redirector::DATA_ONLY)));
		filter.MessageEnd();
		byte b;
		filter.Get(b);
		OutputData(output, "Result ", b ? "P" : "F");
	}

	template <class EC>
	static bool EC_PKV(RandomNumberGenerator &rng, const SecByteBlock &x, const SecByteBlock &y, const OID &oid)
	{
		typename EC::FieldElement Qx(x, x.size());
		typename EC::FieldElement Qy(y, y.size());
		typename EC::Element Q(Qx, Qy);

		DL_GroupParameters_EC<EC> params(oid);
		typename ECDSA<EC, SHA1>::PublicKey pub;
		pub.Initialize(params, Q);
		return pub.Validate(rng, 3);
	}

	template <class H, class Result>
	Result * CreateRSA2(const std::string &standard)
	{
		if (typeid(Result) == typeid(PK_Verifier))
		{
			if (standard == "R")
				return (Result *) new typename RSASS_ISO<H>::Verifier;
			else if (standard == "P")
				return (Result *) new typename RSASS<PSS, H>::Verifier;
			else if (standard == "1")
				return (Result *) new typename RSASS<PKCS1v15, H>::Verifier;
		}
		else if (typeid(Result) == typeid(PK_Signer))
		{
			if (standard == "R")
				return (Result *) new typename RSASS_ISO<H>::Signer;
			else if (standard == "P")
				return (Result *) new typename RSASS<PSS, H>::Signer;
			else if (standard == "1")
				return (Result *) new typename RSASS<PKCS1v15, H>::Signer;
		}

		return NULL;
	}

	template <class Result>
	Result * CreateRSA(const std::string &standard, const std::string &hash)
	{
		if (hash == "1")
			return CreateRSA2<SHA1, Result>(standard);
		else if (hash == "224")
			return CreateRSA2<SHA224, Result>(standard);
		else if (hash == "256")
			return CreateRSA2<SHA256, Result>(standard);
		else if (hash == "384")
			return CreateRSA2<SHA384, Result>(standard);
		else if (hash == "512")
			return CreateRSA2<SHA512, Result>(standard);
		else
			return NULL;
	}

	virtual void DoTest()
	{
		std::string output;

		if (m_algorithm == "DSA")
		{
			if (m_test == "KeyPair")
			{
				DL_GroupParameters_DSA pqg;
				int modLen = atol(m_bracketString.substr(6).c_str());
				pqg.GenerateRandomWithKeySize(m_rng, modLen);

				OutputData(output, "P ", pqg.GetModulus());
				OutputData(output, "Q ", pqg.GetSubgroupOrder());
				OutputData(output, "G ", pqg.GetSubgroupGenerator());

				int n = atol(m_data["N"].c_str());
				for (int i=0; i<n; i++)
				{
					DSA::Signer priv;
					priv.AccessKey().GenerateRandom(m_rng, pqg);
					DSA::Verifier pub(priv);

					OutputData(output, "X ", priv.GetKey().GetPrivateExponent());
					OutputData(output, "Y ", pub.GetKey().GetPublicElement());
					AttachedTransformation()->Put((byte *)output.data(), output.size());
					output.resize(0);
				}
			}
			else if (m_test == "PQGGen")
			{
				int n = atol(m_data["N"].c_str());
				for (int i=0; i<n; i++)
				{
					Integer p, q, h, g;
					int counter;

					SecByteBlock seed(SHA::DIGESTSIZE);
					do
					{
						m_rng.GenerateBlock(seed, seed.size());
					}
					while (!DSA::GeneratePrimes(seed, seed.size()*8, counter, p, 1024, q));
					h.Randomize(m_rng, 2, p-2);
					g = a_exp_b_mod_c(h, (p-1)/q, p);

					OutputData(output, "P ", p);
					OutputData(output, "Q ", q);
					OutputData(output, "G ", g);
					OutputData(output, "Seed ", seed);
					OutputData(output, "c ", counter);
					OutputData(output, "H ", h, p.ByteCount());
					AttachedTransformation()->Put((byte *)output.data(), output.size());
					output.resize(0);
				}
			}
			else if (m_test == "SigGen")
			{
				std::string &encodedKey = m_data["PrivKey"];
				int modLen = atol(m_bracketString.substr(6).c_str());
				DSA::PrivateKey priv;

				if (!encodedKey.empty())
				{
					StringStore s(encodedKey);
					priv.BERDecode(s);
					if (priv.GetGroupParameters().GetModulus().BitCount() != modLen)
						encodedKey.clear();
				}

				if (encodedKey.empty())
				{
					priv.Initialize(m_rng, modLen);
					StringSink s(encodedKey);
					priv.DEREncode(s);
					OutputData(output, "P ", priv.GetGroupParameters().GetModulus());
					OutputData(output, "Q ", priv.GetGroupParameters().GetSubgroupOrder());
					OutputData(output, "G ", priv.GetGroupParameters().GetSubgroupGenerator());
				}

				DSA::Signer signer(priv);
				DSA::Verifier pub(signer);
				OutputData(output, "Msg ", m_data["Msg"]);
				OutputData(output, "Y ", pub.GetKey().GetPublicElement());

				SecByteBlock sig(signer.SignatureLength());
				StringSource(m_data["Msg"], true, new HexDecoder(new SignerFilter(m_rng, signer, new ArraySink(sig, sig.size()))));
				SecByteBlock R(sig, sig.size()/2), S(sig+sig.size()/2, sig.size()/2);
				OutputData(output, "R ", R);
				OutputData(output, "S ", S);
				AttachedTransformation()->Put((byte *)output.data(), output.size());
				output.resize(0);
			}
			else if (m_test == "SigVer")
			{
				Integer p((m_data["P"] + "h").c_str());
				Integer	q((m_data["Q"] + "h").c_str());
				Integer g((m_data["G"] + "h").c_str());
				Integer y((m_data["Y"] + "h").c_str());
				DSA::Verifier verifier(p, q, g, y);

				HexDecoder filter(new SignatureVerificationFilter(verifier));
				StringSource(m_data["R"], true, new Redirector(filter, Redirector::DATA_ONLY));
				StringSource(m_data["S"], true, new Redirector(filter, Redirector::DATA_ONLY));
				StringSource(m_data["Msg"], true, new Redirector(filter, Redirector::DATA_ONLY));
				filter.MessageEnd();
				byte b;
				filter.Get(b);
				OutputData(output, "Result ", b ? "P" : "F");
				AttachedTransformation()->Put((byte *)output.data(), output.size());
				output.resize(0);
			}
			else if (m_test == "PQGVer")
			{
				Integer p((m_data["P"] + "h").c_str());
				Integer	q((m_data["Q"] + "h").c_str());
				Integer g((m_data["G"] + "h").c_str());
				Integer h((m_data["H"] + "h").c_str());
				int c = atol(m_data["c"].c_str());
				SecByteBlock seed(m_data["Seed"].size()/2);
				StringSource(m_data["Seed"], true, new HexDecoder(new ArraySink(seed, seed.size())));

				Integer p1, q1;
				bool result = DSA::GeneratePrimes(seed, seed.size()*8, c, p1, 1024, q1, true);
				result = result && (p1 == p && q1 == q);
				result = result && g == a_exp_b_mod_c(h, (p-1)/q, p);

				OutputData(output, "Result ", result ? "P" : "F");
				AttachedTransformation()->Put((byte *)output.data(), output.size());
				output.resize(0);
			}

			return;
		}

		if (m_algorithm == "ECDSA")
		{
			std::map<std::string, OID> name2oid;
			name2oid["P-192"] = ASN1::secp192r1();
			name2oid["P-224"] = ASN1::secp224r1();
			name2oid["P-256"] = ASN1::secp256r1();
			name2oid["P-384"] = ASN1::secp384r1();
			name2oid["P-521"] = ASN1::secp521r1();
			name2oid["K-163"] = ASN1::sect163k1();
			name2oid["K-233"] = ASN1::sect233k1();
			name2oid["K-283"] = ASN1::sect283k1();
			name2oid["K-409"] = ASN1::sect409k1();
			name2oid["K-571"] = ASN1::sect571k1();
			name2oid["B-163"] = ASN1::sect163r2();
			name2oid["B-233"] = ASN1::sect233r1();
			name2oid["B-283"] = ASN1::sect283r1();
			name2oid["B-409"] = ASN1::sect409r1();
			name2oid["B-571"] = ASN1::sect571r1();

			if (m_test == "PKV")
			{
				bool pass;
				if (m_bracketString[0] == 'P')
					pass = EC_PKV<ECP>(m_rng, DecodeHex(m_data["Qx"]), DecodeHex(m_data["Qy"]), name2oid[m_bracketString]);
				else
					pass = EC_PKV<EC2N>(m_rng, DecodeHex(m_data["Qx"]), DecodeHex(m_data["Qy"]), name2oid[m_bracketString]);

				OutputData(output, "Result ", pass ? "P" : "F");
			}
			else if (m_test == "KeyPair")
			{
				if (m_bracketString[0] == 'P')
					EC_KeyPair<ECP>(output, atol(m_data["N"].c_str()), name2oid[m_bracketString]);
				else
					EC_KeyPair<EC2N>(output, atol(m_data["N"].c_str()), name2oid[m_bracketString]);
			}
			else if (m_test == "SigGen")
			{
				if (m_bracketString[0] == 'P')
					EC_SigGen<ECP>(output, name2oid[m_bracketString]);
				else
					EC_SigGen<EC2N>(output, name2oid[m_bracketString]);
			}
			else if (m_test == "SigVer")
			{
				if (m_bracketString[0] == 'P')
					EC_SigVer<ECP>(output, name2oid[m_bracketString]);
				else
					EC_SigVer<EC2N>(output, name2oid[m_bracketString]);
			}

			AttachedTransformation()->Put((byte *)output.data(), output.size());
			output.resize(0);
			return;
		}

		if (m_algorithm == "RSA")
		{
			std::string shaAlg = m_data["SHAAlg"].substr(3);

			if (m_test == "Ver")
			{
				Integer n((m_data["n"] + "h").c_str());
				Integer e((m_data["e"] + "h").c_str());
				RSA::PublicKey pub;
				pub.Initialize(n, e);

				member_ptr<PK_Verifier> pV(CreateRSA<PK_Verifier>(m_mode, shaAlg));
				pV->AccessMaterial().AssignFrom(pub);

				HexDecoder filter(new SignatureVerificationFilter(*pV));
				for (unsigned int i=m_data["S"].size(); i<pV->SignatureLength()*2; i++)
					filter.Put('0');
				StringSource(m_data["S"], true, new Redirector(filter, Redirector::DATA_ONLY));
				StringSource(m_data["Msg"], true, new Redirector(filter, Redirector::DATA_ONLY));
				filter.MessageEnd();
				byte b;
				filter.Get(b);
				OutputData(output, "Result ", b ? "P" : "F");
			}
			else
			{
				CRYPTOPP_ASSERT(m_test == "Gen");
				int modLen = atol(m_bracketString.substr(6).c_str());
				std::string &encodedKey = m_data["PrivKey"];
				RSA::PrivateKey priv;

				if (!encodedKey.empty())
				{
					StringStore s(encodedKey);
					priv.BERDecode(s);
					if (priv.GetModulus().BitCount() != modLen)
						encodedKey.clear();
				}

				if (encodedKey.empty())
				{
					priv.Initialize(m_rng, modLen);
					StringSink s(encodedKey);
					priv.DEREncode(s);
					OutputData(output, "n ", priv.GetModulus());
					OutputData(output, "e ", priv.GetPublicExponent(), modLen/8);
				}

				member_ptr<PK_Signer> pS(CreateRSA<PK_Signer>(m_mode, shaAlg));
				pS->AccessMaterial().AssignFrom(priv);

				SecByteBlock sig(pS->SignatureLength());
				StringSource(m_data["Msg"], true, new HexDecoder(new SignerFilter(m_rng, *pS, new ArraySink(sig, sig.size()))));
				OutputData(output, "SHAAlg ", m_data["SHAAlg"]);
				OutputData(output, "Msg ", m_data["Msg"]);
				OutputData(output, "S ", sig);
			}

			AttachedTransformation()->Put((byte *)output.data(), output.size());
			output.resize(0);
			return;
		}

		if (m_algorithm == "SHA")
		{
			member_ptr<HashFunction> pHF;

			if (m_mode == "1")
				pHF.reset(new SHA1);
			else if (m_mode == "224")
				pHF.reset(new SHA224);
			else if (m_mode == "256")
				pHF.reset(new SHA256);
			else if (m_mode == "384")
				pHF.reset(new SHA384);
			else if (m_mode == "512")
				pHF.reset(new SHA512);

			if (m_test == "MONTE")
			{
				SecByteBlock seed = m_data2[INPUT];
				SecByteBlock MD[1003];
				int i,j;

				for (j=0; j<100; j++)
				{
					MD[0] = MD[1] = MD[2] = seed;
					for (i=3; i<1003; i++)
					{
						SecByteBlock Mi = MD[i-3] + MD[i-2] + MD[i-1];
						MD[i].resize(pHF->DigestSize());
						pHF->CalculateDigest(MD[i], Mi, Mi.size());
					}
					seed = MD[1002];
					OutputData(output, "COUNT ", j);
					OutputData(output, "MD ", seed);
					AttachedTransformation()->Put((byte *)output.data(), output.size());
					output.resize(0);
				}
			}
			else
			{
				SecByteBlock tag(pHF->DigestSize());
				SecByteBlock &msg(m_data2[INPUT]);
				int len = atol(m_data["Len"].c_str());
				StringSource(msg.begin(), len/8, true, new HashFilter(*pHF, new ArraySink(tag, tag.size())));
				OutputData(output, "MD ", tag);
				AttachedTransformation()->Put((byte *)output.data(), output.size());
				output.resize(0);
			}
			return;
		}

		SecByteBlock &key = m_data2[KEY_T];

		if (m_algorithm == "TDES")
		{
			if (!m_data["KEY1"].empty())
			{
				const std::string keys[3] = {m_data["KEY1"], m_data["KEY2"], m_data["KEY3"]};
				key.resize(24);
				HexDecoder hexDec(new ArraySink(key, key.size()));
				for (int i=0; i<3; i++)
					hexDec.Put((byte *)keys[i].data(), keys[i].size());

				if (keys[0] == keys[2])
				{
					if (keys[0] == keys[1])
						key.resize(8);
					else
						key.resize(16);
				}
				else
					key.resize(24);
			}
		}

		if (m_algorithm == "RNG")
		{
			key.resize(24);
			StringSource(m_data["Key1"] + m_data["Key2"] + m_data["Key3"], true, new HexDecoder(new ArraySink(key, key.size())));

			SecByteBlock seed(m_data2[INPUT]), dt(m_data2[IV]), r(8);
			X917RNG rng(new DES_EDE3::Encryption(key, key.size()), seed, dt);

			if (m_test == "MCT")
			{
				for (int i=0; i<10000; i++)
					rng.GenerateBlock(r, r.size());
			}
			else
			{
				rng.GenerateBlock(r, r.size());
			}

			OutputData(output, "R ", r);
			AttachedTransformation()->Put((byte *)output.data(), output.size());
			output.resize(0);
			return;
		}

		if (m_algorithm == "HMAC")
		{
			member_ptr<MessageAuthenticationCode> pMAC;

			if (m_bracketString == "L=20")
				pMAC.reset(new HMAC<SHA1>);
			else if (m_bracketString == "L=28")
				pMAC.reset(new HMAC<SHA224>);
			else if (m_bracketString == "L=32")
				pMAC.reset(new HMAC<SHA256>);
			else if (m_bracketString == "L=48")
				pMAC.reset(new HMAC<SHA384>);
			else if (m_bracketString == "L=64")
				pMAC.reset(new HMAC<SHA512>);
			else
				throw Exception(Exception::OTHER_ERROR, "TestDataParser: unexpected HMAC bracket string: " + m_bracketString);

			pMAC->SetKey(key, key.size());
			int Tlen = atol(m_data["Tlen"].c_str());
			SecByteBlock tag(Tlen);
			StringSource(m_data["Msg"], true, new HexDecoder(new HashFilter(*pMAC, new ArraySink(tag, Tlen), false, Tlen)));
			OutputData(output, "Mac ", tag);
			AttachedTransformation()->Put((byte *)output.data(), output.size());
			output.resize(0);
			return;
		}

		member_ptr<BlockCipher> pBT;
		if (m_algorithm == "DES")
			pBT.reset(NewBT((DES*)0));
		else if (m_algorithm == "TDES")
		{
			if (key.size() == 8)
				pBT.reset(NewBT((DES*)0));
			else if (key.size() == 16)
				pBT.reset(NewBT((DES_EDE2*)0));
			else
				pBT.reset(NewBT((DES_EDE3*)0));
		}
		else if (m_algorithm == "SKIPJACK")
			pBT.reset(NewBT((SKIPJACK*)0));
		else if (m_algorithm == "AES")
			pBT.reset(NewBT((AES*)0));
		else
			throw Exception(Exception::OTHER_ERROR, "TestDataParser: unexpected algorithm: " + m_algorithm);

		if (!pBT->IsValidKeyLength(key.size()))
			key.CleanNew(pBT->DefaultKeyLength());	// for Scbcvrct
		pBT->SetKey(key.data(), key.size());

		SecByteBlock &iv = m_data2[IV];
		if (iv.empty())
			iv.CleanNew(pBT->BlockSize());

		member_ptr<SymmetricCipher> pCipher;
		unsigned int K = m_feedbackSize;

		if (m_mode == "ECB")
			pCipher.reset(NewMode((ECB_Mode_ExternalCipher*)0, *pBT, iv));
		else if (m_mode == "CBC")
			pCipher.reset(NewMode((CBC_Mode_ExternalCipher*)0, *pBT, iv));
		else if (m_mode == "CFB")
			pCipher.reset(NewMode((CFB_Mode_ExternalCipher*)0, *pBT, iv));
		else if (m_mode == "OFB")
			pCipher.reset(NewMode((OFB_Mode_ExternalCipher*)0, *pBT, iv));
		else
			throw Exception(Exception::OTHER_ERROR, "TestDataParser: unexpected mode: " + m_mode);

		bool encrypt = m_encrypt;

		if (m_test == "MONTE")
		{
			SecByteBlock KEY[401];
			KEY[0] = key;
			int keySize = key.size();
			int blockSize = pBT->BlockSize();

			std::vector<SecByteBlock> IB(10001), OB(10001), PT(10001), CT(10001), RESULT(10001), TXT(10001), CV(10001);
			PT[0] = GetData("PLAINTEXT");
			CT[0] = GetData("CIPHERTEXT");
			CV[0] = IB[0] = iv;
			TXT[0] = GetData("TEXT");

			int outerCount = (m_algorithm == "AES") ? 100 : 400;
			int innerCount = (m_algorithm == "AES") ? 1000 : 10000;

			for (int i=0; i<outerCount; i++)
			{
				pBT->SetKey(KEY[i], keySize);

				for (int j=0; j<innerCount; j++)
				{
					if (m_mode == "ECB")
					{
						if (encrypt)
						{
							IB[j] = PT[j];
							CT[j].resize(blockSize);
							pBT->ProcessBlock(IB[j], CT[j]);
							PT[j+1] = CT[j];
						}
						else
						{
							IB[j] = CT[j];
							PT[j].resize(blockSize);
							pBT->ProcessBlock(IB[j], PT[j]);
							CT[j+1] = PT[j];
						}
					}
					else if (m_mode == "OFB")
					{
						OB[j].resize(blockSize);
						pBT->ProcessBlock(IB[j], OB[j]);
						Xor(RESULT[j], OB[j], TXT[j]);
						TXT[j+1] = IB[j];
						IB[j+1] = OB[j];
					}
					else if (m_mode == "CBC")
					{
						if (encrypt)
						{
							Xor(IB[j], PT[j], CV[j]);
							CT[j].resize(blockSize);
							pBT->ProcessBlock(IB[j], CT[j]);
							PT[j+1] = CV[j];
							CV[j+1] = CT[j];
						}
						else
						{
							IB[j] = CT[j];
							OB[j].resize(blockSize);
							pBT->ProcessBlock(IB[j], OB[j]);
							Xor(PT[j], OB[j], CV[j]);
							CV[j+1] = CT[j];
							CT[j+1] = PT[j];
						}
					}
					else if (m_mode == "CFB")
					{
						if (encrypt)
						{
							OB[j].resize(blockSize);
							pBT->ProcessBlock(IB[j], OB[j]);
							AssignLeftMostBits(CT[j], OB[j], K);
							Xor(CT[j], CT[j], PT[j]);
							AssignLeftMostBits(PT[j+1], IB[j], K);
							IB[j+1].resize(blockSize);
							memcpy(IB[j+1], IB[j]+K/8, blockSize-K/8);
							memcpy(IB[j+1]+blockSize-K/8, CT[j], K/8);
						}
						else
						{
							OB[j].resize(blockSize);
							pBT->ProcessBlock(IB[j], OB[j]);
							AssignLeftMostBits(PT[j], OB[j], K);
							Xor(PT[j], PT[j], CT[j]);
							IB[j+1].resize(blockSize);
							memcpy(IB[j+1], IB[j]+K/8, blockSize-K/8);
							memcpy(IB[j+1]+blockSize-K/8, CT[j], K/8);
							AssignLeftMostBits(CT[j+1], OB[j], K);
						}
					}
					else
						throw Exception(Exception::OTHER_ERROR, "TestDataParser: unexpected mode: " + m_mode);
				}

				OutputData(output, COUNT, IntToString(i));
				OutputData(output, KEY_T, KEY[i]);
				if (m_mode == "CBC")
					OutputData(output, IV, CV[0]);
				if (m_mode == "OFB" || m_mode == "CFB")
					OutputData(output, IV, IB[0]);
				if (m_mode == "ECB" || m_mode == "CBC" || m_mode == "CFB")
				{
					if (encrypt)
					{
						OutputData(output, INPUT, PT[0]);
						OutputData(output, OUTPUT, CT[innerCount-1]);
						KEY[i+1] = UpdateKey(KEY[i], &CT[0]);
					}
					else
					{
						OutputData(output, INPUT, CT[0]);
						OutputData(output, OUTPUT, PT[innerCount-1]);
						KEY[i+1] = UpdateKey(KEY[i], &PT[0]);
					}
					PT[0] = PT[innerCount];
					IB[0] = IB[innerCount];
					CV[0] = CV[innerCount];
					CT[0] = CT[innerCount];
				}
				else if (m_mode == "OFB")
				{
					OutputData(output, INPUT, TXT[0]);
					OutputData(output, OUTPUT, RESULT[innerCount-1]);
					KEY[i+1] = UpdateKey(KEY[i], &RESULT[0]);
					Xor(TXT[0], TXT[0], IB[innerCount-1]);
					IB[0] = OB[innerCount-1];
				}
				output += "\n";
				AttachedTransformation()->Put((byte *)output.data(), output.size());
				output.resize(0);
			}
		}
		else if (m_test == "MCT")
		{
			SecByteBlock KEY[101];
			KEY[0] = key;
			int keySize = key.size();
			int blockSize = pBT->BlockSize();

			SecByteBlock ivs[101], inputs[1001], outputs[1001];
			ivs[0] = iv;
			inputs[0] = m_data2[INPUT];

			for (int i=0; i<100; i++)
			{
				pCipher->SetKey(KEY[i], keySize, MakeParameters(Name::IV(), (const byte *)ivs[i])(Name::FeedbackSize(), (int)K/8, false));

				for (int j=0; j<1000; j++)
				{
					outputs[j] = inputs[j];
					pCipher->ProcessString(outputs[j], outputs[j].size());
					if (K==8 && m_mode == "CFB")
					{
						if (j<16)
							inputs[j+1].Assign(ivs[i]+j, 1);
						else
							inputs[j+1] = outputs[j-16];
					}
					else if (m_mode == "ECB")
						inputs[j+1] = outputs[j];
					else if (j == 0)
						inputs[j+1] = ivs[i];
					else
						inputs[j+1] = outputs[j-1];
				}

				if (m_algorithm == "AES")
					OutputData(output, COUNT, m_count++);
				OutputData(output, KEY_T, KEY[i]);
				if (m_mode != "ECB")
					OutputData(output, IV, ivs[i]);
				OutputData(output, INPUT, inputs[0]);
				OutputData(output, OUTPUT, outputs[999]);
				output += "\n";
				AttachedTransformation()->Put((byte *)output.data(), output.size());
				output.resize(0);

				KEY[i+1] = UpdateKey(KEY[i], outputs);
				ivs[i+1].CleanNew(pCipher->IVSize());
				ivs[i+1] = UpdateKey(ivs[i+1], outputs);
				if (K==8 && m_mode == "CFB")
					inputs[0] = outputs[999-16];
				else if (m_mode == "ECB")
					inputs[0] = outputs[999];
				else
					inputs[0] = outputs[998];
			}
		}
		else
		{
			CRYPTOPP_ASSERT(m_test == "KAT");

			SecByteBlock &input = m_data2[INPUT];
			SecByteBlock result(input.size());
			member_ptr<Filter> pFilter(new StreamTransformationFilter(*pCipher, new ArraySink(result, result.size()), StreamTransformationFilter::NO_PADDING));
			StringSource(input.data(), input.size(), true, pFilter.release());

			OutputGivenData(output, COUNT, true);
			OutputData(output, KEY_T, key);
			OutputGivenData(output, IV, true);
			OutputGivenData(output, INPUT);
			OutputData(output, OUTPUT, result);
			output += "\n";
			AttachedTransformation()->Put((byte *)output.data(), output.size());
		}
	}

	std::vector<std::string> Tokenize(const std::string &line)
	{
		std::vector<std::string> result;
		std::string s;
		for (unsigned int i=0; i<line.size(); i++)
		{
			if (isalnum(line[i]) || line[i] == '^')
				s += line[i];
			else if (!s.empty())
			{
				result.push_back(s);
				s = "";
			}
			if (line[i] == '=')
				result.push_back("=");
		}
		if (!s.empty())
			result.push_back(s);
		return result;
	}

	bool IsolatedMessageEnd(bool blocking)
	{
		if (!blocking)
			throw BlockingInputOnly("TestDataParser");

		m_line.resize(0);
		m_inQueue.TransferTo(StringSink(m_line).Ref());

		if (m_line[0] == '#')
			return false;

		bool copyLine = false;

		if (m_line[0] == '[')
		{
			m_bracketString = m_line.substr(1, m_line.size()-2);
			if (m_bracketString == "ENCRYPT")
				SetEncrypt(true);
			if (m_bracketString == "DECRYPT")
				SetEncrypt(false);
			copyLine = true;
		}

		if (m_line.substr(0, 2) == "H>")
		{
			CRYPTOPP_ASSERT(m_test == "sha");
			m_bracketString = m_line.substr(2, m_line.size()-4);
			m_line = m_line.substr(0, 13) + "Hashes<H";
			copyLine = true;
		}

		if (m_line == "D>")
			copyLine = true;

		if (m_line == "<D")
		{
			m_line += "\n";
			copyLine = true;
		}

		if (copyLine)
		{
			m_line += '\n';
			AttachedTransformation()->Put((byte *)m_line.data(), m_line.size(), blocking);
			return false;
		}

		std::vector<std::string> tokens = Tokenize(m_line);

		if (m_algorithm == "DSA" && m_test == "sha")
		{
			for (unsigned int i = 0; i < tokens.size(); i++)
			{
				if (tokens[i] == "^")
					DoTest();
				else if (tokens[i] != "")
					m_compactString.push_back(atol(tokens[i].c_str()));
			}
		}
		else
		{
			if (!m_line.empty() && ((m_algorithm == "RSA" && m_test != "Gen") || m_algorithm == "RNG" || m_algorithm == "HMAC" || m_algorithm == "SHA" || (m_algorithm == "ECDSA" && m_test != "KeyPair") || (m_algorithm == "DSA" && (m_test == "PQGVer" || m_test == "SigVer"))))
			{
				// copy input to output
				std::string output = m_line + '\n';
				AttachedTransformation()->Put((byte *)output.data(), output.size());
			}

			for (unsigned int i = 0; i < tokens.size(); i++)
			{
				if (m_firstLine && m_algorithm != "DSA")
				{
					if (tokens[i] == "Encrypt" || tokens[i] == "OFB")
						SetEncrypt(true);
					else if (tokens[i] == "Decrypt")
						SetEncrypt(false);
					else if (tokens[i] == "Modes")
						m_test = "MONTE";
				}
				else
				{
					if (tokens[i] != "=")
						continue;

					if (i == 0)
						throw Exception(Exception::OTHER_ERROR, "TestDataParser: unexpected data: " + m_line);

					const std::string &key = tokens[i-1];
					std::string &data = m_data[key];
					data = (tokens.size() > i+1) ? tokens[i+1] : "";
					DataType t = m_nameToType[key];
					m_typeToName[t] = key;
					m_data2[t] = DecodeHex(data);

					if (key == m_trigger || (t == OUTPUT && !m_data2[INPUT].empty() && !isspace(m_line[0])))
						DoTest();
				}
			}
		}

		m_firstLine = false;

		return false;
	}

	inline const SecByteBlock & GetData(const std::string &key)
	{
		return m_data2[m_nameToType[key]];
	}

	static SecByteBlock DecodeHex(const std::string &data)
	{
		SecByteBlock data2(data.size() / 2);
		StringSource(data, true, new HexDecoder(new ArraySink(data2, data2.size())));
		return data2;
	}

	std::string m_algorithm, m_test, m_mode, m_line, m_bracketString, m_trigger;
	unsigned int m_feedbackSize, m_blankLineTransition;
	bool m_encrypt, m_firstLine;

	typedef std::map<std::string, DataType> NameToTypeMap;
	NameToTypeMap m_nameToType;
	typedef std::map<DataType, std::string> TypeToNameMap;
	TypeToNameMap m_typeToName;

	typedef std::map<std::string, std::string> Map;
	Map m_data;		// raw data
	typedef std::map<DataType, SecByteBlock> Map2;
	Map2 m_data2;
	int m_count;

	AutoSeededX917RNG<AES> m_rng;
	std::vector<unsigned int> m_compactString;
};

int FIPS_140_AlgorithmTest(int argc, char **argv)
{
	argc--;
	argv++;

	std::string algorithm = argv[1];
	std::string pathname = argv[2];
	unsigned int i = pathname.find_last_of("\\/");
	std::string filename = pathname.substr(i == std::string::npos ? 0 : i+1);
	std::string dirname = pathname.substr(0, i);

	if (algorithm == "auto")
	{
		string algTable[] = {"AES", "ECDSA", "DSA", "HMAC", "RNG", "RSA", "TDES", "SKIPJACK", "SHA"};	// order is important here
		for (i=0; i<sizeof(algTable)/sizeof(algTable[0]); i++)
		{
			if (dirname.find(algTable[i]) != std::string::npos)
			{
				algorithm = algTable[i];
				break;
			}
		}
	}

	try
	{
		std::string mode;
		if (algorithm == "SHA")
			mode = IntToString(atol(filename.substr(3, 3).c_str()));
		else if (algorithm == "RSA")
			mode = filename.substr(6, 1);
		else if (filename[0] == 'S' || filename[0] == 'T')
			mode = filename.substr(1, 3);
		else
			mode = filename.substr(0, 3);
		for (i = 0; i<mode.size(); i++)
			mode[i] = toupper(mode[i]);
		unsigned int feedbackSize = mode == "CFB" ? atoi(filename.substr(filename.find_first_of("0123456789")).c_str()) : 0;
		std::string test;
		if (algorithm == "DSA" || algorithm == "ECDSA")
			test = filename.substr(0, filename.size() - 4);
		else if (algorithm == "RSA")
			test = filename.substr(3, 3);
		else if (filename.find("Monte") != std::string::npos)
			test = "MONTE";
		else if (filename.find("MCT") != std::string::npos)
			test = "MCT";
		else
			test = "KAT";
		bool encrypt = (filename.find("vrct") == std::string::npos);

		BufferedTransformation *pSink = NULL;

		if (argc > 3)
		{
			std::string outDir = argv[3];

			if (outDir == "auto")
			{
				if (dirname.substr(dirname.size()-3) == "req")
					outDir = dirname.substr(0, dirname.size()-3) + "resp";
			}

			if (*outDir.rbegin() != '\\' && *outDir.rbegin() != '/')
				outDir += '/';
			std::string outPathname = outDir + filename.substr(0, filename.size() - 3) + "rsp";
			pSink = new FileSink(outPathname.c_str(), false);
		}
		else
			pSink = new FileSink(cout);

		FileSource(pathname.c_str(), true, new LineBreakParser(new TestDataParser(algorithm, test, mode, feedbackSize, encrypt, pSink)), false);
	}
	catch (...)
	{
		cout << "file: " << filename << endl;
		throw;
	}
	return 0;
}

extern int (*AdhocTest)(int argc, char *argv[]);
static int s_i = (AdhocTest = &FIPS_140_AlgorithmTest, 0);
#endif
