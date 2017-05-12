// fipstest.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

#ifndef CRYPTOPP_IMPORTS

#define CRYPTOPP_DEFAULT_NO_DLL
#include "dll.h"
#include "cryptlib.h"
#include "filters.h"
#include "smartptr.h"
#include "misc.h"

// Simply disable CRYPTOPP_WIN32_AVAILABLE for Windows Phone and Windows Store apps
#ifdef CRYPTOPP_WIN32_AVAILABLE
# if defined(WINAPI_FAMILY)
#   if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#	  undef CRYPTOPP_WIN32_AVAILABLE
#   endif
# endif
#endif

#ifdef CRYPTOPP_WIN32_AVAILABLE
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>

#if defined(_MSC_VER) && _MSC_VER >= 1400
#ifdef _M_IX86
#define _CRT_DEBUGGER_HOOK _crt_debugger_hook
#else
#define _CRT_DEBUGGER_HOOK __crt_debugger_hook
#endif
#if _MSC_VER < 1900
extern "C" {_CRTIMP void __cdecl _CRT_DEBUGGER_HOOK(int);}
#else
extern "C" {void __cdecl _CRT_DEBUGGER_HOOK(int); }
#endif
#endif
#endif

#include <sstream>
#include <iostream>

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4100)
#endif

NAMESPACE_BEGIN(CryptoPP)

extern PowerUpSelfTestStatus g_powerUpSelfTestStatus;
SecByteBlock g_actualMac;
unsigned long g_macFileLocation = 0;

// $ grep -iIR baseaddress *.*proj
// cryptdll.vcxproj:      <BaseAddress>0x42900000</BaseAddress>
// cryptdll.vcxproj:      <BaseAddress>0x42900000</BaseAddress>
// cryptdll.vcxproj:      <BaseAddress>0x42900000</BaseAddress>
// cryptdll.vcxproj:      <BaseAddress>0x42900000</BaseAddress>
const void* g_BaseAddressOfMAC = reinterpret_cast<void*>(0x42900000);

// use a random dummy string here, to be searched/replaced later with the real MAC
static const byte s_moduleMac[CryptoPP::HMAC<CryptoPP::SHA1>::DIGESTSIZE] = CRYPTOPP_DUMMY_DLL_MAC;
CRYPTOPP_COMPILE_ASSERT(sizeof(s_moduleMac) == CryptoPP::SHA1::DIGESTSIZE);

#ifdef CRYPTOPP_WIN32_AVAILABLE
static HMODULE s_hModule = NULL;
#endif

const byte * CRYPTOPP_API GetActualMacAndLocation(unsigned int &macSize, unsigned int &fileLocation)
{
	macSize = (unsigned int)g_actualMac.size();
	fileLocation = g_macFileLocation;
	return g_actualMac;
}

void KnownAnswerTest(RandomNumberGenerator &rng, const char *output)
{
	EqualityComparisonFilter comparison;

	RandomNumberStore(rng, strlen(output)/2).TransferAllTo(comparison, "0");
	StringSource(output, true, new HexDecoder(new ChannelSwitch(comparison, "1")));

	comparison.ChannelMessageSeriesEnd("0");
	comparison.ChannelMessageSeriesEnd("1");
}

template <class CIPHER>
void X917RNG_KnownAnswerTest(
	const char *key,
	const char *seed,
	const char *deterministicTimeVector,
	const char *output,
	CIPHER *dummy = NULL)
{
	CRYPTOPP_UNUSED(dummy);
#ifdef OS_RNG_AVAILABLE
	std::string decodedKey, decodedSeed, decodedDeterministicTimeVector;
	StringSource(key, true, new HexDecoder(new StringSink(decodedKey)));
	StringSource(seed, true, new HexDecoder(new StringSink(decodedSeed)));
	StringSource(deterministicTimeVector, true, new HexDecoder(new StringSink(decodedDeterministicTimeVector)));

	AutoSeededX917RNG<CIPHER> rng(false, false);
	rng.Reseed((const byte *)decodedKey.data(), decodedKey.size(), (const byte *)decodedSeed.data(), (const byte *)decodedDeterministicTimeVector.data());
	KnownAnswerTest(rng, output);
#else
	throw 0;
#endif
}

