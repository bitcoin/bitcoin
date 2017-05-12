// validat1.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include "cryptlib.h"
#include "pubkey.h"
#include "gfpcrypt.h"
#include "eccrypto.h"
#include "filters.h"
#include "files.h"
#include "hex.h"
#include "base32.h"
#include "base64.h"
#include "modes.h"
#include "cbcmac.h"
#include "dmac.h"
#include "idea.h"
#include "des.h"
#include "rc2.h"
#include "arc4.h"
#include "rc5.h"
#include "blowfish.h"
#include "3way.h"
#include "safer.h"
#include "gost.h"
#include "shark.h"
#include "cast.h"
#include "square.h"
#include "seal.h"
#include "rc6.h"
#include "mars.h"
#include "aes.h"
#include "cpu.h"
#include "rng.h"
#include "rijndael.h"
#include "twofish.h"
#include "serpent.h"
#include "skipjack.h"
#include "shacal2.h"
#include "camellia.h"
#include "osrng.h"
#include "rdrand.h"
#include "zdeflate.h"
#include "smartptr.h"
#include "channels.h"
#include "misc.h"

#include <time.h>
#include <memory>
#include <iostream>
#include <iomanip>

#include "validate.h"

// Aggressive stack checking with VS2005 SP1 and above.
#if (CRYPTOPP_MSC_VERSION >= 1410)
# pragma strict_gs_check (on)
#endif

#if CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

USING_NAMESPACE(CryptoPP)
USING_NAMESPACE(std)

bool ValidateAll(bool thorough)
{
	bool pass=TestSettings();
	pass=TestOS_RNG() && pass;
	pass=TestAutoSeeded() && pass;
	pass=TestAutoSeededX917() && pass;

#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64)
	pass=TestRDRAND() && pass;
	pass=TestRDSEED() && pass;
#endif

#if CRYPTOPP_DEBUG && !defined(CRYPTOPP_IMPORTS)
	// http://github.com/weidai11/cryptopp/issues/92
	pass=TestSecBlock() && pass;
	// http://github.com/weidai11/cryptopp/issues/64
	pass=TestPolynomialMod2() && pass;
	// http://github.com/weidai11/cryptopp/pull/242
	pass=TestHuffmanCodes() && pass;
#endif

	pass=ValidateCRC32() && pass;
	pass=ValidateCRC32C() && pass;
	pass=ValidateAdler32() && pass;
	pass=ValidateMD2() && pass;
	pass=ValidateMD5() && pass;
	pass=ValidateSHA() && pass;

	pass=RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/keccak.txt") && pass;
	pass=RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/sha3_fips_202.txt") && pass;

	pass=ValidateTiger() && pass;
	pass=ValidateRIPEMD() && pass;
	pass=ValidatePanama() && pass;
	pass=ValidateWhirlpool() && pass;
	pass=ValidateBLAKE2s() && pass;
	pass=ValidateBLAKE2b() && pass;

	pass=ValidateHMAC() && pass;
	pass=ValidateTTMAC() && pass;

	pass=ValidatePBKDF() && pass;
	pass=ValidateHKDF() && pass;

	pass=ValidateDES() && pass;
	pass=ValidateCipherModes() && pass;
	pass=ValidateIDEA() && pass;
	pass=ValidateSAFER() && pass;
	pass=ValidateRC2() && pass;
	pass=ValidateARC4() && pass;
	pass=ValidateRC5() && pass;
	pass=ValidateBlowfish() && pass;
	pass=ValidateThreeWay() && pass;
	pass=ValidateGOST() && pass;
	pass=ValidateSHARK() && pass;
	pass=ValidateCAST() && pass;
	pass=ValidateSquare() && pass;
	pass=ValidateSKIPJACK() && pass;
	pass=ValidateSEAL() && pass;
	pass=ValidateRC6() && pass;
	pass=ValidateMARS() && pass;
	pass=ValidateRijndael() && pass;
	pass=ValidateTwofish() && pass;
	pass=ValidateSerpent() && pass;
	pass=ValidateSHACAL2() && pass;
	pass=ValidateCamellia() && pass;
	pass=ValidateSalsa() && pass;
	pass=ValidateSosemanuk() && pass;
	pass=ValidateVMAC() && pass;
	pass=ValidateCCM() && pass;
	pass=ValidateGCM() && pass;
	pass=ValidateCMAC() && pass;
	pass=RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/eax.txt") && pass;
	pass=RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/seed.txt") && pass;

	pass=ValidateBBS() && pass;
	pass=ValidateDH() && pass;
	pass=ValidateMQV() && pass;
	pass=ValidateHMQV() && pass;
	pass=ValidateFHMQV() && pass;
	pass=ValidateRSA() && pass;
	pass=ValidateElGamal() && pass;
	pass=ValidateDLIES() && pass;
	pass=ValidateNR() && pass;
	pass=ValidateDSA(thorough) && pass;
	pass=ValidateLUC() && pass;
	pass=ValidateLUC_DH() && pass;
	pass=ValidateLUC_DL() && pass;
	pass=ValidateXTR_DH() && pass;
	pass=ValidateRabin() && pass;
	pass=ValidateRW() && pass;
//	pass=ValidateBlumGoldwasser() && pass;
	pass=ValidateECP() && pass;
	pass=ValidateEC2N() && pass;
	pass=ValidateECDSA() && pass;
	pass=ValidateESIGN() && pass;

	if (pass)
		cout << "\nAll tests passed!\n";
	else
		cout << "\nOops!  Not all tests passed.\n";

	return pass;
}

bool TestSettings()
{
	// Thanks to IlyaBizyaev and Zireael, http://github.com/weidai11/cryptopp/issues/28
#if defined(__MINGW32__)
	using CryptoPP::memcpy_s;
#endif

	bool pass = true;

	cout << "\nTesting Settings...\n\n";

	word32 w;
	memcpy_s(&w, sizeof(w), "\x01\x02\x03\x04", 4);

	if (w == 0x04030201L)
	{
#ifdef IS_LITTLE_ENDIAN
		cout << "passed:  ";
#else
		cout << "FAILED:  ";
		pass = false;
#endif
		cout << "Your machine is little endian.\n";
	}
	else if (w == 0x01020304L)
	{
#ifndef IS_LITTLE_ENDIAN
		cout << "passed:  ";
#else
		cout << "FAILED:  ";
		pass = false;
#endif
		cout << "Your machine is big endian.\n";
	}
	else
	{
		cout << "FAILED:  Your machine is neither big endian nor little endian.\n";
		pass = false;
	}

#ifdef CRYPTOPP_ALLOW_UNALIGNED_DATA_ACCESS
	// Don't CRYPTOPP_ASSERT the alignment of testvals. That's what this test is for.
	byte testvals[10] = {1,2,2,3,3,3,3,2,2,1};
	if (*(word32 *)(void *)(testvals+3) == 0x03030303 && *(word64 *)(void *)(testvals+1) == W64LIT(0x0202030303030202))
		cout << "passed:  Your machine allows unaligned data access.\n";
	else
	{
		cout << "FAILED:  Unaligned data access gave incorrect results.\n";
		pass = false;
	}
#else
	cout << "passed:  CRYPTOPP_ALLOW_UNALIGNED_DATA_ACCESS is not defined. Will restrict to aligned data access.\n";
#endif

	if (sizeof(byte) == 1)
		cout << "passed:  ";
	else
	{
		cout << "FAILED:  ";
		pass = false;
	}
	cout << "sizeof(byte) == " << sizeof(byte) << endl;

	if (sizeof(word16) == 2)
		cout << "passed:  ";
	else
	{
		cout << "FAILED:  ";
		pass = false;
	}
	cout << "sizeof(word16) == " << sizeof(word16) << endl;

	if (sizeof(word32) == 4)
		cout << "passed:  ";
	else
	{
		cout << "FAILED:  ";
		pass = false;
	}
	cout << "sizeof(word32) == " << sizeof(word32) << endl;

	if (sizeof(word64) == 8)
		cout << "passed:  ";
	else
	{
		cout << "FAILED:  ";
		pass = false;
	}
	cout << "sizeof(word64) == " << sizeof(word64) << endl;

#ifdef CRYPTOPP_WORD128_AVAILABLE
	if (sizeof(word128) == 16)
		cout << "passed:  ";
	else
	{
		cout << "FAILED:  ";
		pass = false;
	}
	cout << "sizeof(word128) == " << sizeof(word128) << endl;
#endif

	if (sizeof(word) == 2*sizeof(hword)
#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
		&& sizeof(dword) == 2*sizeof(word)
#endif
		)
		cout << "passed:  ";
	else
	{
		cout << "FAILED:  ";
		pass = false;
	}
	cout << "sizeof(hword) == " << sizeof(hword) << ", sizeof(word) == " << sizeof(word);
#ifdef CRYPTOPP_NATIVE_DWORD_AVAILABLE
	cout << ", sizeof(dword) == " << sizeof(dword);
#endif
	cout << endl;

#ifdef CRYPTOPP_CPUID_AVAILABLE
	bool hasMMX = HasMMX();
	bool hasISSE = HasISSE();
	bool hasSSE2 = HasSSE2();
	bool hasSSSE3 = HasSSSE3();
	bool hasSSE4 = HasSSE4();
	bool isP4 = IsP4();
	int cacheLineSize = GetCacheLineSize();

	if ((isP4 && (!hasMMX || !hasSSE2)) || (hasSSE2 && !hasMMX) || (cacheLineSize < 16 || cacheLineSize > 256 || !IsPowerOf2(cacheLineSize)))
	{
		cout << "FAILED:  ";
		pass = false;
	}
	else
		cout << "passed:  ";

	cout << "hasMMX == " << hasMMX << ", hasISSE == " << hasISSE << ", hasSSE2 == " << hasSSE2 << ", hasSSSE3 == " << hasSSSE3 << ", hasSSE4 == " << hasSSE4;
	cout << ", hasAESNI == " << HasAESNI() << ", hasCLMUL == " << HasCLMUL() << ", hasRDRAND == " << HasRDRAND() << ", hasRDSEED == " << HasRDSEED();
	cout << ", isP4 == " << isP4 << ", cacheLineSize == " << cacheLineSize << endl;

#elif (CRYPTOPP_BOOL_ARM32 || CRYPTOPP_BOOL_ARM64)
	bool hasNEON = HasNEON();
	bool hasPMULL = HasPMULL();
	bool hasCRC32 = HasCRC32();
	bool hasAES = HasAES();
	bool hasSHA1 = HasSHA1();
	bool hasSHA2 = HasSHA2();

	cout << "passed:  ";
	cout << "hasNEON == " << hasNEON << ", hasPMULL == " << hasPMULL << ", hasCRC32 == " << hasCRC32 << ", hasAES == " << hasAES << ", hasSHA1 == " << hasSHA1 << ", hasSHA2 == " << hasSHA2 << endl;
#endif

	if (!pass)
	{
		cout << "Some critical setting in config.h is in error.  Please fix it and recompile." << endl;
		abort();
	}
	return pass;
}