void KnownAnswerTest(StreamTransformation &encryption, StreamTransformation &decryption, const char *plaintext, const char *ciphertext)
{
	EqualityComparisonFilter comparison;

	StringSource(plaintext, true, new HexDecoder(new StreamTransformationFilter(encryption, new ChannelSwitch(comparison, "0"), StreamTransformationFilter::NO_PADDING)));
	StringSource(ciphertext, true, new HexDecoder(new ChannelSwitch(comparison, "1")));

	StringSource(ciphertext, true, new HexDecoder(new StreamTransformationFilter(decryption, new ChannelSwitch(comparison, "0"), StreamTransformationFilter::NO_PADDING)));
	StringSource(plaintext, true, new HexDecoder(new ChannelSwitch(comparison, "1")));

	comparison.ChannelMessageSeriesEnd("0");
	comparison.ChannelMessageSeriesEnd("1");
}

template <class CIPHER>
void SymmetricEncryptionKnownAnswerTest(
	const char *key,
	const char *hexIV,
	const char *plaintext,
	const char *ecb,
	const char *cbc,
	const char *cfb,
	const char *ofb,
	const char *ctr,
	CIPHER *dummy = NULL)
{
	CRYPTOPP_UNUSED(dummy);
	std::string decodedKey;
	StringSource(key, true, new HexDecoder(new StringSink(decodedKey)));

	typename CIPHER::Encryption encryption((const byte *)decodedKey.data(), decodedKey.size());
	typename CIPHER::Decryption decryption((const byte *)decodedKey.data(), decodedKey.size());

	SecByteBlock iv(encryption.BlockSize());
	StringSource(hexIV, true, new HexDecoder(new ArraySink(iv, iv.size())));

	if (ecb)
		KnownAnswerTest(ECB_Mode_ExternalCipher::Encryption(encryption).Ref(), ECB_Mode_ExternalCipher::Decryption(decryption).Ref(), plaintext, ecb);
	if (cbc)
		KnownAnswerTest(CBC_Mode_ExternalCipher::Encryption(encryption, iv).Ref(), CBC_Mode_ExternalCipher::Decryption(decryption, iv).Ref(), plaintext, cbc);
	if (cfb)
		KnownAnswerTest(CFB_Mode_ExternalCipher::Encryption(encryption, iv).Ref(), CFB_Mode_ExternalCipher::Decryption(encryption, iv).Ref(), plaintext, cfb);
	if (ofb)
		KnownAnswerTest(OFB_Mode_ExternalCipher::Encryption(encryption, iv).Ref(), OFB_Mode_ExternalCipher::Decryption(encryption, iv).Ref(), plaintext, ofb);
	if (ctr)
		KnownAnswerTest(CTR_Mode_ExternalCipher::Encryption(encryption, iv).Ref(), CTR_Mode_ExternalCipher::Decryption(encryption, iv).Ref(), plaintext, ctr);
}

void KnownAnswerTest(HashTransformation &hash, const char *message, const char *digest)
{
	EqualityComparisonFilter comparison;
	StringSource(digest, true, new HexDecoder(new ChannelSwitch(comparison, "1")));
	StringSource(message, true, new HashFilter(hash, new ChannelSwitch(comparison, "0")));

	comparison.ChannelMessageSeriesEnd("0");
	comparison.ChannelMessageSeriesEnd("1");
}

template <class HASH>
void SecureHashKnownAnswerTest(const char *message, const char *digest, HASH *dummy = NULL)
{
	CRYPTOPP_UNUSED(dummy);
	HASH hash;
	KnownAnswerTest(hash, message, digest);
}

template <class MAC>
void MAC_KnownAnswerTest(const char *key, const char *message, const char *digest, MAC *dummy = NULL)
{
	CRYPTOPP_UNUSED(dummy);
	std::string decodedKey;
	StringSource(key, true, new HexDecoder(new StringSink(decodedKey)));

	MAC mac((const byte *)decodedKey.data(), decodedKey.size());
	KnownAnswerTest(mac, message, digest);
}