#if CRYPTOPP_DEBUG && !defined(CRYPTOPP_IMPORTS)
bool TestSecBlock()
{
	cout << "\nTesting SecBlock...\n\n";

	bool pass1=true, pass2=true, pass3=true, pass4=true, pass5=true, pass6=true, pass7=true, temp=false;

	//********** Zeroized block **********//

	{
		// NULL ptr with a size means to create a new SecBlock with all elements zero'd
		SecByteBlock z1(NULL, 256);
		temp = true;

		for (size_t i = 0; i < z1.size(); i++)
			temp &= (z1[i] == 0);

		pass1 &= temp;
		if (!temp)
			cout << "FAILED:";
		else
			cout << "passed:";
		cout << "  Zeroized byte array" << endl;

		SecBlock<word32> z2(NULL, 256);
		temp = true;

		for (size_t i = 0; i < z2.size(); i++)
			temp &= (z2[i] == 0);

		pass1 &= temp;
		if (!temp)
			cout << "FAILED:";
		else
			cout << "passed:";
		cout << "  Zeroized word32 array" << endl;

		SecBlock<word64> z3(NULL, 256);
		temp = true;

		for (size_t i = 0; i < z3.size(); i++)
			temp &= (z3[i] == 0);

		pass1 &= temp;
		if (!temp)
			cout << "FAILED:";
		else
			cout << "passed:";
		cout << "  Zeroized word64 array" << endl;

#if defined(CRYPTOPP_WORD128_AVAILABLE)
		SecBlock<word128> z4(NULL, 256);
		temp = true;

		for (size_t i = 0; i < z4.size(); i++)
			temp &= (z4[i] == 0);

		pass1 &= temp;
		if (!temp)
			cout << "FAILED:";
		else
			cout << "passed:";
		cout << "  Zeroized word128 array" << endl;
#endif
	}

	//********** Assign **********//

	try
	{
		SecByteBlock a, b;
		temp = true;

		a.Assign((const byte*)"a", 1);
		b.Assign((const byte*)"b", 1);

		temp &= (a.SizeInBytes() == 1);
		temp &= (b.SizeInBytes() == 1);
		temp &= (a[0] == 'a');
		temp &= (b[0] == 'b');

		a.Assign((const byte*)"ab", 2);
		b.Assign((const byte*)"cd", 2);

		temp &= (a.SizeInBytes() == 2);
		temp &= (b.SizeInBytes() == 2);
		temp &= (a[0] == 'a' && a[1] == 'b');
		temp &= (b[0] == 'c' && b[1] == 'd');
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass2 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Assign byte" << endl;

	try
	{
		SecBlock<word32> a, b;
		temp = true;

		word32 one[1] = {1}, two[1] = {2};
		a.Assign(one, 1);
		b.Assign(two, 1);

		temp &= (a.SizeInBytes() == 4);
		temp &= (b.SizeInBytes() == 4);
		temp &= (a[0] == 1);
		temp &= (b[0] == 2);

		word32 three[2] = {1,2}, four[2] = {3,4};
		a.Assign(three, 2);
		b.Assign(four, 2);

		temp &= (a.SizeInBytes() == 8);
		temp &= (b.SizeInBytes() == 8);
		temp &= (a[0] == 1 && a[1] == 2);
		temp &= (b[0] == 3 && b[1] == 4);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass2 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Assign word32" << endl;

	try
	{
		SecBlock<word64> a, b;
		temp = true;

		word64 one[1] = {1}, two[1] = {2};
		a.Assign(one, 1);
		b.Assign(two, 1);

		temp &= (a.SizeInBytes() == 8);
		temp &= (b.SizeInBytes() == 8);
		temp &= (a[0] == 1);
		temp &= (b[0] == 2);

		word64 three[2] = {1,2}, four[2] = {3,4};
		a.Assign(three, 2);
		b.Assign(four, 2);

		temp &= (a.SizeInBytes() == 16);
		temp &= (b.SizeInBytes() == 16);
		temp &= (a[0] == 1 && a[1] == 2);
		temp &= (b[0] == 3 && b[1] == 4);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass2 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Assign word64" << endl;

#if defined(CRYPTOPP_WORD128_AVAILABLE)
	try
	{
		SecBlock<word128> a, b;
		temp = true;

		word128 one[1] = {1}, two[1] = {2};
		a.Assign(one, 1);
		b.Assign(two, 1);

		temp &= (a.SizeInBytes() == 16);
		temp &= (b.SizeInBytes() == 16);
		temp &= (a[0] == 1);
		temp &= (b[0] == 2);

		word128 three[2] = {1,2}, four[2] = {3,4};
		a.Assign(three, 2);
		b.Assign(four, 2);

		temp &= (a.SizeInBytes() == 32);
		temp &= (b.SizeInBytes() == 32);
		temp &= (a[0] == 1 && a[1] == 2);
		temp &= (b[0] == 3 && b[1] == 4);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass2 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Assign word128" << endl;
#endif

	//********** Append **********//

	try
	{
		SecByteBlock a, b;
		temp = true;

		a.Assign((const byte*)"a", 1);
		b.Assign((const byte*)"b", 1);

		a += b;
		temp &= (a.SizeInBytes() == 2);
		temp &= (a[0] == 'a' && a[1] == 'b');

		a.Assign((const byte*)"ab", 2);
		b.Assign((const byte*)"cd", 2);

		a += b;
		temp &= (a.SizeInBytes() == 4);
		temp &= (a[0] == 'a' && a[1] == 'b' && a[2] == 'c' && a[3] == 'd');

		a.Assign((const byte*)"a", 1);

		a += a;
		temp &= (a.SizeInBytes() == 2);
		temp &= (a[0] == 'a' && a[1] == 'a');

		a.Assign((const byte*)"ab", 2);

		a += a;
		temp &= (a.SizeInBytes() == 4);
		temp &= (a[0] == 'a' && a[1] == 'b' && a[2] == 'a' && a[3] == 'b');
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass3 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Append byte" << endl;

	try
	{
		SecBlock<word32> a, b;
		temp = true;

		const word32 one[1] = {1}, two[1] = {2};
		a.Assign(one, 1);
		b.Assign(two, 1);

		a += b;
		temp &= (a.SizeInBytes() == 8);
		temp &= (a[0] == 1 && a[1] == 2);

		const word32 three[2] = {1,2}, four[2] = {3,4};
		a.Assign(three, 2);
		b.Assign(four, 2);

		a += b;
		temp &= (a.SizeInBytes() == 16);
		temp &= (a[0] == 1 && a[1] == 2 && a[2] == 3 && a[3] == 4);

		a.Assign(one, 1);

		a += a;
		temp &= (a.SizeInBytes() == 8);
		temp &= (a[0] == 1 && a[1] == 1);

		a.Assign(three, 2);

		a += a;
		temp &= (a.SizeInBytes() == 16);
		temp &= (a[0] == 1 && a[1] == 2 && a[2] == 1 && a[3] == 2);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass3 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Append word32" << endl;

	try
	{
		SecBlock<word64> a, b;
		temp = true;

		const word64 one[1] = {1}, two[1] = {2};
		a.Assign(one, 1);
		b.Assign(two, 1);

		a += b;
		temp &= (a.SizeInBytes() == 16);
		temp &= (a[0] == 1 && a[1] == 2);

		const word64 three[2] = {1,2}, four[2] = {3,4};
		a.Assign(three, 2);
		b.Assign(four, 2);

		a += b;
		temp &= (a.SizeInBytes() == 32);
		temp &= (a[0] == 1 && a[1] == 2 && a[2] == 3 && a[3] == 4);

		a.Assign(one, 1);

		a += a;
		temp &= (a.SizeInBytes() == 16);
		temp &= (a[0] == 1 && a[1] == 1);

		a.Assign(three, 2);

		a += a;
		temp &= (a.SizeInBytes() == 32);
		temp &= (a[0] == 1 && a[1] == 2 && a[2] == 1 && a[3] == 2);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass3 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Append word64" << endl;

#if defined(CRYPTOPP_WORD128_AVAILABLE)
	try
	{
		SecBlock<word128> a, b;
		temp = true;

		const word128 one[1] = {1}, two[1] = {2};
		a.Assign(one, 1);
		b.Assign(two, 1);

		a += b;
		temp &= (a.SizeInBytes() == 32);
		temp &= (a[0] == 1 && a[1] == 2);

		const word128 three[2] = {1,2}, four[2] = {3,4};
		a.Assign(three, 2);
		b.Assign(four, 2);

		a += b;
		temp &= (a.SizeInBytes() == 64);
		temp &= (a[0] == 1 && a[1] == 2 && a[2] == 3 && a[3] == 4);

		a.Assign(one, 1);

		a += a;
		temp &= (a.SizeInBytes() == 32);
		temp &= (a[0] == 1 && a[1] == 1);

		a.Assign(three, 2);

		a += a;
		temp &= (a.SizeInBytes() == 64);
		temp &= (a[0] == 1 && a[1] == 2 && a[2] == 1 && a[3] == 2);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass3 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Append word128" << endl;
#endif

	//********** Concatenate **********//

	// byte
	try
	{
		SecByteBlock a, b, c;
		temp = true;

		a.Assign((const byte*)"a", 1);
		b.Assign((const byte*)"b", 1);

		c = a + b;
		temp &= (a[0] == 'a');
		temp &= (b[0] == 'b');
		temp &= (c.SizeInBytes() == 2);
		temp &= (c[0] == 'a' && c[1] == 'b');

		a.Assign((const byte*)"ab", 2);
		b.Assign((const byte*)"cd", 2);

		c = a + b;
		temp &= (a[0] == 'a' && a[1] == 'b');
		temp &= (b[0] == 'c' && b[1] == 'd');
		temp &= (c.SizeInBytes() == 4);
		temp &= (c[0] == 'a' && c[1] == 'b' && c[2] == 'c' && c[3] == 'd');
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass4 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Concatenate byte" << endl;

	// word32
	try
	{
		SecBlock<word32> a, b, c;
		temp = true;

		const word32 one[1] = {1}, two[1] = {2};
		a.Assign(one, 1);
		b.Assign(two, 1);

		c = a + b;
		temp &= (a[0] == 1);
		temp &= (b[0] == 2);
		temp &= (c.SizeInBytes() == 8);
		temp &= (c[0] == 1 && c[1] == 2);

		const word32 three[2] = {1,2}, four[2] = {3,4};
		a.Assign(three, 2);
		b.Assign(four, 2);

		c = a + b;
		temp &= (a[0] == 1 && a[1] == 2);
		temp &= (b[0] == 3 && b[1] == 4);
		temp &= (c.SizeInBytes() == 16);
		temp &= (c[0] == 1 && c[1] == 2 && c[2] == 3 && c[3] == 4);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass4 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Concatenate word32" << endl;

	// word64
	try
	{
		SecBlock<word64> a, b, c;
		temp = true;

		const word64 one[1] = {1}, two[1] = {2};
		a.Assign(one, 1);
		b.Assign(two, 1);

		c = a + b;
		temp &= (a[0] == 1);
		temp &= (b[0] == 2);
		temp &= (c.SizeInBytes() == 16);
		temp &= (c[0] == 1 && c[1] == 2);

		const word64 three[2] = {1,2}, four[2] = {3,4};
		a.Assign(three, 2);
		b.Assign(four, 2);

		c = a + b;
		temp &= (a[0] == 1 && a[1] == 2);
		temp &= (b[0] == 3 && b[1] == 4);
		temp &= (c.SizeInBytes() == 32);
		temp &= (c[0] == 1 && c[1] == 2 && c[2] == 3 && c[3] == 4);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass4 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Concatenate word64" << endl;

#if defined(CRYPTOPP_WORD128_AVAILABLE)
	try
	{
		SecBlock<word128> a, b, c;
		temp = true;

		const word128 one[1] = {1}, two[1] = {2};
		a.Assign(one, 1);
		b.Assign(two, 1);

		c = a + b;
		temp &= (a[0] == 1);
		temp &= (b[0] == 2);
		temp &= (c.SizeInBytes() == 32);
		temp &= (c[0] == 1 && c[1] == 2);

		const word128 three[2] = {1,2}, four[2] = {3,4};
		a.Assign(three, 2);
		b.Assign(four, 2);

		c = a + b;
		temp &= (a[0] == 1 && a[1] == 2);
		temp &= (b[0] == 3 && b[1] == 4);
		temp &= (c.SizeInBytes() == 64);
		temp &= (c[0] == 1 && c[1] == 2 && c[2] == 3 && c[3] == 4);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass4 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Concatenate word128" << endl;
#endif

	//********** Equality **********//

	// byte
	try
	{
		static const byte str1[] = "abcdefghijklmnopqrstuvwxyz";
		static const byte str2[] = "zyxwvutsrqponmlkjihgfedcba";
		static const byte str3[] = "0123456789";

		temp = true;
		SecByteBlock a,b;

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str1, COUNTOF(str1));
		temp &= (a.operator==(b));

		a.Assign(str3, COUNTOF(str3));
		b.Assign(str3, COUNTOF(str3));
		temp &= (a == b);

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str2, COUNTOF(str2));
		temp &= (a.operator!=(b));

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str3, COUNTOF(str3));
		temp &= (a != b);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass5 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Equality byte" << endl;

	// word32
	try
	{
		static const word32 str1[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97};
		static const word32 str2[] = {97,89,83,79,73,71,67,61,59,53,47,43,41,37,31,29,23,19,17,13,11,7,5,3,2};
		static const word32 str3[] = {0,1,2,3,4,5,6,7,8,9};

		temp = true;
		SecBlock<word32> a,b;

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str1, COUNTOF(str1));
		temp &= (a.operator==(b));

		a.Assign(str3, COUNTOF(str3));
		b.Assign(str3, COUNTOF(str3));
		temp &= (a == b);

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str2, COUNTOF(str2));
		temp &= (a.operator!=(b));

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str3, COUNTOF(str3));
		temp &= (a != b);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass5 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Equality word32" << endl;

	// word64
	try
	{
		static const word64 str1[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97};
		static const word64 str2[] = {97,89,83,79,73,71,67,61,59,53,47,43,41,37,31,29,23,19,17,13,11,7,5,3,2};
		static const word64 str3[] = {0,1,2,3,4,5,6,7,8,9};

		temp = true;
		SecBlock<word64> a,b;

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str1, COUNTOF(str1));
		temp &= (a.operator==(b));

		a.Assign(str3, COUNTOF(str3));
		b.Assign(str3, COUNTOF(str3));
		temp &= (a == b);

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str2, COUNTOF(str2));
		temp &= (a.operator!=(b));

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str3, COUNTOF(str3));
		temp &= (a != b);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass5 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Equality word64" << endl;

#if defined(CRYPTOPP_WORD128_AVAILABLE)
	// word128
	try
	{
		static const word128 str1[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97};
		static const word128 str2[] = {97,89,83,79,73,71,67,61,59,53,47,43,41,37,31,29,23,19,17,13,11,7,5,3,2};
		static const word128 str3[] = {0,1,2,3,4,5,6,7,8,9};

		temp = true;
		SecBlock<word128> a,b;

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str1, COUNTOF(str1));
		temp &= (a.operator==(b));

		a.Assign(str3, COUNTOF(str3));
		b.Assign(str3, COUNTOF(str3));
		temp &= (a == b);

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str2, COUNTOF(str2));
		temp &= (a.operator!=(b));

		a.Assign(str1, COUNTOF(str1));
		b.Assign(str3, COUNTOF(str3));
		temp &= (a != b);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}

	pass5 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Equality word128" << endl;
#endif

	//********** Allocator Size/Overflow **********//

	try
	{
		temp = false;

		AllocatorBase<word32> A;
		const size_t max = A.max_size();
		SecBlock<word32> t(max+1);
	}
	catch(const Exception& /*ex*/)
	{
		temp = true;
	}
	catch(const std::exception& /*ex*/)
	{
		temp = true;
	}

	pass6 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Overflow word32" << endl;

	try
	{
		temp = false;

		AllocatorBase<word64> A;
		const size_t max = A.max_size();
		SecBlock<word64> t(max+1);
	}
	catch(const Exception& /*ex*/)
	{
		temp = true;
	}
	catch(const std::exception& /*ex*/)
	{
		temp = true;
	}

	pass6 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Overflow word64" << endl;

#if defined(CRYPTOPP_WORD128_AVAILABLE)
	try
	{
		temp = false;

		AllocatorBase<word128> A;
		const size_t max = A.max_size();
		SecBlock<word128> t(max+1);
	}
	catch(const Exception& /*ex*/)
	{
		temp = true;
	}
	catch(const std::exception& /*ex*/)
	{
		temp = true;
	}

	pass6 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  Overflow word128" << endl;
#endif

	//********** FixedSizeAllocatorWithCleanup and Grow **********//

	// byte
	try
	{
		static const unsigned int SIZE = 8;
		SecBlockWithHint<byte, SIZE> block(SIZE);
		memset(block, 0xaa, block.SizeInBytes());

		temp = true;
		block.CleanGrow(SIZE*2);
		temp &= (block.size() == SIZE*2);

		for (size_t i = 0; i < block.size()/2; i++)
			temp &= (block[i] == 0xaa);
		for (size_t i = block.size()/2; i < block.size(); i++)
			temp &= (block[i] == 0);

		block.CleanNew(SIZE*4);
		temp &= (block.size() == SIZE*4);
		for (size_t i = 0; i < block.size(); i++)
			temp &= (block[i] == 0);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}
	catch(const std::exception& /*ex*/)
	{
		temp = false;
	}

	pass7 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  FixedSizeAllocator Grow with byte" << endl;

	// word32
	try
	{
		static const unsigned int SIZE = 8;
		SecBlockWithHint<word32, SIZE> block(SIZE);
		memset(block, 0xaa, block.SizeInBytes());

		temp = true;
		block.CleanGrow(SIZE*2);
		temp &= (block.size() == SIZE*2);

		for (size_t i = 0; i < block.size()/2; i++)
			temp &= (block[i] == 0xaaaaaaaa);

		for (size_t i = block.size()/2; i < block.size(); i++)
			temp &= (block[i] == 0);

		block.CleanNew(SIZE*4);
		temp &= (block.size() == SIZE*4);
		for (size_t i = 0; i < block.size(); i++)
			temp &= (block[i] == 0);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}
	catch(const std::exception& /*ex*/)
	{
		temp = false;
	}

	pass7 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  FixedSizeAllocator Grow with word32" << endl;

	// word64
	try
	{
		static const unsigned int SIZE = 8;
		SecBlockWithHint<word64, SIZE> block(SIZE);
		memset(block, 0xaa, block.SizeInBytes());

		temp = true;
		block.CleanGrow(SIZE*2);
		temp &= (block.size() == SIZE*2);

		for (size_t i = 0; i < block.size()/2; i++)
			temp &= (block[i] == W64LIT(0xaaaaaaaaaaaaaaaa));

		for (size_t i = block.size()/2; i < block.size(); i++)
			temp &= (block[i] == 0);

		block.CleanNew(SIZE*4);
		temp &= (block.size() == SIZE*4);
		for (size_t i = 0; i < block.size(); i++)
			temp &= (block[i] == 0);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}
	catch(const std::exception& /*ex*/)
	{
		temp = false;
	}

	pass7 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  FixedSizeAllocator Grow with word64" << endl;

#if defined(CRYPTOPP_WORD128_AVAILABLE)
	// word128
	try
	{
		static const unsigned int SIZE = 8;
		SecBlock<word128, AllocatorWithCleanup<word128, true> > block(SIZE);
		memset(block, 0xaa, block.SizeInBytes());

		temp = true;
		block.CleanGrow(SIZE*2);
		temp &= (block.size() == SIZE*2);

		for (size_t i = 0; i < block.size()/2; i++)
			temp &= (block[i] == (((word128)W64LIT(0xaaaaaaaaaaaaaaaa) << 64U) | W64LIT(0xaaaaaaaaaaaaaaaa)));

		for (size_t i = block.size()/2; i < block.size(); i++)
			temp &= (block[i] == 0);

		block.CleanNew(SIZE*4);
		temp &= (block.size() == SIZE*4);
		for (size_t i = 0; i < block.size(); i++)
			temp &= (block[i] == 0);
	}
	catch(const Exception& /*ex*/)
	{
		temp = false;
	}
	catch(const std::exception& /*ex*/)
	{
		temp = false;
	}

	pass7 &= temp;
	if (!temp)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  FixedSizeAllocator Grow with word128" << endl;
#endif

	return pass1 && pass2 && pass3 && pass4 && pass5 && pass6 && pass7;
}
#endif

#if CRYPTOPP_DEBUG && !defined(CRYPTOPP_IMPORTS)
bool TestHuffmanCodes()
{
    cout << "\nTesting Huffman codes...\n\n";
	bool pass=true;

    static const size_t nCodes = 30;
    const unsigned int codeCounts[nCodes] = {
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    static const unsigned int maxCodeBits = nCodes >> 1;
    unsigned int codeBits[nCodes] = {
        ~0u, ~0u, ~0u, ~0u, ~0u,
        ~0u, ~0u, ~0u, ~0u, ~0u,
        ~0u, ~0u, ~0u, ~0u, ~0u,
    };

    try
    {
        HuffmanEncoder::GenerateCodeLengths(codeBits, maxCodeBits, codeCounts, nCodes);
    }
    catch(const Exception& /*ex*/)
    {
        pass=false;
    }

	if (!pass)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  GenerateCodeLengths" << endl;

    return pass;
}
#endif

bool TestOS_RNG()
{
	bool pass = true;

	member_ptr<RandomNumberGenerator> rng;

#ifdef BLOCKING_RNG_AVAILABLE
	try {rng.reset(new BlockingRng);}
	catch (OS_RNG_Err &) {}
#endif

	if (rng.get())
	{
		cout << "\nTesting operating system provided blocking random number generator...\n\n";

		MeterFilter meter(new Redirector(TheBitBucket()));
		RandomNumberSource test(*rng, UINT_MAX, false, new Deflator(new Redirector(meter)));
		unsigned long total=0, length=0;
		time_t t = time(NULL), t1 = 0;
		CRYPTOPP_UNUSED(length);

		// check that it doesn't take too long to generate a reasonable amount of randomness
		while (total < 16 && (t1 < 10 || total*8 > (unsigned long)t1))
		{
			test.Pump(1);
			total += 1;
			t1 = time(NULL) - t;
		}

		if (total < 16)
		{
			cout << "FAILED:";
			pass = false;
		}
		else
			cout << "passed:";
		cout << "  it took " << long(t1) << " seconds to generate " << total << " bytes" << endl;

#if 0	// disable this part. it's causing an unpredictable pause during the validation testing
		if (t1 < 2)
		{
			// that was fast, are we really blocking?
			// first exhaust the extropy reserve
			t = time(NULL);
			while (time(NULL) - t < 2)
			{
				test.Pump(1);
				total += 1;
			}

			// if it generates too many bytes in a certain amount of time,
			// something's probably wrong
			t = time(NULL);
			while (time(NULL) - t < 2)
			{
				test.Pump(1);
				total += 1;
				length += 1;
			}
			if (length > 1024)
			{
				cout << "FAILED:";
				pass = false;
			}
			else
				cout << "passed:";
			cout << "  it generated " << length << " bytes in " << long(time(NULL) - t) << " seconds" << endl;
		}
#endif

		test.AttachedTransformation()->MessageEnd();

		if (meter.GetTotalBytes() < total)
		{
			cout << "FAILED:";
			pass = false;
		}
		else
			cout << "passed:";
		cout << "  " << total << " generated bytes compressed to " << meter.GetTotalBytes() << " bytes by DEFLATE" << endl;
	}
	else
		cout << "\nNo operating system provided blocking random number generator, skipping test." << endl;

	rng.reset(NULL);
#ifdef NONBLOCKING_RNG_AVAILABLE
	try {rng.reset(new NonblockingRng);}
	catch (OS_RNG_Err &) {}
#endif

	if (rng.get())
	{
		cout << "\nTesting operating system provided nonblocking random number generator...\n\n";

		MeterFilter meter(new Redirector(TheBitBucket()));
		RandomNumberSource test(*rng, 100000, true, new Deflator(new Redirector(meter)));

		if (meter.GetTotalBytes() < 100000)
		{
			cout << "FAILED:";
			pass = false;
		}
		else
			cout << "passed:";
		cout << "  100000 generated bytes compressed to " << meter.GetTotalBytes() << " bytes by DEFLATE" << endl;
	}
	else
		cout << "\nNo operating system provided nonblocking random number generator, skipping test." << endl;

	return pass;
}

#if defined(NO_OS_DEPENDENCE) || !defined(OS_RNG_AVAILABLE)
bool TestAutoSeeded()
{
	return true;
}
bool TestAutoSeededX917()
{
	return true;
}
#else
bool TestAutoSeeded()
{
	// This tests Auto-Seeding and GenerateIntoBufferedTransformation.
	cout << "\nTesting AutoSeeded generator...\n\n";

	AutoSeededRandomPool prng;
	static const unsigned int ENTROPY_SIZE = 32;
	bool generate = true, discard = true, incorporate = false;

	MeterFilter meter(new Redirector(TheBitBucket()));
	RandomNumberSource test(prng, 100000, true, new Deflator(new Redirector(meter)));

	if (meter.GetTotalBytes() < 100000)
	{
		cout << "FAILED:";
		generate = false;
	}
	else
		cout << "passed:";
	cout << "  100000 generated bytes compressed to " << meter.GetTotalBytes() << " bytes by DEFLATE" << endl;

	try
	{
		prng.DiscardBytes(100000);
	}
	catch(const Exception&)
	{
		discard = false;
	}

	if (!discard)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  discarded 10000 bytes" << endl;

	try
	{
		if(prng.CanIncorporateEntropy())
		{
			SecByteBlock entropy(ENTROPY_SIZE);
			OS_GenerateRandomBlock(false, entropy, entropy.SizeInBytes());

			prng.IncorporateEntropy(entropy, entropy.SizeInBytes());
			prng.IncorporateEntropy(entropy, entropy.SizeInBytes());
			prng.IncorporateEntropy(entropy, entropy.SizeInBytes());
			prng.IncorporateEntropy(entropy, entropy.SizeInBytes());

			incorporate = true;
		}
	}
	catch(const Exception& /*ex*/)
	{
	}

	if (!incorporate)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  IncorporateEntropy with " << 4*ENTROPY_SIZE << " bytes" << endl;

	return generate && discard && incorporate;
}

bool TestAutoSeededX917()
{
	// This tests Auto-Seeding and GenerateIntoBufferedTransformation.
	cout << "\nTesting AutoSeeded X917 generator...\n\n";

	AutoSeededX917RNG<AES> prng;
	static const unsigned int ENTROPY_SIZE = 32;
	bool generate = true, discard = true, incorporate = false;

	MeterFilter meter(new Redirector(TheBitBucket()));
	RandomNumberSource test(prng, 100000, true, new Deflator(new Redirector(meter)));

	if (meter.GetTotalBytes() < 100000)
	{
		cout << "FAILED:";
		generate = false;
	}
	else
		cout << "passed:";
	cout << "  100000 generated bytes compressed to " << meter.GetTotalBytes() << " bytes by DEFLATE" << endl;

	try
	{
		prng.DiscardBytes(100000);
	}
	catch(const Exception&)
	{
		discard = false;
	}

	if (!discard)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  discarded 10000 bytes" << endl;

	try
	{
		if(prng.CanIncorporateEntropy())
		{
			SecByteBlock entropy(ENTROPY_SIZE);
			OS_GenerateRandomBlock(false, entropy, entropy.SizeInBytes());

			prng.IncorporateEntropy(entropy, entropy.SizeInBytes());
			prng.IncorporateEntropy(entropy, entropy.SizeInBytes());
			prng.IncorporateEntropy(entropy, entropy.SizeInBytes());
			prng.IncorporateEntropy(entropy, entropy.SizeInBytes());

			incorporate = true;
		}
	}
	catch(const Exception& /*ex*/)
	{
	}

	if (!incorporate)
		cout << "FAILED:";
	else
		cout << "passed:";
	cout << "  IncorporateEntropy with " << 4*ENTROPY_SIZE << " bytes" << endl;

	return generate && discard && incorporate;
}
#endif // NO_OS_DEPENDENCE

#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64)
bool TestRDRAND()
{
	// Testing on 6th generation i7 shows RDRAND needs less than 8 retries for 10K bytes.
	RDRAND rdrand;
	bool entropy = true, compress = true, discard = true;
	static const unsigned int SIZE = 10000;

	if (HasRDRAND())
	{
		cout << "\nTesting RDRAND generator...\n\n";

		MeterFilter meter(new Redirector(TheBitBucket()));
		Deflator deflator(new Redirector(meter));
		MaurerRandomnessTest maurer;

		ChannelSwitch chsw;
		chsw.AddDefaultRoute(deflator);
		chsw.AddDefaultRoute(maurer);

		RandomNumberSource rns(rdrand, SIZE, true, new Redirector(chsw));
		deflator.Flush(true);

		CRYPTOPP_ASSERT(0 == maurer.BytesNeeded());
		const double mv = maurer.GetTestValue();
		if (mv < 0.98f)
		{
			cout << "FAILED:";
			entropy = false;
		}
		else
			cout << "passed:";

		// Coverity finding, also see http://stackoverflow.com/a/34509163/608639.
		StreamState ss(cout);
		cout << std::setiosflags(std::ios::fixed) << std::setprecision(6);
		cout << "  Maurer Randomness Test returned value " << mv << endl;

		if (meter.GetTotalBytes() < SIZE)
		{
			cout << "FAILED:";
			compress = false;
		}
		else
			cout << "passed:";
		cout << "  " << SIZE << " generated bytes compressed to " << meter.GetTotalBytes() << " bytes by DEFLATE\n";

		try
		{
			rdrand.DiscardBytes(SIZE);
		}
		catch(const Exception&)
		{
			discard = false;
		}

		if (!discard)
			cout << "FAILED:";
		else
			cout << "passed:";
		cout << "  discarded " << SIZE << " bytes\n";
	}
	else
		cout << "\nRDRAND generator not available, skipping test.\n";

	// Squash code coverage warnings on unused functions
	(void)rdrand.AlgorithmName();
	(void)rdrand.CanIncorporateEntropy();
	rdrand.SetRetries(rdrand.GetRetries());
	rdrand.IncorporateEntropy(NULL, 0);

	if (!(entropy && compress && discard))
		cout.flush();

	return entropy && compress && discard;
}
#endif

#if (CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || CRYPTOPP_BOOL_X64)
bool TestRDSEED()
{
	// Testing on 5th generation i5 shows RDSEED needs about 128 retries for 10K bytes
	//  on 64-bit/amd64 VM, and it needs more for an 32-bit/i686 VM.
	RDSEED rdseed(256);
	bool entropy = true, compress = true, discard = true;
	static const unsigned int SIZE = 10000;

	if (HasRDSEED())
	{
		cout << "\nTesting RDSEED generator...\n\n";

		MeterFilter meter(new Redirector(TheBitBucket()));
		Deflator deflator(new Redirector(meter));
		MaurerRandomnessTest maurer;

		ChannelSwitch chsw;
		chsw.AddDefaultRoute(deflator);
		chsw.AddDefaultRoute(maurer);

		RandomNumberSource rns(rdseed, SIZE, true, new Redirector(chsw));
		deflator.Flush(true);

		CRYPTOPP_ASSERT(0 == maurer.BytesNeeded());
		const double mv = maurer.GetTestValue();
		if (mv < 0.98f)
		{
			cout << "FAILED:";
			entropy = false;
		}
		else
			cout << "passed:";

		// Coverity finding, also see http://stackoverflow.com/a/34509163/608639.
		StreamState ss(cout);
		cout << std::setiosflags(std::ios::fixed) << std::setprecision(6);
		cout << "  Maurer Randomness Test returned value " << mv << endl;

		if (meter.GetTotalBytes() < SIZE)
		{
			cout << "FAILED:";
			compress = false;
		}
		else
			cout << "passed:";
		cout << "  " << SIZE << " generated bytes compressed to " << meter.GetTotalBytes() << " bytes by DEFLATE\n";

		try
		{
			rdseed.DiscardBytes(SIZE);
		}
		catch(const Exception&)
		{
			discard = false;
		}

		if (!discard)
			cout << "FAILED:";
		else
			cout << "passed:";
		cout << "  discarded " << SIZE << " bytes\n";
	}
	else
		cout << "\nRDSEED generator not available, skipping test.\n";

	// Squash code coverage warnings on unused functions
	(void)rdseed.AlgorithmName();
	(void)rdseed.CanIncorporateEntropy();
	rdseed.SetRetries(rdseed.GetRetries());
	rdseed.IncorporateEntropy(NULL, 0);

	if (!(entropy && compress && discard))
		cout.flush();

	return entropy && compress && discard;
}
#endif

// VC50 workaround
typedef auto_ptr<BlockTransformation> apbt;

class CipherFactory
{
public:
	virtual unsigned int BlockSize() const =0;
	virtual unsigned int KeyLength() const =0;

	virtual apbt NewEncryption(const byte *keyStr) const =0;
	virtual apbt NewDecryption(const byte *keyStr) const =0;
};

template <class E, class D> class FixedRoundsCipherFactory : public CipherFactory
{
public:
	FixedRoundsCipherFactory(unsigned int keylen=0) : m_keylen(keylen?keylen:E::DEFAULT_KEYLENGTH) {}
	unsigned int BlockSize() const {return E::BLOCKSIZE;}
	unsigned int KeyLength() const {return m_keylen;}

	apbt NewEncryption(const byte *keyStr) const
		{return apbt(new E(keyStr, m_keylen));}
	apbt NewDecryption(const byte *keyStr) const
		{return apbt(new D(keyStr, m_keylen));}

	unsigned int m_keylen;
};

template <class E, class D> class VariableRoundsCipherFactory : public CipherFactory
{
public:
	VariableRoundsCipherFactory(unsigned int keylen=0, unsigned int rounds=0)
		: m_keylen(keylen ? keylen : E::DEFAULT_KEYLENGTH), m_rounds(rounds ? rounds : E::DEFAULT_ROUNDS) {}
	unsigned int BlockSize() const {return E::BLOCKSIZE;}
	unsigned int KeyLength() const {return m_keylen;}

	apbt NewEncryption(const byte *keyStr) const
		{return apbt(new E(keyStr, m_keylen, m_rounds));}
	apbt NewDecryption(const byte *keyStr) const
		{return apbt(new D(keyStr, m_keylen, m_rounds));}

	unsigned int m_keylen, m_rounds;
};

bool BlockTransformationTest(const CipherFactory &cg, BufferedTransformation &valdata, unsigned int tuples = 0xffff)
{
	HexEncoder output(new FileSink(cout));
	SecByteBlock plain(cg.BlockSize()), cipher(cg.BlockSize()), out(cg.BlockSize()), outplain(cg.BlockSize());
	SecByteBlock key(cg.KeyLength());
	bool pass=true, fail;

	while (valdata.MaxRetrievable() && tuples--)
	{
		valdata.Get(key, cg.KeyLength());
		valdata.Get(plain, cg.BlockSize());
		valdata.Get(cipher, cg.BlockSize());

		apbt transE = cg.NewEncryption(key);
		transE->ProcessBlock(plain, out);
		fail = memcmp(out, cipher, cg.BlockSize()) != 0;

		apbt transD = cg.NewDecryption(key);
		transD->ProcessBlock(out, outplain);
		fail=fail || memcmp(outplain, plain, cg.BlockSize());

		pass = pass && !fail;

		cout << (fail ? "FAILED   " : "passed   ");
		output.Put(key, cg.KeyLength());
		cout << "   ";
		output.Put(outplain, cg.BlockSize());
		cout << "   ";
		output.Put(out, cg.BlockSize());
		cout << endl;
	}
	return pass;
}

class FilterTester : public Unflushable<Sink>
{
public:
	FilterTester(const byte *validOutput, size_t outputLen)
		: validOutput(validOutput), outputLen(outputLen), counter(0), fail(false) {}
	void PutByte(byte inByte)
	{
		if (counter >= outputLen || validOutput[counter] != inByte)
		{
			std::cerr << "incorrect output " << counter << ", " << (word16)validOutput[counter] << ", " << (word16)inByte << "\n";
			fail = true;
			CRYPTOPP_ASSERT(false);
		}
		counter++;
	}
	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
	{
		CRYPTOPP_UNUSED(messageEnd), CRYPTOPP_UNUSED(blocking);

		while (length--)
			FilterTester::PutByte(*inString++);

		if (messageEnd)
			if (counter != outputLen)
			{
				fail = true;
				CRYPTOPP_ASSERT(false);
			}

		return 0;
	}
	bool GetResult()
	{
		return !fail;
	}

	const byte *validOutput;
	size_t outputLen, counter;
	bool fail;
};

bool TestFilter(BufferedTransformation &bt, const byte *in, size_t inLen, const byte *out, size_t outLen)
{
	FilterTester *ft;
	bt.Attach(ft = new FilterTester(out, outLen));

	while (inLen)
	{
		size_t randomLen = GlobalRNG().GenerateWord32(0, (word32)inLen);
		bt.Put(in, randomLen);
		in += randomLen;
		inLen -= randomLen;
	}
	bt.MessageEnd();
	return ft->GetResult();
}

bool ValidateDES()
{
	cout << "\nDES validation suite running...\n\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/descert.dat", true, new HexDecoder);
	bool pass = BlockTransformationTest(FixedRoundsCipherFactory<DESEncryption, DESDecryption>(), valdata);

	cout << "\nTesting EDE2, EDE3, and XEX3 variants...\n\n";

	FileSource valdata1(CRYPTOPP_DATA_DIR "TestData/3desval.dat", true, new HexDecoder);
	pass = BlockTransformationTest(FixedRoundsCipherFactory<DES_EDE2_Encryption, DES_EDE2_Decryption>(), valdata1, 1) && pass;
	pass = BlockTransformationTest(FixedRoundsCipherFactory<DES_EDE3_Encryption, DES_EDE3_Decryption>(), valdata1, 1) && pass;
	pass = BlockTransformationTest(FixedRoundsCipherFactory<DES_XEX3_Encryption, DES_XEX3_Decryption>(), valdata1, 1) && pass;

	return pass;
}

bool TestModeIV(SymmetricCipher &e, SymmetricCipher &d)
{
	SecByteBlock lastIV, iv(e.IVSize());
	StreamTransformationFilter filter(e, new StreamTransformationFilter(d));

	// vector_ptr<byte> due to Enterprise Analysis finding on the stack based array.
	vector_ptr<byte> plaintext(20480);

	for (unsigned int i=1; i<20480; i*=2)
	{
		e.GetNextIV(GlobalRNG(), iv);
		if (iv == lastIV)
			return false;
		else
			lastIV = iv;

		e.Resynchronize(iv);
		d.Resynchronize(iv);

		unsigned int length = STDMAX(GlobalRNG().GenerateWord32(0, i), (word32)e.MinLastBlockSize());
		GlobalRNG().GenerateBlock(plaintext, length);

		if (!TestFilter(filter, plaintext, length, plaintext, length))
			return false;
	}

	return true;
}

bool ValidateCipherModes()
{
	cout << "\nTesting DES modes...\n\n";
	const byte key[] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef};
	const byte iv[] = {0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef};
	const byte plain[] = {	// "Now is the time for all " without tailing 0
		0x4e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,
		0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
		0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20};
	DESEncryption desE(key);
	DESDecryption desD(key);
	bool pass=true, fail;

	{
		// from FIPS 81
		const byte encrypted[] = {
			0x3f, 0xa4, 0x0e, 0x8a, 0x98, 0x4d, 0x48, 0x15,
			0x6a, 0x27, 0x17, 0x87, 0xab, 0x88, 0x83, 0xf9,
			0x89, 0x3d, 0x51, 0xec, 0x4b, 0x56, 0x3b, 0x53};

		ECB_Mode_ExternalCipher::Encryption modeE(desE);
		fail = !TestFilter(StreamTransformationFilter(modeE, NULL, StreamTransformationFilter::NO_PADDING).Ref(),
			plain, sizeof(plain), encrypted, sizeof(encrypted));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "ECB encryption" << endl;

		ECB_Mode_ExternalCipher::Decryption modeD(desD);
		fail = !TestFilter(StreamTransformationFilter(modeD, NULL, StreamTransformationFilter::NO_PADDING).Ref(),
			encrypted, sizeof(encrypted), plain, sizeof(plain));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "ECB decryption" << endl;
	}
	{
		// from FIPS 81
		const byte encrypted[] = {
			0xE5, 0xC7, 0xCD, 0xDE, 0x87, 0x2B, 0xF2, 0x7C,
			0x43, 0xE9, 0x34, 0x00, 0x8C, 0x38, 0x9C, 0x0F,
			0x68, 0x37, 0x88, 0x49, 0x9A, 0x7C, 0x05, 0xF6};

		CBC_Mode_ExternalCipher::Encryption modeE(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeE, NULL, StreamTransformationFilter::NO_PADDING).Ref(),
			plain, sizeof(plain), encrypted, sizeof(encrypted));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC encryption with no padding" << endl;

		CBC_Mode_ExternalCipher::Decryption modeD(desD, iv);
		fail = !TestFilter(StreamTransformationFilter(modeD, NULL, StreamTransformationFilter::NO_PADDING).Ref(),
			encrypted, sizeof(encrypted), plain, sizeof(plain));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC decryption with no padding" << endl;

		fail = !TestModeIV(modeE, modeD);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC mode IV generation" << endl;
	}
	{
		// generated with Crypto++, matches FIPS 81
		// but has extra 8 bytes as result of padding
		const byte encrypted[] = {
			0xE5, 0xC7, 0xCD, 0xDE, 0x87, 0x2B, 0xF2, 0x7C,
			0x43, 0xE9, 0x34, 0x00, 0x8C, 0x38, 0x9C, 0x0F,
			0x68, 0x37, 0x88, 0x49, 0x9A, 0x7C, 0x05, 0xF6,
			0x62, 0xC1, 0x6A, 0x27, 0xE4, 0xFC, 0xF2, 0x77};

		CBC_Mode_ExternalCipher::Encryption modeE(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeE).Ref(),
			plain, sizeof(plain), encrypted, sizeof(encrypted));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC encryption with PKCS #7 padding" << endl;

		CBC_Mode_ExternalCipher::Decryption modeD(desD, iv);
		fail = !TestFilter(StreamTransformationFilter(modeD).Ref(),
			encrypted, sizeof(encrypted), plain, sizeof(plain));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC decryption with PKCS #7 padding" << endl;
	}
	{
		// generated with Crypto++ 5.2, matches FIPS 81
		// but has extra 8 bytes as result of padding
		const byte encrypted[] = {
			0xE5, 0xC7, 0xCD, 0xDE, 0x87, 0x2B, 0xF2, 0x7C,
			0x43, 0xE9, 0x34, 0x00, 0x8C, 0x38, 0x9C, 0x0F,
			0x68, 0x37, 0x88, 0x49, 0x9A, 0x7C, 0x05, 0xF6,
			0xcf, 0xb7, 0xc7, 0x64, 0x0e, 0x7c, 0xd9, 0xa7};

		CBC_Mode_ExternalCipher::Encryption modeE(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeE, NULL, StreamTransformationFilter::ONE_AND_ZEROS_PADDING).Ref(),
			plain, sizeof(plain), encrypted, sizeof(encrypted));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC encryption with one-and-zeros padding" << endl;

		CBC_Mode_ExternalCipher::Decryption modeD(desD, iv);
		fail = !TestFilter(StreamTransformationFilter(modeD, NULL, StreamTransformationFilter::ONE_AND_ZEROS_PADDING).Ref(),
			encrypted, sizeof(encrypted), plain, sizeof(plain));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC decryption with one-and-zeros padding" << endl;
	}
	{
		const byte plain_1[] = {'a', 0, 0, 0, 0, 0, 0, 0};
		// generated with Crypto++
		const byte encrypted[] = {
			0x9B, 0x47, 0x57, 0x59, 0xD6, 0x9C, 0xF6, 0xD0};

		CBC_Mode_ExternalCipher::Encryption modeE(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeE, NULL, StreamTransformationFilter::ZEROS_PADDING).Ref(),
			plain_1, 1, encrypted, sizeof(encrypted));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC encryption with zeros padding" << endl;

		CBC_Mode_ExternalCipher::Decryption modeD(desD, iv);
		fail = !TestFilter(StreamTransformationFilter(modeD, NULL, StreamTransformationFilter::ZEROS_PADDING).Ref(),
			encrypted, sizeof(encrypted), plain_1, sizeof(plain_1));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC decryption with zeros padding" << endl;
	}
	{
		// generated with Crypto++, matches FIPS 81
		// but with last two blocks swapped as result of CTS
		const byte encrypted[] = {
			0xE5, 0xC7, 0xCD, 0xDE, 0x87, 0x2B, 0xF2, 0x7C,
			0x68, 0x37, 0x88, 0x49, 0x9A, 0x7C, 0x05, 0xF6,
			0x43, 0xE9, 0x34, 0x00, 0x8C, 0x38, 0x9C, 0x0F};

		CBC_CTS_Mode_ExternalCipher::Encryption modeE(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeE).Ref(),
			plain, sizeof(plain), encrypted, sizeof(encrypted));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC encryption with ciphertext stealing (CTS)" << endl;

		CBC_CTS_Mode_ExternalCipher::Decryption modeD(desD, iv);
		fail = !TestFilter(StreamTransformationFilter(modeD).Ref(),
			encrypted, sizeof(encrypted), plain, sizeof(plain));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC decryption with ciphertext stealing (CTS)" << endl;

		fail = !TestModeIV(modeE, modeD);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC CTS IV generation" << endl;
	}
	{
		// generated with Crypto++
		const byte decryptionIV[] = {0x4D, 0xD0, 0xAC, 0x8F, 0x47, 0xCF, 0x79, 0xCE};
		const byte encrypted[] = {0x12, 0x34, 0x56};

		byte stolenIV[8];

		CBC_CTS_Mode_ExternalCipher::Encryption modeE(desE, iv);
		modeE.SetStolenIV(stolenIV);
		fail = !TestFilter(StreamTransformationFilter(modeE).Ref(),
			plain, 3, encrypted, sizeof(encrypted));
		fail = memcmp(stolenIV, decryptionIV, 8) != 0 || fail;
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC encryption with ciphertext and IV stealing" << endl;

		CBC_CTS_Mode_ExternalCipher::Decryption modeD(desD, stolenIV);
		fail = !TestFilter(StreamTransformationFilter(modeD).Ref(),
			encrypted, sizeof(encrypted), plain, 3);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC decryption with ciphertext and IV stealing" << endl;
	}
	{
		const byte encrypted[] = {	// from FIPS 81
			0xF3,0x09,0x62,0x49,0xC7,0xF4,0x6E,0x51,
			0xA6,0x9E,0x83,0x9B,0x1A,0x92,0xF7,0x84,
			0x03,0x46,0x71,0x33,0x89,0x8E,0xA6,0x22};

		CFB_Mode_ExternalCipher::Encryption modeE(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeE).Ref(),
			plain, sizeof(plain), encrypted, sizeof(encrypted));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CFB encryption" << endl;

		CFB_Mode_ExternalCipher::Decryption modeD(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeD).Ref(),
			encrypted, sizeof(encrypted), plain, sizeof(plain));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CFB decryption" << endl;

		fail = !TestModeIV(modeE, modeD);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CFB mode IV generation" << endl;
	}
	{
		const byte plain_2[] = {	// "Now is the." without tailing 0
			0x4e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,0x68,0x65};
		const byte encrypted[] = {	// from FIPS 81
			0xf3,0x1f,0xda,0x07,0x01,0x14,0x62,0xee,0x18,0x7f};

		CFB_Mode_ExternalCipher::Encryption modeE(desE, iv, 1);
		fail = !TestFilter(StreamTransformationFilter(modeE).Ref(),
			plain_2, sizeof(plain_2), encrypted, sizeof(encrypted));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CFB (8-bit feedback) encryption" << endl;

		CFB_Mode_ExternalCipher::Decryption modeD(desE, iv, 1);
		fail = !TestFilter(StreamTransformationFilter(modeD).Ref(),
			encrypted, sizeof(encrypted), plain_2, sizeof(plain_2));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CFB (8-bit feedback) decryption" << endl;

		fail = !TestModeIV(modeE, modeD);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CFB (8-bit feedback) IV generation" << endl;
	}
	{
		const byte encrypted[] = {	// from Eric Young's libdes
			0xf3,0x09,0x62,0x49,0xc7,0xf4,0x6e,0x51,
			0x35,0xf2,0x4a,0x24,0x2e,0xeb,0x3d,0x3f,
			0x3d,0x6d,0x5b,0xe3,0x25,0x5a,0xf8,0xc3};

		OFB_Mode_ExternalCipher::Encryption modeE(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeE).Ref(),
			plain, sizeof(plain), encrypted, sizeof(encrypted));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "OFB encryption" << endl;

		OFB_Mode_ExternalCipher::Decryption modeD(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeD).Ref(),
			encrypted, sizeof(encrypted), plain, sizeof(plain));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "OFB decryption" << endl;

		fail = !TestModeIV(modeE, modeD);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "OFB IV generation" << endl;
	}
	{
		const byte encrypted[] = {	// generated with Crypto++
			0xF3, 0x09, 0x62, 0x49, 0xC7, 0xF4, 0x6E, 0x51,
			0x16, 0x3A, 0x8C, 0xA0, 0xFF, 0xC9, 0x4C, 0x27,
			0xFA, 0x2F, 0x80, 0xF4, 0x80, 0xB8, 0x6F, 0x75};

		CTR_Mode_ExternalCipher::Encryption modeE(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeE).Ref(),
			plain, sizeof(plain), encrypted, sizeof(encrypted));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "Counter Mode encryption" << endl;

		CTR_Mode_ExternalCipher::Decryption modeD(desE, iv);
		fail = !TestFilter(StreamTransformationFilter(modeD).Ref(),
			encrypted, sizeof(encrypted), plain, sizeof(plain));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "Counter Mode decryption" << endl;

		fail = !TestModeIV(modeE, modeD);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "Counter Mode IV generation" << endl;
	}
	{
		const byte plain_3[] = {	// "7654321 Now is the time for "
			0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x20,
			0x4e, 0x6f, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74,
			0x68, 0x65, 0x20, 0x74, 0x69, 0x6d, 0x65, 0x20,
			0x66, 0x6f, 0x72, 0x20};
		const byte mac1[] = {	// from FIPS 113
			0xf1, 0xd3, 0x0f, 0x68, 0x49, 0x31, 0x2c, 0xa4};
		const byte mac2[] = {	// generated with Crypto++
			0x35, 0x80, 0xC5, 0xC4, 0x6B, 0x81, 0x24, 0xE2};

		CBC_MAC<DES> cbcmac(key);
		HashFilter cbcmacFilter(cbcmac);
		fail = !TestFilter(cbcmacFilter, plain_3, sizeof(plain_3), mac1, sizeof(mac1));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "CBC MAC" << endl;

		DMAC<DES> dmac(key);
		HashFilter dmacFilter(dmac);
		fail = !TestFilter(dmacFilter, plain_3, sizeof(plain_3), mac2, sizeof(mac2));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "DMAC" << endl;
	}
	{
		CTR_Mode<AES>::Encryption modeE(plain, 16, plain);
		CTR_Mode<AES>::Decryption modeD(plain, 16, plain);
		fail = !TestModeIV(modeE, modeD);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "AES CTR Mode" << endl;
	}
	{
		OFB_Mode<AES>::Encryption modeE(plain, 16, plain);
		OFB_Mode<AES>::Decryption modeD(plain, 16, plain);
		fail = !TestModeIV(modeE, modeD);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "AES OFB Mode" << endl;
	}
	{
		CFB_Mode<AES>::Encryption modeE(plain, 16, plain);
		CFB_Mode<AES>::Decryption modeD(plain, 16, plain);
		fail = !TestModeIV(modeE, modeD);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "AES CFB Mode" << endl;
	}
	{
		CBC_Mode<AES>::Encryption modeE(plain, 16, plain);
		CBC_Mode<AES>::Decryption modeD(plain, 16, plain);
		fail = !TestModeIV(modeE, modeD);
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ") << "AES CBC Mode" << endl;
	}

	return pass;
}

bool ValidateIDEA()
{
	cout << "\nIDEA validation suite running...\n\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/ideaval.dat", true, new HexDecoder);
	return BlockTransformationTest(FixedRoundsCipherFactory<IDEAEncryption, IDEADecryption>(), valdata);
}

bool ValidateSAFER()
{
	cout << "\nSAFER validation suite running...\n\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/saferval.dat", true, new HexDecoder);
	bool pass = true;
	pass = BlockTransformationTest(VariableRoundsCipherFactory<SAFER_K_Encryption, SAFER_K_Decryption>(8,6), valdata, 4) && pass;
	pass = BlockTransformationTest(VariableRoundsCipherFactory<SAFER_K_Encryption, SAFER_K_Decryption>(16,12), valdata, 4) && pass;
	pass = BlockTransformationTest(VariableRoundsCipherFactory<SAFER_SK_Encryption, SAFER_SK_Decryption>(8,6), valdata, 4) && pass;
	pass = BlockTransformationTest(VariableRoundsCipherFactory<SAFER_SK_Encryption, SAFER_SK_Decryption>(16,10), valdata, 4) && pass;
	return pass;
}

bool ValidateRC2()
{
	cout << "\nRC2 validation suite running...\n\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/rc2val.dat", true, new HexDecoder);
	HexEncoder output(new FileSink(cout));
	SecByteBlock plain(RC2Encryption::BLOCKSIZE), cipher(RC2Encryption::BLOCKSIZE), out(RC2Encryption::BLOCKSIZE), outplain(RC2Encryption::BLOCKSIZE);
	SecByteBlock key(128);
	bool pass=true, fail;

	while (valdata.MaxRetrievable())
	{
		byte keyLen, effectiveLen;

		valdata.Get(keyLen);
		valdata.Get(effectiveLen);
		valdata.Get(key, keyLen);
		valdata.Get(plain, RC2Encryption::BLOCKSIZE);
		valdata.Get(cipher, RC2Encryption::BLOCKSIZE);

		apbt transE(new RC2Encryption(key, keyLen, effectiveLen));
		transE->ProcessBlock(plain, out);
		fail = memcmp(out, cipher, RC2Encryption::BLOCKSIZE) != 0;

		apbt transD(new RC2Decryption(key, keyLen, effectiveLen));
		transD->ProcessBlock(out, outplain);
		fail=fail || memcmp(outplain, plain, RC2Encryption::BLOCKSIZE);

		pass = pass && !fail;

		cout << (fail ? "FAILED   " : "passed   ");
		output.Put(key, keyLen);
		cout << "   ";
		output.Put(outplain, RC2Encryption::BLOCKSIZE);
		cout << "   ";
		output.Put(out, RC2Encryption::BLOCKSIZE);
		cout << endl;
	}
	return pass;
}