template <class SCHEME>
void SignatureKnownAnswerTest(const char *key, const char *message, const char *signature, SCHEME *dummy = NULL)
{
	typename SCHEME::Signer signer(StringSource(key, true, new HexDecoder).Ref());
	typename SCHEME::Verifier verifier(signer);

	CRYPTOPP_UNUSED(dummy);
	RandomPool rng;
	EqualityComparisonFilter comparison;

	StringSource(message, true, new SignerFilter(rng, signer, new ChannelSwitch(comparison, "0")));
	StringSource(signature, true, new HexDecoder(new ChannelSwitch(comparison, "1")));

	comparison.ChannelMessageSeriesEnd("0");
	comparison.ChannelMessageSeriesEnd("1");

	VerifierFilter verifierFilter(verifier, NULL, VerifierFilter::SIGNATURE_AT_BEGIN | VerifierFilter::THROW_EXCEPTION);
	StringSource(signature, true, new HexDecoder(new Redirector(verifierFilter, Redirector::DATA_ONLY)));
	StringSource(message, true, new Redirector(verifierFilter));
}

void EncryptionPairwiseConsistencyTest(const PK_Encryptor &encryptor, const PK_Decryptor &decryptor)
{
	try
	{
		RandomPool rng;
		const char *testMessage ="test message";
		std::string ciphertext, decrypted;

		StringSource(
			testMessage,
			true,
			new PK_EncryptorFilter(
				rng,
				encryptor,
				new StringSink(ciphertext)));

		if (ciphertext == testMessage)
			throw 0;

		StringSource(
			ciphertext,
			true,
			new PK_DecryptorFilter(
				rng,
				decryptor,
				new StringSink(decrypted)));

		if (decrypted != testMessage)
			throw 0;
	}
	catch (...)
	{
		throw SelfTestFailure(encryptor.AlgorithmName() + ": pairwise consistency test failed");
	}
}

void SignaturePairwiseConsistencyTest(const PK_Signer &signer, const PK_Verifier &verifier)
{
	try
	{
		RandomPool rng;

		StringSource(
			"test message",
			true,
			new SignerFilter(
				rng,
				signer,
				new VerifierFilter(verifier, NULL, VerifierFilter::THROW_EXCEPTION),
				true));
	}
	catch (...)
	{
		throw SelfTestFailure(signer.AlgorithmName() + ": pairwise consistency test failed");
	}
}

template <class SCHEME>
void SignaturePairwiseConsistencyTest(const char *key, SCHEME *dummy = NULL)
{
	typename SCHEME::Signer signer(StringSource(key, true, new HexDecoder).Ref());
	typename SCHEME::Verifier verifier(signer);

	CRYPTOPP_UNUSED(dummy);
	SignaturePairwiseConsistencyTest(signer, verifier);
}

MessageAuthenticationCode * NewIntegrityCheckingMAC()
{
	byte key[] = {0x47, 0x1E, 0x33, 0x96, 0x65, 0xB1, 0x6A, 0xED, 0x0B, 0xF8, 0x6B, 0xFD, 0x01, 0x65, 0x05, 0xCC};
	return new HMAC<SHA1>(key, sizeof(key));
}

bool IntegrityCheckModule(const char *moduleFilename, const byte *expectedModuleMac, SecByteBlock *pActualMac, unsigned long *pMacFileLocation)
{
	member_ptr<MessageAuthenticationCode> mac(NewIntegrityCheckingMAC());
	unsigned int macSize = mac->DigestSize();

	SecByteBlock tempMac;
	SecByteBlock &actualMac = pActualMac ? *pActualMac : tempMac;
	actualMac.resize(macSize);

	unsigned long tempLocation = 0;
	unsigned long &macFileLocation = pMacFileLocation ? *pMacFileLocation : tempLocation;
	macFileLocation = 0;

	MeterFilter verifier(new HashFilter(*mac, new ArraySink(actualMac, actualMac.size())));
//	MeterFilter verifier(new FileSink("c:\\dt.tmp"));
	std::ifstream moduleStream;

#ifdef CRYPTOPP_WIN32_AVAILABLE
	HMODULE h = NULL;
	{
	const size_t FIPS_MODULE_MAX_PATH = 2*MAX_PATH;
	char moduleFilenameBuf[FIPS_MODULE_MAX_PATH] = "";
	if (moduleFilename == NULL)
	{
#if (_MSC_VER >= 1400 && !defined(_STLPORT_VERSION))	// ifstream doesn't support wide filename on other compilers
		wchar_t wideModuleFilename[FIPS_MODULE_MAX_PATH];
		if (GetModuleFileNameW(s_hModule, wideModuleFilename, FIPS_MODULE_MAX_PATH) > 0)
		{
			moduleStream.open(wideModuleFilename, std::ios::in | std::ios::binary);
			h = GetModuleHandleW(wideModuleFilename);
		}
		else
#endif
		{
			GetModuleFileNameA(s_hModule, moduleFilenameBuf, FIPS_MODULE_MAX_PATH);
			moduleFilename = moduleFilenameBuf;
		}
	}
#endif
	if (moduleFilename != NULL)
	{
			moduleStream.open(moduleFilename, std::ios::in | std::ios::binary);
#ifdef CRYPTOPP_WIN32_AVAILABLE
			h = GetModuleHandleA(moduleFilename);
			moduleFilename = NULL;
	}
#endif
	}

#ifdef CRYPTOPP_WIN32_AVAILABLE
	if (h == g_BaseAddressOfMAC)
	{
		std::ostringstream oss;
		oss << "Crypto++ DLL loaded at base address 0x" << std::hex << h << ".\n";
		OutputDebugString(oss.str().c_str());
	}
	else
	{
		std::ostringstream oss;
		oss << "Crypto++ DLL integrity check may fail. Expected module base address is 0x";
		oss << std::hex << g_BaseAddressOfMAC << ", but module loaded at 0x" << h << ".\n";
		OutputDebugString(oss.str().c_str());
	}
#endif

	if (!moduleStream)
	{
#ifdef CRYPTOPP_WIN32_AVAILABLE
		OutputDebugString("Crypto++ DLL integrity check failed. Cannot open file for reading.");
#endif
		return false;
	}
	FileStore file(moduleStream);

#ifdef CRYPTOPP_WIN32_AVAILABLE
	// try to hash from memory first
	const byte *memBase = (const byte *)h;
	const IMAGE_DOS_HEADER *ph = (IMAGE_DOS_HEADER *)memBase;
	const IMAGE_NT_HEADERS *phnt = (IMAGE_NT_HEADERS *)(memBase + ph->e_lfanew);
	const IMAGE_SECTION_HEADER *phs = IMAGE_FIRST_SECTION(phnt);
	DWORD nSections = phnt->FileHeader.NumberOfSections;
	size_t currentFilePos = 0;

	size_t checksumPos = (byte *)&phnt->OptionalHeader.CheckSum - memBase;
	size_t checksumSize = sizeof(phnt->OptionalHeader.CheckSum);
	size_t certificateTableDirectoryPos = (byte *)&phnt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY] - memBase;
	size_t certificateTableDirectorySize = sizeof(phnt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]);
	size_t certificateTablePos = phnt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress;
	size_t certificateTableSize = phnt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size;

	verifier.AddRangeToSkip(0, checksumPos, checksumSize);
	verifier.AddRangeToSkip(0, certificateTableDirectoryPos, certificateTableDirectorySize);
	verifier.AddRangeToSkip(0, certificateTablePos, certificateTableSize);

	while (nSections--)
	{
		switch (phs->Characteristics)
		{
		default:
			break;
		case IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ:
		case IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ:
			unsigned int sectionSize = STDMIN(phs->SizeOfRawData, phs->Misc.VirtualSize);
			const byte *sectionMemStart = memBase + phs->VirtualAddress;
			unsigned int sectionFileStart = phs->PointerToRawData;
			size_t subSectionStart = 0, nextSubSectionStart;

			do
			{
				const byte *subSectionMemStart = sectionMemStart + subSectionStart;
				size_t subSectionFileStart = sectionFileStart + subSectionStart;
				size_t subSectionSize = sectionSize - subSectionStart;
				nextSubSectionStart = 0;

				unsigned int entriesToReadFromDisk[] = {IMAGE_DIRECTORY_ENTRY_IMPORT, IMAGE_DIRECTORY_ENTRY_IAT};
				for (unsigned int i=0; i<sizeof(entriesToReadFromDisk)/sizeof(entriesToReadFromDisk[0]); i++)
				{
					const IMAGE_DATA_DIRECTORY &entry = phnt->OptionalHeader.DataDirectory[entriesToReadFromDisk[i]];
					const byte *entryMemStart = memBase + entry.VirtualAddress;
					if (subSectionMemStart <= entryMemStart && entryMemStart < subSectionMemStart + subSectionSize)
					{
						subSectionSize = entryMemStart - subSectionMemStart;
						nextSubSectionStart = entryMemStart - sectionMemStart + entry.Size;
					}
				}

#if defined(_MSC_VER) && _MSC_VER >= 1400
				// first byte of _CRT_DEBUGGER_HOOK gets modified in memory by the debugger invisibly, so read it from file
				if (IsDebuggerPresent())
				{
					if (subSectionMemStart <= (byte *)&_CRT_DEBUGGER_HOOK && (byte *)&_CRT_DEBUGGER_HOOK < subSectionMemStart + subSectionSize)
					{
						subSectionSize = (byte *)&_CRT_DEBUGGER_HOOK - subSectionMemStart;
						nextSubSectionStart = (byte *)&_CRT_DEBUGGER_HOOK - sectionMemStart + 1;
					}
				}
#endif

				if (subSectionMemStart <= expectedModuleMac && expectedModuleMac < subSectionMemStart + subSectionSize)
				{
					// found stored MAC
					macFileLocation = (unsigned long)(subSectionFileStart + (expectedModuleMac - subSectionMemStart));
					verifier.AddRangeToSkip(0, macFileLocation, macSize);
				}

				file.TransferTo(verifier, subSectionFileStart - currentFilePos);
				verifier.Put(subSectionMemStart, subSectionSize);
				file.Skip(subSectionSize);
				currentFilePos = subSectionFileStart + subSectionSize;
				subSectionStart = nextSubSectionStart;
			} while (nextSubSectionStart != 0);
		}
		phs++;
	}