bool ValidateARC4()
{
	unsigned char Key0[] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef };
	unsigned char Input0[]={0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef};
	unsigned char Output0[] = {0x75,0xb7,0x87,0x80,0x99,0xe0,0xc5,0x96};

	unsigned char Key1[]={0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef};
	unsigned char Input1[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char Output1[]={0x74,0x94,0xc2,0xe7,0x10,0x4b,0x08,0x79};

	unsigned char Key2[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char Input2[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char Output2[]={0xde,0x18,0x89,0x41,0xa3,0x37,0x5d,0x3a};

	unsigned char Key3[]={0xef,0x01,0x23,0x45};
	unsigned char Input3[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char Output3[]={0xd6,0xa1,0x41,0xa7,0xec,0x3c,0x38,0xdf,0xbd,0x61};

	unsigned char Key4[]={ 0x01,0x23,0x45,0x67,0x89,0xab, 0xcd,0xef };
	unsigned char Input4[] =
	{0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01};
	unsigned char Output4[]= {
	0x75,0x95,0xc3,0xe6,0x11,0x4a,0x09,0x78,0x0c,0x4a,0xd4,
	0x52,0x33,0x8e,0x1f,0xfd,0x9a,0x1b,0xe9,0x49,0x8f,
	0x81,0x3d,0x76,0x53,0x34,0x49,0xb6,0x77,0x8d,0xca,
	0xd8,0xc7,0x8a,0x8d,0x2b,0xa9,0xac,0x66,0x08,0x5d,
	0x0e,0x53,0xd5,0x9c,0x26,0xc2,0xd1,0xc4,0x90,0xc1,
	0xeb,0xbe,0x0c,0xe6,0x6d,0x1b,0x6b,0x1b,0x13,0xb6,
	0xb9,0x19,0xb8,0x47,0xc2,0x5a,0x91,0x44,0x7a,0x95,
	0xe7,0x5e,0x4e,0xf1,0x67,0x79,0xcd,0xe8,0xbf,0x0a,
	0x95,0x85,0x0e,0x32,0xaf,0x96,0x89,0x44,0x4f,0xd3,
	0x77,0x10,0x8f,0x98,0xfd,0xcb,0xd4,0xe7,0x26,0x56,
	0x75,0x00,0x99,0x0b,0xcc,0x7e,0x0c,0xa3,0xc4,0xaa,
	0xa3,0x04,0xa3,0x87,0xd2,0x0f,0x3b,0x8f,0xbb,0xcd,
	0x42,0xa1,0xbd,0x31,0x1d,0x7a,0x43,0x03,0xdd,0xa5,
	0xab,0x07,0x88,0x96,0xae,0x80,0xc1,0x8b,0x0a,0xf6,
	0x6d,0xff,0x31,0x96,0x16,0xeb,0x78,0x4e,0x49,0x5a,
	0xd2,0xce,0x90,0xd7,0xf7,0x72,0xa8,0x17,0x47,0xb6,
	0x5f,0x62,0x09,0x3b,0x1e,0x0d,0xb9,0xe5,0xba,0x53,
	0x2f,0xaf,0xec,0x47,0x50,0x83,0x23,0xe6,0x71,0x32,
	0x7d,0xf9,0x44,0x44,0x32,0xcb,0x73,0x67,0xce,0xc8,
	0x2f,0x5d,0x44,0xc0,0xd0,0x0b,0x67,0xd6,0x50,0xa0,
	0x75,0xcd,0x4b,0x70,0xde,0xdd,0x77,0xeb,0x9b,0x10,
	0x23,0x1b,0x6b,0x5b,0x74,0x13,0x47,0x39,0x6d,0x62,
	0x89,0x74,0x21,0xd4,0x3d,0xf9,0xb4,0x2e,0x44,0x6e,
	0x35,0x8e,0x9c,0x11,0xa9,0xb2,0x18,0x4e,0xcb,0xef,
	0x0c,0xd8,0xe7,0xa8,0x77,0xef,0x96,0x8f,0x13,0x90,
	0xec,0x9b,0x3d,0x35,0xa5,0x58,0x5c,0xb0,0x09,0x29,
	0x0e,0x2f,0xcd,0xe7,0xb5,0xec,0x66,0xd9,0x08,0x4b,
	0xe4,0x40,0x55,0xa6,0x19,0xd9,0xdd,0x7f,0xc3,0x16,
	0x6f,0x94,0x87,0xf7,0xcb,0x27,0x29,0x12,0x42,0x64,
	0x45,0x99,0x85,0x14,0xc1,0x5d,0x53,0xa1,0x8c,0x86,
	0x4c,0xe3,0xa2,0xb7,0x55,0x57,0x93,0x98,0x81,0x26,
	0x52,0x0e,0xac,0xf2,0xe3,0x06,0x6e,0x23,0x0c,0x91,
	0xbe,0xe4,0xdd,0x53,0x04,0xf5,0xfd,0x04,0x05,0xb3,
	0x5b,0xd9,0x9c,0x73,0x13,0x5d,0x3d,0x9b,0xc3,0x35,
	0xee,0x04,0x9e,0xf6,0x9b,0x38,0x67,0xbf,0x2d,0x7b,
	0xd1,0xea,0xa5,0x95,0xd8,0xbf,0xc0,0x06,0x6f,0xf8,
	0xd3,0x15,0x09,0xeb,0x0c,0x6c,0xaa,0x00,0x6c,0x80,
	0x7a,0x62,0x3e,0xf8,0x4c,0x3d,0x33,0xc1,0x95,0xd2,
	0x3e,0xe3,0x20,0xc4,0x0d,0xe0,0x55,0x81,0x57,0xc8,
	0x22,0xd4,0xb8,0xc5,0x69,0xd8,0x49,0xae,0xd5,0x9d,
	0x4e,0x0f,0xd7,0xf3,0x79,0x58,0x6b,0x4b,0x7f,0xf6,
	0x84,0xed,0x6a,0x18,0x9f,0x74,0x86,0xd4,0x9b,0x9c,
	0x4b,0xad,0x9b,0xa2,0x4b,0x96,0xab,0xf9,0x24,0x37,
	0x2c,0x8a,0x8f,0xff,0xb1,0x0d,0x55,0x35,0x49,0x00,
	0xa7,0x7a,0x3d,0xb5,0xf2,0x05,0xe1,0xb9,0x9f,0xcd,
	0x86,0x60,0x86,0x3a,0x15,0x9a,0xd4,0xab,0xe4,0x0f,
	0xa4,0x89,0x34,0x16,0x3d,0xdd,0xe5,0x42,0xa6,0x58,
	0x55,0x40,0xfd,0x68,0x3c,0xbf,0xd8,0xc0,0x0f,0x12,
	0x12,0x9a,0x28,0x4d,0xea,0xcc,0x4c,0xde,0xfe,0x58,
	0xbe,0x71,0x37,0x54,0x1c,0x04,0x71,0x26,0xc8,0xd4,
	0x9e,0x27,0x55,0xab,0x18,0x1a,0xb7,0xe9,0x40,0xb0,
	0xc0};

	// VC60 workaround: auto_ptr lacks reset()
	member_ptr<Weak::ARC4> arc4;
	bool pass=true, fail;
	unsigned int i;

	cout << "\nARC4 validation suite running...\n\n";

	arc4.reset(new Weak::ARC4(Key0, sizeof(Key0)));
	arc4->ProcessString(Input0, sizeof(Input0));
	fail = memcmp(Input0, Output0, sizeof(Input0)) != 0;
	cout << (fail ? "FAILED" : "passed") << "    Test 0" << endl;
	pass = pass && !fail;

	arc4.reset(new Weak::ARC4(Key1, sizeof(Key1)));
	arc4->ProcessString(Key1, Input1, sizeof(Key1));
	fail = memcmp(Output1, Key1, sizeof(Key1)) != 0;
	cout << (fail ? "FAILED" : "passed") << "    Test 1" << endl;
	pass = pass && !fail;

	arc4.reset(new Weak::ARC4(Key2, sizeof(Key2)));
	for (i=0, fail=false; i<sizeof(Input2); i++)
		if (arc4->ProcessByte(Input2[i]) != Output2[i])
			fail = true;
	cout << (fail ? "FAILED" : "passed") << "    Test 2" << endl;
	pass = pass && !fail;

	arc4.reset(new Weak::ARC4(Key3, sizeof(Key3)));
	for (i=0, fail=false; i<sizeof(Input3); i++)
		if (arc4->ProcessByte(Input3[i]) != Output3[i])
			fail = true;
	cout << (fail ? "FAILED" : "passed") << "    Test 3" << endl;
	pass = pass && !fail;

	arc4.reset(new Weak::ARC4(Key4, sizeof(Key4)));
	for (i=0, fail=false; i<sizeof(Input4); i++)
		if (arc4->ProcessByte(Input4[i]) != Output4[i])
			fail = true;
	cout << (fail ? "FAILED" : "passed") << "    Test 4" << endl;
	pass = pass && !fail;

	return pass;
}

bool ValidateRC5()
{
	cout << "\nRC5 validation suite running...\n\n";
	bool pass1 = true, pass2 = true;

	RC5Encryption enc;  // 0 to 2040-bits (255-bytes)
	pass1 = RC5Encryption::DEFAULT_KEYLENGTH ==  16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(0) == 0 && pass1;
	pass1 = enc.StaticGetValidKeyLength(254) == 254 && pass1;
	pass1 = enc.StaticGetValidKeyLength(255) == 255 && pass1;
	pass1 = enc.StaticGetValidKeyLength(256) == 255 && pass1;
	pass1 = enc.StaticGetValidKeyLength(0) == enc.MinKeyLength() && pass1;
	pass1 = enc.StaticGetValidKeyLength(SIZE_MAX) == enc.MaxKeyLength() && pass1;

	RC5Decryption dec;
	pass2 = RC5Decryption::DEFAULT_KEYLENGTH ==  16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(0) == 0 && pass2;
	pass2 = dec.StaticGetValidKeyLength(254) == 254 && pass2;
	pass2 = dec.StaticGetValidKeyLength(255) == 255 && pass2;
	pass2 = dec.StaticGetValidKeyLength(256) == 255 && pass2;
	pass2 = dec.StaticGetValidKeyLength(0) == dec.MinKeyLength() && pass2;
	pass2 = dec.StaticGetValidKeyLength(SIZE_MAX) == dec.MaxKeyLength() && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/rc5val.dat", true, new HexDecoder);
	return BlockTransformationTest(VariableRoundsCipherFactory<RC5Encryption, RC5Decryption>(16, 12), valdata) && pass1 && pass2;
}

bool ValidateRC6()
{
	cout << "\nRC6 validation suite running...\n\n";
	bool pass1 = true, pass2 = true, pass3 = true;

	RC6Encryption enc;
	pass1 = enc.StaticGetValidKeyLength(8) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(24) == 24 && pass1;
	pass1 = enc.StaticGetValidKeyLength(32) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(64) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(128) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(0) == enc.MinKeyLength() && pass1;
	pass1 = enc.StaticGetValidKeyLength(SIZE_MAX) == enc.MaxKeyLength() && pass1;

	RC6Decryption dec;
	pass2 = dec.StaticGetValidKeyLength(8) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(24) == 24 && pass2;
	pass2 = dec.StaticGetValidKeyLength(32) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(64) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(128) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(0) == dec.MinKeyLength() && pass2;
	pass2 = dec.StaticGetValidKeyLength(SIZE_MAX) == dec.MaxKeyLength() && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/rc6val.dat", true, new HexDecoder);
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<RC6Encryption, RC6Decryption>(16), valdata, 2) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<RC6Encryption, RC6Decryption>(24), valdata, 2) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<RC6Encryption, RC6Decryption>(32), valdata, 2) && pass3;
	return pass1 && pass2 && pass3;
}