#endif
	file.TransferAllTo(verifier);

#ifdef CRYPTOPP_WIN32_AVAILABLE
	// if that fails (could be caused by debug breakpoints or DLL base relocation modifying image in memory),
	// hash from disk instead
	if (!VerifyBufsEqual(expectedModuleMac, actualMac, macSize))
	{
		OutputDebugString("Crypto++ DLL in-memory integrity check failed. This may be caused by debug breakpoints or DLL relocation.\n");
		moduleStream.clear();
		moduleStream.seekg(0);
		verifier.Initialize(MakeParameters(Name::OutputBuffer(), ByteArrayParameter(actualMac, (unsigned int)actualMac.size())));
//		verifier.Initialize(MakeParameters(Name::OutputFileName(), (const char *)"c:\\dt2.tmp"));
		verifier.AddRangeToSkip(0, checksumPos, checksumSize);
		verifier.AddRangeToSkip(0, certificateTableDirectoryPos, certificateTableDirectorySize);
		verifier.AddRangeToSkip(0, certificateTablePos, certificateTableSize);
		verifier.AddRangeToSkip(0, macFileLocation, macSize);
		FileStore(moduleStream).TransferAllTo(verifier);
	}
#endif

	if (VerifyBufsEqual(expectedModuleMac, actualMac, macSize))
		return true;