bool ValidateMARS()
{
	cout << "\nMARS validation suite running...\n\n";
	bool pass1 = true, pass2 = true, pass3 = true;

	MARSEncryption enc;
	pass1 = enc.StaticGetValidKeyLength(8) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(24) == 24 && pass1;
	pass1 = enc.StaticGetValidKeyLength(32) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(64) == 56 && pass1;
	pass1 = enc.StaticGetValidKeyLength(128) == 56 && pass1;
	pass1 = enc.StaticGetValidKeyLength(0) == enc.MinKeyLength() && pass1;
	pass1 = enc.StaticGetValidKeyLength(SIZE_MAX) == enc.MaxKeyLength() && pass1;

	MARSDecryption dec;
	pass2 = dec.StaticGetValidKeyLength(8) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(24) == 24 && pass2;
	pass2 = dec.StaticGetValidKeyLength(32) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(64) == 56 && pass2;
	pass2 = dec.StaticGetValidKeyLength(128) == 56 && pass2;
	pass2 = dec.StaticGetValidKeyLength(0) == dec.MinKeyLength() && pass2;
	pass2 = dec.StaticGetValidKeyLength(SIZE_MAX) == dec.MaxKeyLength() && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/marsval.dat", true, new HexDecoder);
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<MARSEncryption, MARSDecryption>(16), valdata, 4) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<MARSEncryption, MARSDecryption>(24), valdata, 3) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<MARSEncryption, MARSDecryption>(32), valdata, 2) && pass3;
	return pass1 && pass2 && pass3;
}

bool ValidateRijndael()
{
	cout << "\nRijndael (AES) validation suite running...\n\n";
	bool pass1 = true, pass2 = true, pass3 = true;

	RijndaelEncryption enc;
	pass1 = enc.StaticGetValidKeyLength(8) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(24) == 24 && pass1;
	pass1 = enc.StaticGetValidKeyLength(32) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(64) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(128) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(0) == enc.MinKeyLength() && pass1;
	pass1 = enc.StaticGetValidKeyLength(SIZE_MAX) == enc.MaxKeyLength() && pass1;

	RijndaelDecryption dec;
	pass2 = dec.StaticGetValidKeyLength(8) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(24) == 24 && pass2;
	pass2 = dec.StaticGetValidKeyLength(32) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(64) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(128) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(0) == dec.MinKeyLength() && pass2;
	pass2 = dec.StaticGetValidKeyLength(SIZE_MAX) == dec.MaxKeyLength() && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/rijndael.dat", true, new HexDecoder);
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<RijndaelEncryption, RijndaelDecryption>(16), valdata, 4) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<RijndaelEncryption, RijndaelDecryption>(24), valdata, 3) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<RijndaelEncryption, RijndaelDecryption>(32), valdata, 2) && pass3;
	pass3 = RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/aes.txt") && pass3;
	return pass1 && pass2 && pass3;
}