#ifdef CRYPTOPP_WIN32_AVAILABLE
	std::string hexMac;
	HexEncoder(new StringSink(hexMac)).PutMessageEnd(actualMac, actualMac.size());
	OutputDebugString((("Crypto++ DLL integrity check failed. Actual MAC is: " + hexMac) + ".\n").c_str());
#endif
	return false;
}

void DoPowerUpSelfTest(const char *moduleFilename, const byte *expectedModuleMac)
{
	g_powerUpSelfTestStatus = POWER_UP_SELF_TEST_NOT_DONE;
	SetPowerUpSelfTestInProgressOnThisThread(true);

	try
	{
		if (FIPS_140_2_ComplianceEnabled() || expectedModuleMac != NULL)
		{
			if (!IntegrityCheckModule(moduleFilename, expectedModuleMac, &g_actualMac, &g_macFileLocation))
				throw 0;	// throw here so we break in the debugger, this will be caught right away
		}

		// algorithm tests

		X917RNG_KnownAnswerTest<AES>(
			"2b7e151628aed2a6abf7158809cf4f3c",										// key
			"000102030405060708090a0b0c0d0e0f",										// seed
			"00000000000000000000000000000001",										// time vector
			"D176EDD27493B0395F4D10546232B0693DC7061C03C3A554F09CECF6F6B46D945A");	// output

		SymmetricEncryptionKnownAnswerTest<DES_EDE3>(
			"385D7189A5C3D485E1370AA5D408082B5CCCCB5E19F2D90E",
			"C141B5FCCD28DC8A",
			"6E1BD7C6120947A464A6AAB293A0F89A563D8D40D3461B68",
			"64EAAD4ACBB9CEAD6C7615E7C7E4792FE587D91F20C7D2F4",
			"6235A461AFD312973E3B4F7AA7D23E34E03371F8E8C376C9",
			"E26BA806A59B0330DE40CA38E77A3E494BE2B212F6DD624B",
			"E26BA806A59B03307DE2BCC25A08BA40A8BA335F5D604C62",
			"E26BA806A59B03303C62C2EFF32D3ACDD5D5F35EBCC53371");

		SymmetricEncryptionKnownAnswerTest<SKIPJACK>(
			"1555E5531C3A169B2D65",
			"6EC9795701F49864",
			"00AFA48E9621E52E8CBDA312660184EDDB1F33D9DACDA8DA",
			"DBEC73562EFCAEB56204EB8AE9557EBF77473FBB52D17CD1",
			"0C7B0B74E21F99B8F2C8DF37879F6C044967F42A796DCA8B",
			"79FDDA9724E36CC2E023E9A5C717A8A8A7FDA465CADCBF63",
			"79FDDA9724E36CC26CACBD83C1ABC06EAF5B249BE5B1E040",
			"79FDDA9724E36CC211B0AEC607B95A96BCDA318440B82F49");

		SymmetricEncryptionKnownAnswerTest<AES>(
			"2b7e151628aed2a6abf7158809cf4f3c",
			"000102030405060708090a0b0c0d0e0f",
			"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",	// plaintext
			"3ad77bb40d7a3660a89ecaf32466ef97f5d3d58503b9699de785895a96fdbaaf43b1cd7f598ece23881b00e3ed0306887b0c785e27e8ad3f8223207104725dd4", // ecb
			"7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b273bed6b8e3c1743b7116e69e222295163ff1caa1681fac09120eca307586e1a7",	// cbc
			"3b3fd92eb72dad20333449f8e83cfb4ac8a64537a0b3a93fcde3cdad9f1ce58b26751f67a3cbb140b1808cf187a4f4dfc04b05357c5d1c0eeac4c66f9ff7f2e6", // cfb
			"3b3fd92eb72dad20333449f8e83cfb4a7789508d16918f03f53c52dac54ed8259740051e9c5fecf64344f7a82260edcc304c6528f659c77866a510d9c1d6ae5e", // ofb
			NULL);

		SymmetricEncryptionKnownAnswerTest<AES>(
			"2b7e151628aed2a6abf7158809cf4f3c",
			"f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
			"6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710",
			NULL,
			NULL,
			NULL,
			NULL,
			"874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee"); // ctr


		SecureHashKnownAnswerTest<SHA1>(
			"abc",
			"A9993E364706816ABA3E25717850C26C9CD0D89D");

		SecureHashKnownAnswerTest<SHA224>(
			"abc",
			"23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7");

		SecureHashKnownAnswerTest<SHA256>(
			"abc",
			"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

		SecureHashKnownAnswerTest<SHA384>(
			"abc",
			"cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7");

		SecureHashKnownAnswerTest<SHA512>(
			"abc",
			"ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");

		MAC_KnownAnswerTest<HMAC<SHA1> >(
			"303132333435363738393a3b3c3d3e3f40414243",
			"Sample #2",
			"0922d3405faa3d194f82a45830737d5cc6c75d24");

		const char *keyRSA1 =
			"30820150020100300d06092a864886f70d01010105000482013a3082013602010002400a66791dc6988168de7ab77419bb7fb0"
			"c001c62710270075142942e19a8d8c51d053b3e3782a1de5dc5af4ebe99468170114a1dfe67cdc9a9af55d655620bbab0203010001"
			"02400123c5b61ba36edb1d3679904199a89ea80c09b9122e1400c09adcf7784676d01d23356a7d44d6bd8bd50e94bfc723fa"
			"87d8862b75177691c11d757692df8881022033d48445c859e52340de704bcdda065fbb4058d740bd1d67d29e9c146c11cf61"
			"0220335e8408866b0fd38dc7002d3f972c67389a65d5d8306566d5c4f2a5aa52628b0220045ec90071525325d3d46db79695e9af"
			"acc4523964360e02b119baa366316241022015eb327360c7b60d12e5e2d16bdcd97981d17fba6b70db13b20b436e24eada590220"
			"2ca6366d72781dfa24d34a9a24cbc2ae927a9958af426563ff63fb11658a461d";

		const char *keyRSA2 =
			"30820273020100300D06092A864886F70D01010105000482025D3082025902010002818100D40AF9"
			"A2B713034249E5780056D70FC7DE75D76E44565AA6A6B8ED9646F3C19F9E254D72D7DE6E49DB2264"
			"0C1D05AB9E2A5F901D8F3FE1F7AE02CEE2ECCE54A40ABAE55A004692752E70725AEEE7CDEA67628A"
			"82A9239B4AB660C2BC56D9F01E90CBAAB9BF0FC8E17173CEFC5709A29391A7DDF3E0B758691AAF30"
			"725B292F4F020111027F18C0BA087D082C45D75D3594E0767E4820818EB35612B80CEAB8C880ACA5"
			"44B6876DFFEF85A576C0D45B551AFAA1FD63209CD745DF75C5A0F0B580296EA466CD0338207E4752"
			"FF4E7DB724D8AE18CE5CF4153BB94C27869FBB50E64F02546E4B02997A0B8623E64017CC770759C6"
			"695DB649EEFD829D688D441BCC4E7348F1024100EF86DD7AF3F32CDE8A9F6564E43A559A0C9F8BAD"
			"36CC25330548B347AC158A345631FA90F7B873C36EFFAE2F7823227A3F580B5DD18304D5932751E7"
			"43E9234F024100E2A039854B55688740E32A51DF4AF88613D91A371CF8DDD95D780A89D7CF2119A9"
			"54F1AC0F3DCDB2F6959926E6D9D37D8BC07A4C634DE6F16315BD5F0DAC340102407ECEEDB9903572"
			"1B76909F174BA6698DCA72953D957B22C0A871C8531EDE3A1BB52984A719BC010D1CA57A555DB83F"
			"6DE54CBAB932AEC652F38D497A6F3F30CF024100854F30E4FF232E6DADB2CD99926855F484255AB7"
			"01FBCDCB27EC426F33A7046972AA700ADBCA008763DF87440F52F4E070531AC385B55AAC1C2AE7DD"
			"8F9278F1024100C313F4AF9E4A9DE1253C21080CE524251560C111550772FD08690F13FBE658342E"
			"BD2D41C9DCB12374E871B1839E26CAE252E1AE3DAAD5F1EE1F42B4D0EE7581";

		SignatureKnownAnswerTest<RSASS<PKCS1v15, SHA1> >(
			keyRSA1,
			"Everyone gets Friday off.",
			"0610761F95FFD1B8F29DA34212947EC2AA0E358866A722F03CC3C41487ADC604A48FF54F5C6BEDB9FB7BD59F82D6E55D8F3174BA361B2214B2D74E8825E04E81");

		SignatureKnownAnswerTest<RSASS_ISO<SHA1> >(
			keyRSA2,
			"test",
			"32F6BA41C8930DE71EE67F2627172CC539EDE04267FDE03AC295E3C50311F26C3B275D3AF513AC96"
			"8EE493BAB7DA3A754661D1A7C4A0D1A2B7EE8B313AACD8CB8BFBC5C15EFB0EF15C86A9334A1E87AD"
			"291EB961B5CA0E84930429B28780816AA94F96FC2367B71E2D2E4866FA966795B147F00600E5207E"
			"2F189C883B37477C");

		SignaturePairwiseConsistencyTest<DSA>(
			"3082014A0201003082012B06072A8648CE3804013082011E02818100F468699A6F6EBCC0120D3B34C8E007F125EC7D81F763B8D0F33869AE3BD6B9F2ECCC7DF34DF84C0307449E9B85D30D57194BCCEB310F48141914DD13A077AAF9B624A6CBE666BBA1D7EBEA95B5BA6F54417FD5D4E4220C601E071D316A24EA814E8B0122DBF47EE8AEEFD319EBB01DD95683F10DBB4FEB023F8262A07EAEB7FD02150082AD4E034DA6EEACDFDAE68C36F2BAD614F9E53B02818071AAF73361A26081529F7D84078ADAFCA48E031DB54AD57FB1A833ADBD8672328AABAA0C756247998D7A5B10DACA359D231332CE8120B483A784FE07D46EEBFF0D7D374A10691F78653E6DC29E27CCB1B174923960DFE5B959B919B2C3816C19251832AFD8E35D810E598F82877ABF7D40A041565168BD7F0E21E3FE2A8D8C1C0416021426EBA66E846E755169F84A1DA981D86502405DDF");

		SignaturePairwiseConsistencyTest<ECDSA<EC2N, SHA1> >(
			"302D020100301006072A8648CE3D020106052B8104000404163014020101040F0070337065E1E196980A9D00E37211");

		SignaturePairwiseConsistencyTest<ECDSA<ECP, SHA1> >(
			"3039020100301306072A8648CE3D020106082A8648CE3D030101041F301D02010104182BB8A13C8B867010BD9471D9E81FDB01ABD0538C64D6249A");

		SignaturePairwiseConsistencyTest<RSASS<PSS, SHA1> >(keyRSA1);
	}
	catch (...)
	{
		g_powerUpSelfTestStatus = POWER_UP_SELF_TEST_FAILED;
		goto done;
	}

	g_powerUpSelfTestStatus = POWER_UP_SELF_TEST_PASSED;

done:
	SetPowerUpSelfTestInProgressOnThisThread(false);
	return;
}

#ifdef CRYPTOPP_WIN32_AVAILABLE

void DoDllPowerUpSelfTest()
{
	CryptoPP::DoPowerUpSelfTest(NULL, s_moduleMac);
}

#else

void DoDllPowerUpSelfTest()
{
	throw NotImplemented("DoDllPowerUpSelfTest() only available on Windows");
}

#endif	// #ifdef CRYPTOPP_WIN32_AVAILABLE

NAMESPACE_END

#ifdef CRYPTOPP_WIN32_AVAILABLE

// DllMain needs to be in the global namespace
BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  dwReason,
                      LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CryptoPP::s_hModule = (HMODULE)hModule;
		CryptoPP::DoDllPowerUpSelfTest();
	}
    return TRUE;
}

#endif	// #ifdef CRYPTOPP_WIN32_AVAILABLE

#endif	// #ifndef CRYPTOPP_IMPORTS