bool ValidateTwofish()
{
	cout << "\nTwofish validation suite running...\n\n";
	bool pass1 = true, pass2 = true, pass3 = true;

	TwofishEncryption enc;
	pass1 = enc.StaticGetValidKeyLength(8) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(24) == 24 && pass1;
	pass1 = enc.StaticGetValidKeyLength(32) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(64) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(128) == 32 && pass1;

	TwofishDecryption dec;
	pass2 = dec.StaticGetValidKeyLength(8) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(24) == 24 && pass2;
	pass2 = dec.StaticGetValidKeyLength(32) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(64) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(128) == 32 && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/twofishv.dat", true, new HexDecoder);
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<TwofishEncryption, TwofishDecryption>(16), valdata, 4) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<TwofishEncryption, TwofishDecryption>(24), valdata, 3) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<TwofishEncryption, TwofishDecryption>(32), valdata, 2) && pass3;
	return pass1 && pass2 && pass3;
}

bool ValidateSerpent()
{
	cout << "\nSerpent validation suite running...\n\n";
	bool pass1 = true, pass2 = true, pass3 = true;

	SerpentEncryption enc;
	pass1 = enc.StaticGetValidKeyLength(8) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(24) == 24 && pass1;
	pass1 = enc.StaticGetValidKeyLength(32) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(64) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(128) == 32 && pass1;

	SerpentDecryption dec;
	pass2 = dec.StaticGetValidKeyLength(8) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(24) == 24 && pass2;
	pass2 = dec.StaticGetValidKeyLength(32) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(64) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(128) == 32 && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/serpentv.dat", true, new HexDecoder);
	bool pass = true;
	pass = BlockTransformationTest(FixedRoundsCipherFactory<SerpentEncryption, SerpentDecryption>(16), valdata, 5) && pass;
	pass = BlockTransformationTest(FixedRoundsCipherFactory<SerpentEncryption, SerpentDecryption>(24), valdata, 4) && pass;
	pass = BlockTransformationTest(FixedRoundsCipherFactory<SerpentEncryption, SerpentDecryption>(32), valdata, 3) && pass;
	return pass1 && pass2 && pass3;
}

bool ValidateBlowfish()
{
	cout << "\nBlowfish validation suite running...\n\n";
	bool pass1 = true, pass2 = true, pass3 = true, fail;

	BlowfishEncryption enc1;	// 32 to 448-bits (4 to 56-bytes)
	pass1 = enc1.StaticGetValidKeyLength(3) == 4 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(4) == 4 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(5) == 5 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(8) == 8 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(24) == 24 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(32) == 32 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(56) == 56 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(57) == 56 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(60) == 56 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(64) == 56 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(128) == 56 && pass1;

	BlowfishDecryption dec1; // 32 to 448-bits (4 to 56-bytes)
	pass2 = dec1.StaticGetValidKeyLength(3) == 4 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(4) == 4 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(5) == 5 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(8) == 8 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(24) == 24 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(32) == 32 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(56) == 56 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(57) == 56 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(60) == 56 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(64) == 56 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(128) == 56 && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	HexEncoder output(new FileSink(cout));
	const char *key[]={"abcdefghijklmnopqrstuvwxyz", "Who is John Galt?"};
	byte *plain[]={(byte *)"BLOWFISH", (byte *)"\xfe\xdc\xba\x98\x76\x54\x32\x10"};
	byte *cipher[]={(byte *)"\x32\x4e\xd0\xfe\xf4\x13\xa2\x03", (byte *)"\xcc\x91\x73\x2b\x80\x22\xf6\x84"};
	byte out[8], outplain[8];

	for (int i=0; i<2; i++)
	{
		ECB_Mode<Blowfish>::Encryption enc2((byte *)key[i], strlen(key[i]));
		enc2.ProcessData(out, plain[i], 8);
		fail = memcmp(out, cipher[i], 8) != 0;

		ECB_Mode<Blowfish>::Decryption dec2((byte *)key[i], strlen(key[i]));
		dec2.ProcessData(outplain, cipher[i], 8);
		fail = fail || memcmp(outplain, plain[i], 8);
		pass3 = pass3 && !fail;

		cout << (fail ? "FAILED    " : "passed    ");
		cout << '\"' << key[i] << '\"';
		for (int j=0; j<(signed int)(30-strlen(key[i])); j++)
			cout << ' ';
		output.Put(outplain, 8);
		cout << "  ";
		output.Put(out, 8);
		cout << endl;
	}
	return pass1 && pass2 && pass3;
}

bool ValidateThreeWay()
{
	cout << "\n3-WAY validation suite running...\n\n";
	bool pass1 = true, pass2 = true;

	ThreeWayEncryption enc;  // 96-bit only
	pass1 = ThreeWayEncryption::KEYLENGTH ==  12 && pass1;
	pass1 = enc.StaticGetValidKeyLength(8) == 12 && pass1;
	pass1 = enc.StaticGetValidKeyLength(12) == 12 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 12 && pass1;

	ThreeWayDecryption dec;  // 96-bit only
	pass2 = ThreeWayDecryption::KEYLENGTH ==  12 && pass2;
	pass2 = dec.StaticGetValidKeyLength(8) == 12 && pass2;
	pass2 = dec.StaticGetValidKeyLength(12) == 12 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 12 && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/3wayval.dat", true, new HexDecoder);
	return BlockTransformationTest(FixedRoundsCipherFactory<ThreeWayEncryption, ThreeWayDecryption>(), valdata) && pass1 && pass2;
}

bool ValidateGOST()
{
	cout << "\nGOST validation suite running...\n\n";
	bool pass1 = true, pass2 = true;

	GOSTEncryption enc;  // 256-bit only
	pass1 = GOSTEncryption::KEYLENGTH ==  32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(24) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(32) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(40) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(64) == 32 && pass1;

	GOSTDecryption dec;  // 256-bit only
	pass2 = GOSTDecryption::KEYLENGTH ==  32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(24) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(32) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(40) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(64) == 32 && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/gostval.dat", true, new HexDecoder);
	return BlockTransformationTest(FixedRoundsCipherFactory<GOSTEncryption, GOSTDecryption>(), valdata) && pass1 && pass2;
}

bool ValidateSHARK()
{
	cout << "\nSHARK validation suite running...\n\n";
	bool pass1 = true, pass2 = true;

	SHARKEncryption enc;  // 128-bit only
	pass1 = SHARKEncryption::KEYLENGTH ==  16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(8) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(15) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(17) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(32) == 16 && pass1;

	SHARKDecryption dec;  // 128-bit only
	pass2 = SHARKDecryption::KEYLENGTH ==  16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(8) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(15) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(17) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(32) == 16 && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/sharkval.dat", true, new HexDecoder);
	return BlockTransformationTest(FixedRoundsCipherFactory<SHARKEncryption, SHARKDecryption>(), valdata) && pass1 && pass2;
}

bool ValidateCAST()
{
	cout << "\nCAST-128 validation suite running...\n\n";
	bool pass1 = true, pass2 = true, pass3 = true;

	CAST128Encryption enc1;  // 40 to 128-bits (5 to 16-bytes)
	pass1 = CAST128Encryption::DEFAULT_KEYLENGTH ==  16 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(4) == 5 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(5) == 5 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(15) == 15 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc1.StaticGetValidKeyLength(17) == 16 && pass1;

	CAST128Decryption dec1;  // 40 to 128-bits (5 to 16-bytes)
	pass2 = CAST128Decryption::DEFAULT_KEYLENGTH ==  16 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(4) == 5 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(5) == 5 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(15) == 15 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec1.StaticGetValidKeyLength(17) == 16 && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource val128(CRYPTOPP_DATA_DIR "TestData/cast128v.dat", true, new HexDecoder);
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<CAST128Encryption, CAST128Decryption>(16), val128, 1) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<CAST128Encryption, CAST128Decryption>(10), val128, 1) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<CAST128Encryption, CAST128Decryption>(5), val128, 1) && pass3;

	cout << "\nCAST-256 validation suite running...\n\n";
	bool pass4 = true, pass5 = true, pass6 = true;

	CAST256Encryption enc2;  // 128, 160, 192, 224, or 256-bits (16 to 32-bytes, step 4)
	pass1 = CAST128Encryption::DEFAULT_KEYLENGTH ==  16 && pass1;
	pass4 = enc2.StaticGetValidKeyLength(15) == 16 && pass4;
	pass4 = enc2.StaticGetValidKeyLength(16) == 16 && pass4;
	pass4 = enc2.StaticGetValidKeyLength(17) == 20 && pass4;
	pass4 = enc2.StaticGetValidKeyLength(20) == 20 && pass4;
	pass4 = enc2.StaticGetValidKeyLength(24) == 24 && pass4;
	pass4 = enc2.StaticGetValidKeyLength(28) == 28 && pass4;
	pass4 = enc2.StaticGetValidKeyLength(31) == 32 && pass4;
	pass4 = enc2.StaticGetValidKeyLength(32) == 32 && pass4;
	pass4 = enc2.StaticGetValidKeyLength(33) == 32 && pass4;

	CAST256Decryption dec2;  // 128, 160, 192, 224, or 256-bits (16 to 32-bytes, step 4)
	pass2 = CAST256Decryption::DEFAULT_KEYLENGTH ==  16 && pass2;
	pass5 = dec2.StaticGetValidKeyLength(15) == 16 && pass5;
	pass5 = dec2.StaticGetValidKeyLength(16) == 16 && pass5;
	pass5 = dec2.StaticGetValidKeyLength(17) == 20 && pass5;
	pass5 = dec2.StaticGetValidKeyLength(20) == 20 && pass5;
	pass5 = dec2.StaticGetValidKeyLength(24) == 24 && pass5;
	pass5 = dec2.StaticGetValidKeyLength(28) == 28 && pass5;
	pass5 = dec2.StaticGetValidKeyLength(31) == 32 && pass5;
	pass5 = dec2.StaticGetValidKeyLength(32) == 32 && pass5;
	pass5 = dec2.StaticGetValidKeyLength(33) == 32 && pass5;
	cout << (pass4 && pass5 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource val256(CRYPTOPP_DATA_DIR "TestData/cast256v.dat", true, new HexDecoder);
	pass6 = BlockTransformationTest(FixedRoundsCipherFactory<CAST256Encryption, CAST256Decryption>(16), val256, 1) && pass6;
	pass6 = BlockTransformationTest(FixedRoundsCipherFactory<CAST256Encryption, CAST256Decryption>(24), val256, 1) && pass6;
	pass6 = BlockTransformationTest(FixedRoundsCipherFactory<CAST256Encryption, CAST256Decryption>(32), val256, 1) && pass6;

	return pass1 && pass2 && pass3 && pass4 && pass5 && pass6;
}

bool ValidateSquare()
{
	cout << "\nSquare validation suite running...\n\n";
	bool pass1 = true, pass2 = true;

	SquareEncryption enc;  // 128-bits only
	pass1 = enc.StaticGetValidKeyLength(8) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(15) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(17) == 16 && pass1;

	SquareDecryption dec;  // 128-bits only
	pass2 = dec.StaticGetValidKeyLength(8) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(15) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(17) == 16 && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/squareva.dat", true, new HexDecoder);
	return BlockTransformationTest(FixedRoundsCipherFactory<SquareEncryption, SquareDecryption>(), valdata) && pass1 && pass2;
}

bool ValidateSKIPJACK()
{
	cout << "\nSKIPJACK validation suite running...\n\n";
	bool pass1 = true, pass2 = true;

	SKIPJACKEncryption enc;  // 80-bits only
	pass1 = enc.StaticGetValidKeyLength(8) == 10 && pass1;
	pass1 = enc.StaticGetValidKeyLength(9) == 10 && pass1;
	pass1 = enc.StaticGetValidKeyLength(10) == 10 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 10 && pass1;

	SKIPJACKDecryption dec;  // 80-bits only
	pass2 = dec.StaticGetValidKeyLength(8) == 10 && pass2;
	pass2 = dec.StaticGetValidKeyLength(9) == 10 && pass2;
	pass2 = dec.StaticGetValidKeyLength(10) == 10 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 10 && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/skipjack.dat", true, new HexDecoder);
	return BlockTransformationTest(FixedRoundsCipherFactory<SKIPJACKEncryption, SKIPJACKDecryption>(), valdata) && pass1 && pass2;
}

bool ValidateSEAL()
{
	static const byte input[] = {0x37,0xa0,0x05,0x95,0x9b,0x84,0xc4,0x9c,0xa4,0xbe,0x1e,0x05,0x06,0x73,0x53,0x0f,0x5f,0xb0,0x97,0xfd,0xf6,0xa1,0x3f,0xbd,0x6c,0x2c,0xde,0xcd,0x81,0xfd,0xee,0x7c};
	static const byte key[] = {0x67, 0x45, 0x23, 0x01, 0xef, 0xcd, 0xab, 0x89, 0x98, 0xba, 0xdc, 0xfe, 0x10, 0x32, 0x54, 0x76, 0xc3, 0xd2, 0xe1, 0xf0};
	static const byte iv[] = {0x01, 0x35, 0x77, 0xaf};
	byte output[32];

	cout << "\nSEAL validation suite running...\n\n";

	SEAL<>::Encryption seal(key, sizeof(key), iv);
	unsigned int size = sizeof(input);
	bool pass = true;

	memset(output, 1, size);
	seal.ProcessString(output, input, size);
	for (unsigned int i=0; i<size; i++)
		if (output[i] != 0)
			pass = false;

	seal.Seek(1);
	output[1] = seal.ProcessByte(output[1]);
	seal.ProcessString(output+2, size-2);
	pass = pass && memcmp(output+1, input+1, size-1) == 0;

	cout << (pass ? "passed" : "FAILED") << endl;
	return pass;
}

bool ValidateBaseCode()
{
	bool pass = true, fail;
	byte data[255];
	for (unsigned int i=0; i<255; i++)
		data[i] = byte(i);
	static const char hexEncoded[] =
"000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F2021222324252627"
"28292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F"
"505152535455565758595A5B5C5D5E5F606162636465666768696A6B6C6D6E6F7071727374757677"
"78797A7B7C7D7E7F808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9F"
"A0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBFC0C1C2C3C4C5C6C7"
"C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
"F0F1F2F3F4F5F6F7F8F9FAFBFCFDFE";
	static const char base32Encoded[] =
"AAASEA2EAWDAQCAJBIFS2DIQB6IBCESVCSKTNF22DEPBYHA7D2RUAIJCENUCKJTHFAWUWK3NFWZC8NBT"
"GI3VIPJYG66DUQT5HS8V6R4AIFBEGTCFI3DWSUKKJPGE4VURKBIXEW4WKXMFQYC3MJPX2ZK8M7SGC2VD"
"NTUYN35IPFXGY5DPP3ZZA6MUQP4HK7VZRB6ZW856RX9H9AEBSKB2JBNGS8EIVCWMTUG27D6SUGJJHFEX"
"U4M3TGN4VQQJ5HW9WCS4FI7EWYVKRKFJXKX43MPQX82MDNXVYU45PP72ZG7MZRF7Z496BSQC2RCNMTYH"
"3DE6XU8N3ZHN9WGT4MJ7JXQY49NPVYY55VQ77Z9A6HTQH3HF65V8T4RK7RYQ55ZR8D29F69W8Z5RR8H3"
"9M7939R8";
	const char *base64AndHexEncoded =
"41414543417751464267634943516F4C4441304F4478415245684D554652595847426B6147787764"
"486838674953496A4A43556D4A7967704B6973734C5334764D4445794D7A51310A4E6A63344F546F"
"375044302B50304242516B4E4552555A4853456C4B5330784E546B395155564A5456465657563168"
"5A576C746358563566594746695932526C5A6D646F615770720A6247317562334278636E4E306458"
"5A3365486C3665337839666E2B4167594B44684957476834694A696F754D6A5936506B4A47536B35"
"53566C7065596D5A71626E4A32656E3643680A6F714F6B7061616E714B6D717136797472712B7773"
"624B7A744C573274376935757275387662362F774D484377385446787366497963724C7A4D334F7A"
"39445230745055316462580A324E6E6132397A6433742F6734654C6A354F586D352B6A7036757673"
"3765377638504879382F5431397666342B6672372F50332B0A";

	cout << "\nBase64, base32 and hex coding validation suite running...\n\n";

	fail = !TestFilter(HexEncoder().Ref(), data, 255, (const byte *)hexEncoded, strlen(hexEncoded));
	cout << (fail ? "FAILED    " : "passed    ");
	cout << "Hex Encoding\n";
	pass = pass && !fail;

	fail = !TestFilter(HexDecoder().Ref(), (const byte *)hexEncoded, strlen(hexEncoded), data, 255);
	cout << (fail ? "FAILED    " : "passed    ");
	cout << "Hex Decoding\n";
	pass = pass && !fail;

	fail = !TestFilter(Base32Encoder().Ref(), data, 255, (const byte *)base32Encoded, strlen(base32Encoded));
	cout << (fail ? "FAILED    " : "passed    ");
	cout << "Base32 Encoding\n";
	pass = pass && !fail;

	fail = !TestFilter(Base32Decoder().Ref(), (const byte *)base32Encoded, strlen(base32Encoded), data, 255);
	cout << (fail ? "FAILED    " : "passed    ");
	cout << "Base32 Decoding\n";
	pass = pass && !fail;

	fail = !TestFilter(Base64Encoder(new HexEncoder).Ref(), data, 255, (const byte *)base64AndHexEncoded, strlen(base64AndHexEncoded));
	cout << (fail ? "FAILED    " : "passed    ");
	cout << "Base64 Encoding\n";
	pass = pass && !fail;

	fail = !TestFilter(HexDecoder(new Base64Decoder).Ref(), (const byte *)base64AndHexEncoded, strlen(base64AndHexEncoded), data, 255);
	cout << (fail ? "FAILED    " : "passed    ");
	cout << "Base64 Decoding\n";
	pass = pass && !fail;

	return pass;
}

bool ValidateSHACAL2()
{
	cout << "\nSHACAL-2 validation suite running...\n\n";
	bool pass1 = true, pass2 = true, pass3 = true;

	SHACAL2Encryption enc;  // 128 to 512-bits (16 to 64-bytes)
	pass1 = enc.StaticGetValidKeyLength(8) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(15) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(64) == 64 && pass1;
	pass1 = enc.StaticGetValidKeyLength(65) == 64 && pass1;
	pass1 = enc.StaticGetValidKeyLength(128) == 64 && pass1;
	pass1 = enc.StaticGetValidKeyLength(0) == enc.MinKeyLength() && pass1;
	pass1 = enc.StaticGetValidKeyLength(SIZE_MAX) == enc.MaxKeyLength() && pass1;

	SHACAL2Decryption dec;  // 128 to 512-bits (16 to 64-bytes)
	pass2 = dec.StaticGetValidKeyLength(8) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(15) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(64) == 64 && pass2;
	pass2 = dec.StaticGetValidKeyLength(65) == 64 && pass2;
	pass2 = dec.StaticGetValidKeyLength(128) == 64 && pass2;
	pass2 = dec.StaticGetValidKeyLength(0) == dec.MinKeyLength() && pass2;
	pass2 = dec.StaticGetValidKeyLength(SIZE_MAX) == dec.MaxKeyLength() && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/shacal2v.dat", true, new HexDecoder);
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<SHACAL2Encryption, SHACAL2Decryption>(16), valdata, 4) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<SHACAL2Encryption, SHACAL2Decryption>(64), valdata, 10) && pass3;
	return pass1 && pass2 && pass3;
}

bool ValidateCamellia()
{
	cout << "\nCamellia validation suite running...\n\n";
	bool pass1 = true, pass2 = true, pass3 = true;

	CamelliaEncryption enc;
	pass1 = enc.StaticGetValidKeyLength(8) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(16) == 16 && pass1;
	pass1 = enc.StaticGetValidKeyLength(24) == 24 && pass1;
	pass1 = enc.StaticGetValidKeyLength(32) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(64) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(128) == 32 && pass1;
	pass1 = enc.StaticGetValidKeyLength(0) == enc.MinKeyLength() && pass1;
	pass1 = enc.StaticGetValidKeyLength(SIZE_MAX) == enc.MaxKeyLength() && pass1;

	CamelliaDecryption dec;
	pass2 = dec.StaticGetValidKeyLength(8) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(16) == 16 && pass2;
	pass2 = dec.StaticGetValidKeyLength(24) == 24 && pass2;
	pass2 = dec.StaticGetValidKeyLength(32) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(64) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(128) == 32 && pass2;
	pass2 = dec.StaticGetValidKeyLength(0) == dec.MinKeyLength() && pass2;
	pass2 = dec.StaticGetValidKeyLength(SIZE_MAX) == dec.MaxKeyLength() && pass2;
	cout << (pass1 && pass2 ? "passed:" : "FAILED:") << "  Algorithm key lengths\n";

	FileSource valdata(CRYPTOPP_DATA_DIR "TestData/camellia.dat", true, new HexDecoder);
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<CamelliaEncryption, CamelliaDecryption>(16), valdata, 15) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<CamelliaEncryption, CamelliaDecryption>(24), valdata, 15) && pass3;
	pass3 = BlockTransformationTest(FixedRoundsCipherFactory<CamelliaEncryption, CamelliaDecryption>(32), valdata, 15) && pass3;
	return pass1 && pass2 && pass3;
}

bool ValidateSalsa()
{
	cout << "\nSalsa validation suite running...\n";

	return RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/salsa.txt");
}

bool ValidateSosemanuk()
{
	cout << "\nSosemanuk validation suite running...\n";
	return RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/sosemanuk.txt");
}

bool ValidateVMAC()
{
	cout << "\nVMAC validation suite running...\n";
	return RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/vmac.txt");
}

bool ValidateCCM()
{
	cout << "\nAES/CCM validation suite running...\n";
	return RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/ccm.txt");
}

bool ValidateGCM()
{
	cout << "\nAES/GCM validation suite running...\n";
	cout << "\n2K tables:";
	bool pass = RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/gcm.txt", MakeParameters(Name::TableSize(), (int)2048));
	cout << "\n64K tables:";
	return RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/gcm.txt", MakeParameters(Name::TableSize(), (int)64*1024)) && pass;
}

bool ValidateCMAC()
{
	cout << "\nCMAC validation suite running...\n";
	return RunTestDataFile(CRYPTOPP_DATA_DIR "TestVectors/cmac.txt");
}
