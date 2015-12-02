// validat3.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include "cryptlib.h"
#include "pubkey.h"
#include "gfpcrypt.h"
#include "eccrypto.h"

#include "smartptr.h"
#include "crc.h"
#include "adler32.h"
#include "md2.h"
#include "md4.h"
#include "md5.h"
#include "sha.h"
#include "tiger.h"
#include "ripemd.h"
#include "whrlpool.h"
#include "hkdf.h"
#include "hmac.h"
#include "ttmac.h"
#include "integer.h"
#include "pwdbased.h"
#include "filters.h"
#include "files.h"
#include "hex.h"
#include "smartptr.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include "validate.h"

// Aggressive stack checking with VS2005 SP1 and above.
#if (CRYPTOPP_MSC_VERSION >= 1410)
# pragma strict_gs_check (on)
#endif

USING_NAMESPACE(CryptoPP)
USING_NAMESPACE(std)

struct HashTestTuple
{
	HashTestTuple(const char *input, const char *output, unsigned int repeatTimes=1)
		: input((byte *)input), output((byte *)output), inputLen(strlen(input)), repeatTimes(repeatTimes) {}
	
	HashTestTuple(const char *input, unsigned int inputLen, const char *output, unsigned int repeatTimes)
		: input((byte *)input), output((byte *)output), inputLen(inputLen), repeatTimes(repeatTimes) {}

	const byte *input, *output;
	size_t inputLen;
	unsigned int repeatTimes;
};

bool HashModuleTest(HashTransformation &md, const HashTestTuple *testSet, unsigned int testSetSize)
{
	bool pass=true, fail;
	SecByteBlock digest(md.DigestSize());

	// Coverity finding (http://stackoverflow.com/a/30968371 does not squash the finding)
	std::ostringstream out;
	out.copyfmt(cout);

	for (unsigned int i=0; i<testSetSize; i++)
	{
		unsigned j;

		for (j=0; j<testSet[i].repeatTimes; j++)
			md.Update(testSet[i].input, testSet[i].inputLen);
		md.Final(digest);
		fail = memcmp(digest, testSet[i].output, md.DigestSize()) != 0;
		pass = pass && !fail;

		out << (fail ? "FAILED   " : "passed   ");
		for (j=0; j<md.DigestSize(); j++)
			out << setw(2) << setfill('0') << hex << (int)digest[j];
		out << "   \"" << (char *)testSet[i].input << '\"';
		if (testSet[i].repeatTimes != 1)
			out << " repeated " << dec << testSet[i].repeatTimes << " times";
		out  << endl;
	}

	cout << out.str();
	return pass;
}

bool ValidateCRC32()
{
	HashTestTuple testSet[] = 
	{
		HashTestTuple("", "\x00\x00\x00\x00"),
		HashTestTuple("a", "\x43\xbe\xb7\xe8"),
		HashTestTuple("abc", "\xc2\x41\x24\x35"),
		HashTestTuple("message digest", "\x7f\x9d\x15\x20"),
		HashTestTuple("abcdefghijklmnopqrstuvwxyz", "\xbd\x50\x27\x4c"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "\xd2\xe6\xc2\x1f"),
		HashTestTuple("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "\x72\x4a\xa9\x7c"),
		HashTestTuple("123456789", "\x26\x39\xf4\xcb")
	};

	CRC32 crc;

	cout << "\nCRC-32 validation suite running...\n\n";
	return HashModuleTest(crc, testSet, sizeof(testSet)/sizeof(testSet[0]));
}

bool ValidateAdler32()
{
	HashTestTuple testSet[] = 
	{
		HashTestTuple("", "\x00\x00\x00\x01"),
		HashTestTuple("a", "\x00\x62\x00\x62"),
		HashTestTuple("abc", "\x02\x4d\x01\x27"),
		HashTestTuple("message digest", "\x29\x75\x05\x86"),
		HashTestTuple("abcdefghijklmnopqrstuvwxyz", "\x90\x86\x0b\x20"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "\x8a\xdb\x15\x0c"),
		HashTestTuple("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "\x15\xd8\x70\xf9", 15625)
	};

	Adler32 md;

	cout << "\nAdler-32 validation suite running...\n\n";
	return HashModuleTest(md, testSet, sizeof(testSet)/sizeof(testSet[0]));
}

bool ValidateMD2()
{
	HashTestTuple testSet[] = 
	{
		HashTestTuple("", "\x83\x50\xe5\xa3\xe2\x4c\x15\x3d\xf2\x27\x5c\x9f\x80\x69\x27\x73"),
		HashTestTuple("a", "\x32\xec\x01\xec\x4a\x6d\xac\x72\xc0\xab\x96\xfb\x34\xc0\xb5\xd1"),
		HashTestTuple("abc", "\xda\x85\x3b\x0d\x3f\x88\xd9\x9b\x30\x28\x3a\x69\xe6\xde\xd6\xbb"),
		HashTestTuple("message digest", "\xab\x4f\x49\x6b\xfb\x2a\x53\x0b\x21\x9f\xf3\x30\x31\xfe\x06\xb0"),
		HashTestTuple("abcdefghijklmnopqrstuvwxyz", "\x4e\x8d\xdf\xf3\x65\x02\x92\xab\x5a\x41\x08\xc3\xaa\x47\x94\x0b"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "\xda\x33\xde\xf2\xa4\x2d\xf1\x39\x75\x35\x28\x46\xc3\x03\x38\xcd"),
		HashTestTuple("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "\xd5\x97\x6f\x79\xd8\x3d\x3a\x0d\xc9\x80\x6c\x3c\x66\xf3\xef\xd8")
	};

	Weak::MD2 md2;

	cout << "\nMD2 validation suite running...\n\n";
	return HashModuleTest(md2, testSet, sizeof(testSet)/sizeof(testSet[0]));
}

bool ValidateMD4()
{
	HashTestTuple testSet[] = 
	{
		HashTestTuple("", "\x31\xd6\xcf\xe0\xd1\x6a\xe9\x31\xb7\x3c\x59\xd7\xe0\xc0\x89\xc0"),
		HashTestTuple("a", "\xbd\xe5\x2c\xb3\x1d\xe3\x3e\x46\x24\x5e\x05\xfb\xdb\xd6\xfb\x24"),
		HashTestTuple("abc", "\xa4\x48\x01\x7a\xaf\x21\xd8\x52\x5f\xc1\x0a\xe8\x7a\xa6\x72\x9d"),
		HashTestTuple("message digest", "\xd9\x13\x0a\x81\x64\x54\x9f\xe8\x18\x87\x48\x06\xe1\xc7\x01\x4b"),
		HashTestTuple("abcdefghijklmnopqrstuvwxyz", "\xd7\x9e\x1c\x30\x8a\xa5\xbb\xcd\xee\xa8\xed\x63\xdf\x41\x2d\xa9"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "\x04\x3f\x85\x82\xf2\x41\xdb\x35\x1c\xe6\x27\xe1\x53\xe7\xf0\xe4"),
		HashTestTuple("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "\xe3\x3b\x4d\xdc\x9c\x38\xf2\x19\x9c\x3e\x7b\x16\x4f\xcc\x05\x36")
	};

	Weak::MD4 md4;

	cout << "\nMD4 validation suite running...\n\n";
	return HashModuleTest(md4, testSet, sizeof(testSet)/sizeof(testSet[0]));
}

bool ValidateMD5()
{
	HashTestTuple testSet[] = 
	{
		HashTestTuple("", "\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e"),
		HashTestTuple("a", "\x0c\xc1\x75\xb9\xc0\xf1\xb6\xa8\x31\xc3\x99\xe2\x69\x77\x26\x61"),
		HashTestTuple("abc", "\x90\x01\x50\x98\x3c\xd2\x4f\xb0\xd6\x96\x3f\x7d\x28\xe1\x7f\x72"),
		HashTestTuple("message digest", "\xf9\x6b\x69\x7d\x7c\xb7\x93\x8d\x52\x5a\x2f\x31\xaa\xf1\x61\xd0"),
		HashTestTuple("abcdefghijklmnopqrstuvwxyz", "\xc3\xfc\xd3\xd7\x61\x92\xe4\x00\x7d\xfb\x49\x6c\xca\x67\xe1\x3b"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "\xd1\x74\xab\x98\xd2\x77\xd9\xf5\xa5\x61\x1c\x2c\x9f\x41\x9d\x9f"),
		HashTestTuple("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "\x57\xed\xf4\xa2\x2b\xe3\xc9\x55\xac\x49\xda\x2e\x21\x07\xb6\x7a")
	};

	Weak::MD5 md5;

	cout << "\nMD5 validation suite running...\n\n";
	return HashModuleTest(md5, testSet, sizeof(testSet)/sizeof(testSet[0]));
}

bool ValidateSHA()
{
	cout << "\nSHA validation suite running...\n\n";
	return RunTestDataFile("TestVectors/sha.txt");
}

bool ValidateSHA2()
{
	cout << "\nSHA validation suite running...\n\n";
	return RunTestDataFile("TestVectors/sha.txt");
}

bool ValidateTiger()
{
	cout << "\nTiger validation suite running...\n\n";

	HashTestTuple testSet[] =
	{
		HashTestTuple("", "\x32\x93\xac\x63\x0c\x13\xf0\x24\x5f\x92\xbb\xb1\x76\x6e\x16\x16\x7a\x4e\x58\x49\x2d\xde\x73\xf3"),
		HashTestTuple("abc", "\x2a\xab\x14\x84\xe8\xc1\x58\xf2\xbf\xb8\xc5\xff\x41\xb5\x7a\x52\x51\x29\x13\x1c\x95\x7b\x5f\x93"),
		HashTestTuple("Tiger", "\xdd\x00\x23\x07\x99\xf5\x00\x9f\xec\x6d\xeb\xc8\x38\xbb\x6a\x27\xdf\x2b\x9d\x6f\x11\x0c\x79\x37"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-", "\xf7\x1c\x85\x83\x90\x2a\xfb\x87\x9e\xdf\xe6\x10\xf8\x2c\x0d\x47\x86\xa3\xa5\x34\x50\x44\x86\xb5"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZ=abcdefghijklmnopqrstuvwxyz+0123456789", "\x48\xce\xeb\x63\x08\xb8\x7d\x46\xe9\x5d\x65\x61\x12\xcd\xf1\x8d\x97\x91\x5f\x97\x65\x65\x89\x57"),
		HashTestTuple("Tiger - A Fast New Hash Function, by Ross Anderson and Eli Biham", "\x8a\x86\x68\x29\x04\x0a\x41\x0c\x72\x9a\xd2\x3f\x5a\xda\x71\x16\x03\xb3\xcd\xd3\x57\xe4\xc1\x5e"),
		HashTestTuple("Tiger - A Fast New Hash Function, by Ross Anderson and Eli Biham, proceedings of Fast Software Encryption 3, Cambridge.", "\xce\x55\xa6\xaf\xd5\x91\xf5\xeb\xac\x54\x7f\xf8\x4f\x89\x22\x7f\x93\x31\xda\xb0\xb6\x11\xc8\x89"),
		HashTestTuple("Tiger - A Fast New Hash Function, by Ross Anderson and Eli Biham, proceedings of Fast Software Encryption 3, Cambridge, 1996.", "\x63\x1a\xbd\xd1\x03\xeb\x9a\x3d\x24\x5b\x6d\xfd\x4d\x77\xb2\x57\xfc\x74\x39\x50\x1d\x15\x68\xdd"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-", "\xc5\x40\x34\xe5\xb4\x3e\xb8\x00\x58\x48\xa7\xe0\xae\x6a\xac\x76\xe4\xff\x59\x0a\xe7\x15\xfd\x25")
	};

	Tiger tiger;

	return HashModuleTest(tiger, testSet, sizeof(testSet)/sizeof(testSet[0]));
}

bool ValidateRIPEMD()
{
	HashTestTuple testSet128[] = 
	{
		HashTestTuple("", "\xcd\xf2\x62\x13\xa1\x50\xdc\x3e\xcb\x61\x0f\x18\xf6\xb3\x8b\x46"),
		HashTestTuple("a", "\x86\xbe\x7a\xfa\x33\x9d\x0f\xc7\xcf\xc7\x85\xe7\x2f\x57\x8d\x33"),
		HashTestTuple("abc", "\xc1\x4a\x12\x19\x9c\x66\xe4\xba\x84\x63\x6b\x0f\x69\x14\x4c\x77"),
		HashTestTuple("message digest", "\x9e\x32\x7b\x3d\x6e\x52\x30\x62\xaf\xc1\x13\x2d\x7d\xf9\xd1\xb8"),
		HashTestTuple("abcdefghijklmnopqrstuvwxyz", "\xfd\x2a\xa6\x07\xf7\x1d\xc8\xf5\x10\x71\x49\x22\xb3\x71\x83\x4e"),
		HashTestTuple("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "\xa1\xaa\x06\x89\xd0\xfa\xfa\x2d\xdc\x22\xe8\x8b\x49\x13\x3a\x06"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "\xd1\xe9\x59\xeb\x17\x9c\x91\x1f\xae\xa4\x62\x4c\x60\xc5\xc7\x02"),
		HashTestTuple("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "\x3f\x45\xef\x19\x47\x32\xc2\xdb\xb2\xc4\xa2\xc7\x69\x79\x5f\xa3"),
		HashTestTuple("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "\x4a\x7f\x57\x23\xf9\x54\xeb\xa1\x21\x6c\x9d\x8f\x63\x20\x43\x1f", 15625)
	};

	HashTestTuple testSet160[] = 
	{
		HashTestTuple("", "\x9c\x11\x85\xa5\xc5\xe9\xfc\x54\x61\x28\x08\x97\x7e\xe8\xf5\x48\xb2\x25\x8d\x31"),
		HashTestTuple("a", "\x0b\xdc\x9d\x2d\x25\x6b\x3e\xe9\xda\xae\x34\x7b\xe6\xf4\xdc\x83\x5a\x46\x7f\xfe"),
		HashTestTuple("abc", "\x8e\xb2\x08\xf7\xe0\x5d\x98\x7a\x9b\x04\x4a\x8e\x98\xc6\xb0\x87\xf1\x5a\x0b\xfc"),
		HashTestTuple("message digest", "\x5d\x06\x89\xef\x49\xd2\xfa\xe5\x72\xb8\x81\xb1\x23\xa8\x5f\xfa\x21\x59\x5f\x36"),
		HashTestTuple("abcdefghijklmnopqrstuvwxyz", "\xf7\x1c\x27\x10\x9c\x69\x2c\x1b\x56\xbb\xdc\xeb\x5b\x9d\x28\x65\xb3\x70\x8d\xbc"),
		HashTestTuple("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "\x12\xa0\x53\x38\x4a\x9c\x0c\x88\xe4\x05\xa0\x6c\x27\xdc\xf4\x9a\xda\x62\xeb\x2b"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "\xb0\xe2\x0b\x6e\x31\x16\x64\x02\x86\xed\x3a\x87\xa5\x71\x30\x79\xb2\x1f\x51\x89"),
		HashTestTuple("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "\x9b\x75\x2e\x45\x57\x3d\x4b\x39\xf4\xdb\xd3\x32\x3c\xab\x82\xbf\x63\x32\x6b\xfb"),
		HashTestTuple("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "\x52\x78\x32\x43\xc1\x69\x7b\xdb\xe1\x6d\x37\xf9\x7f\x68\xf0\x83\x25\xdc\x15\x28", 15625)
	};

	HashTestTuple testSet256[] = 
	{
		HashTestTuple("", "\x02\xba\x4c\x4e\x5f\x8e\xcd\x18\x77\xfc\x52\xd6\x4d\x30\xe3\x7a\x2d\x97\x74\xfb\x1e\x5d\x02\x63\x80\xae\x01\x68\xe3\xc5\x52\x2d"),
		HashTestTuple("a", "\xf9\x33\x3e\x45\xd8\x57\xf5\xd9\x0a\x91\xba\xb7\x0a\x1e\xba\x0c\xfb\x1b\xe4\xb0\x78\x3c\x9a\xcf\xcd\x88\x3a\x91\x34\x69\x29\x25"),
		HashTestTuple("abc", "\xaf\xbd\x6e\x22\x8b\x9d\x8c\xbb\xce\xf5\xca\x2d\x03\xe6\xdb\xa1\x0a\xc0\xbc\x7d\xcb\xe4\x68\x0e\x1e\x42\xd2\xe9\x75\x45\x9b\x65"),
		HashTestTuple("message digest", "\x87\xe9\x71\x75\x9a\x1c\xe4\x7a\x51\x4d\x5c\x91\x4c\x39\x2c\x90\x18\xc7\xc4\x6b\xc1\x44\x65\x55\x4a\xfc\xdf\x54\xa5\x07\x0c\x0e"),
		HashTestTuple("abcdefghijklmnopqrstuvwxyz", "\x64\x9d\x30\x34\x75\x1e\xa2\x16\x77\x6b\xf9\xa1\x8a\xcc\x81\xbc\x78\x96\x11\x8a\x51\x97\x96\x87\x82\xdd\x1f\xd9\x7d\x8d\x51\x33"),
		HashTestTuple("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "\x38\x43\x04\x55\x83\xaa\xc6\xc8\xc8\xd9\x12\x85\x73\xe7\xa9\x80\x9a\xfb\x2a\x0f\x34\xcc\xc3\x6e\xa9\xe7\x2f\x16\xf6\x36\x8e\x3f"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "\x57\x40\xa4\x08\xac\x16\xb7\x20\xb8\x44\x24\xae\x93\x1c\xbb\x1f\xe3\x63\xd1\xd0\xbf\x40\x17\xf1\xa8\x9f\x7e\xa6\xde\x77\xa0\xb8"),
		HashTestTuple("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "\x06\xfd\xcc\x7a\x40\x95\x48\xaa\xf9\x13\x68\xc0\x6a\x62\x75\xb5\x53\xe3\xf0\x99\xbf\x0e\xa4\xed\xfd\x67\x78\xdf\x89\xa8\x90\xdd"),
		HashTestTuple("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "\xac\x95\x37\x44\xe1\x0e\x31\x51\x4c\x15\x0d\x4d\x8d\x7b\x67\x73\x42\xe3\x33\x99\x78\x82\x96\xe4\x3a\xe4\x85\x0c\xe4\xf9\x79\x78", 15625)
	};

	HashTestTuple testSet320[] = 
	{
		HashTestTuple("", "\x22\xd6\x5d\x56\x61\x53\x6c\xdc\x75\xc1\xfd\xf5\xc6\xde\x7b\x41\xb9\xf2\x73\x25\xeb\xc6\x1e\x85\x57\x17\x7d\x70\x5a\x0e\xc8\x80\x15\x1c\x3a\x32\xa0\x08\x99\xb8"),
		HashTestTuple("a", "\xce\x78\x85\x06\x38\xf9\x26\x58\xa5\xa5\x85\x09\x75\x79\x92\x6d\xda\x66\x7a\x57\x16\x56\x2c\xfc\xf6\xfb\xe7\x7f\x63\x54\x2f\x99\xb0\x47\x05\xd6\x97\x0d\xff\x5d"),
		HashTestTuple("abc", "\xde\x4c\x01\xb3\x05\x4f\x89\x30\xa7\x9d\x09\xae\x73\x8e\x92\x30\x1e\x5a\x17\x08\x5b\xef\xfd\xc1\xb8\xd1\x16\x71\x3e\x74\xf8\x2f\xa9\x42\xd6\x4c\xdb\xc4\x68\x2d"),
		HashTestTuple("message digest", "\x3a\x8e\x28\x50\x2e\xd4\x5d\x42\x2f\x68\x84\x4f\x9d\xd3\x16\xe7\xb9\x85\x33\xfa\x3f\x2a\x91\xd2\x9f\x84\xd4\x25\xc8\x8d\x6b\x4e\xff\x72\x7d\xf6\x6a\x7c\x01\x97"),
		HashTestTuple("abcdefghijklmnopqrstuvwxyz", "\xca\xbd\xb1\x81\x0b\x92\x47\x0a\x20\x93\xaa\x6b\xce\x05\x95\x2c\x28\x34\x8c\xf4\x3f\xf6\x08\x41\x97\x51\x66\xbb\x40\xed\x23\x40\x04\xb8\x82\x44\x63\xe6\xb0\x09"),
		HashTestTuple("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "\xd0\x34\xa7\x95\x0c\xf7\x22\x02\x1b\xa4\xb8\x4d\xf7\x69\xa5\xde\x20\x60\xe2\x59\xdf\x4c\x9b\xb4\xa4\x26\x8c\x0e\x93\x5b\xbc\x74\x70\xa9\x69\xc9\xd0\x72\xa1\xac"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "\xed\x54\x49\x40\xc8\x6d\x67\xf2\x50\xd2\x32\xc3\x0b\x7b\x3e\x57\x70\xe0\xc6\x0c\x8c\xb9\xa4\xca\xfe\x3b\x11\x38\x8a\xf9\x92\x0e\x1b\x99\x23\x0b\x84\x3c\x86\xa4"),
		HashTestTuple("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "\x55\x78\x88\xaf\x5f\x6d\x8e\xd6\x2a\xb6\x69\x45\xc6\xd2\xa0\xa4\x7e\xcd\x53\x41\xe9\x15\xeb\x8f\xea\x1d\x05\x24\x95\x5f\x82\x5d\xc7\x17\xe4\xa0\x08\xab\x2d\x42"),
		HashTestTuple("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "\xbd\xee\x37\xf4\x37\x1e\x20\x64\x6b\x8b\x0d\x86\x2d\xda\x16\x29\x2a\xe3\x6f\x40\x96\x5e\x8c\x85\x09\xe6\x3d\x1d\xbd\xde\xcc\x50\x3e\x2b\x63\xeb\x92\x45\xbb\x66", 15625)
	};

	bool pass = true;

	cout << "\nRIPEMD-128 validation suite running...\n\n";
	RIPEMD128 md128;
	pass = HashModuleTest(md128, testSet128, sizeof(testSet128)/sizeof(testSet128[0])) && pass;

	cout << "\nRIPEMD-160 validation suite running...\n\n";
	RIPEMD160 md160;
	pass = HashModuleTest(md160, testSet160, sizeof(testSet160)/sizeof(testSet160[0])) && pass;

	cout << "\nRIPEMD-256 validation suite running...\n\n";
	RIPEMD256 md256;
	pass = HashModuleTest(md256, testSet256, sizeof(testSet256)/sizeof(testSet256[0])) && pass;

	cout << "\nRIPEMD-320 validation suite running...\n\n";
	RIPEMD320 md320;
	pass = HashModuleTest(md320, testSet320, sizeof(testSet320)/sizeof(testSet320[0])) && pass;

	return pass;
}

#ifdef CRYPTOPP_REMOVED
bool ValidateHAVAL()
{
	HashTestTuple testSet[] = 
	{
		HashTestTuple("", "\xC6\x8F\x39\x91\x3F\x90\x1F\x3D\xDF\x44\xC7\x07\x35\x7A\x7D\x70"),
		HashTestTuple("a", "\x4D\xA0\x8F\x51\x4A\x72\x75\xDB\xC4\xCE\xCE\x4A\x34\x73\x85\x98\x39\x83\xA8\x30"),
		HashTestTuple("HAVAL", "\x0C\x13\x96\xD7\x77\x26\x89\xC4\x67\x73\xF3\xDA\xAC\xA4\xEF\xA9\x82\xAD\xBF\xB2\xF1\x46\x7E\xEA"),
		HashTestTuple("0123456789", "\xBE\xBD\x78\x16\xF0\x9B\xAE\xEC\xF8\x90\x3B\x1B\x9B\xC6\x72\xD9\xFA\x42\x8E\x46\x2B\xA6\x99\xF8\x14\x84\x15\x29"),
		HashTestTuple("abcdefghijklmnopqrstuvwxyz", "\xC9\xC7\xD8\xAF\xA1\x59\xFD\x9E\x96\x5C\xB8\x3F\xF5\xEE\x6F\x58\xAE\xDA\x35\x2C\x0E\xFF\x00\x55\x48\x15\x3A\x61\x55\x1C\x38\xEE"),
		HashTestTuple("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "\xB4\x5C\xB6\xE6\x2F\x2B\x13\x20\xE4\xF8\xF1\xB0\xB2\x73\xD4\x5A\xDD\x47\xC3\x21\xFD\x23\x99\x9D\xCF\x40\x3A\xC3\x76\x36\xD9\x63")
	};

	bool pass=true;

	cout << "\nHAVAL validation suite running...\n\n";
	{
		HAVAL3 md(16);
		pass = HashModuleTest(md, testSet+0, 1) && pass;
	}
	{
		HAVAL3 md(20);
		pass = HashModuleTest(md, testSet+1, 1) && pass;
	}
	{
		HAVAL4 md(24);
		pass = HashModuleTest(md, testSet+2, 1) && pass;
	}
	{
		HAVAL4 md(28);
		pass = HashModuleTest(md, testSet+3, 1) && pass;
	}
	{
		HAVAL5 md(32);
		pass = HashModuleTest(md, testSet+4, 1) && pass;
	}
	{
		HAVAL5 md(32);
		pass = HashModuleTest(md, testSet+5, 1) && pass;
	}

	return pass;
}
#endif

bool ValidatePanama()
{
	return RunTestDataFile("TestVectors/panama.txt");
}

bool ValidateWhirlpool()
{
	return RunTestDataFile("TestVectors/whrlpool.txt");
}

#ifdef CRYPTOPP_REMOVED
bool ValidateMD5MAC()
{
	const byte keys[2][MD5MAC::KEYLENGTH]={
		{0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff},
		{0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10}};

	const char *TestVals[7]={
		"",
		"a",
		"abc",
		"message digest",
		"abcdefghijklmnopqrstuvwxyz",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
		"12345678901234567890123456789012345678901234567890123456789012345678901234567890"};

	const byte output[2][7][MD5MAC::DIGESTSIZE]={
		{{0x1f,0x1e,0xf2,0x37,0x5c,0xc0,0xe0,0x84,0x4f,0x98,0xe7,0xe8,0x11,0xa3,0x4d,0xa8},
		{0x7a,0x76,0xee,0x64,0xca,0x71,0xef,0x23,0x7e,0x26,0x29,0xed,0x94,0x52,0x73,0x65},
		{0xe8,0x01,0x3c,0x11,0xf7,0x20,0x9d,0x13,0x28,0xc0,0xca,0xa0,0x4f,0xd0,0x12,0xa6},
		{0xc8,0x95,0x53,0x4f,0x22,0xa1,0x74,0xbc,0x3e,0x6a,0x25,0xa2,0xb2,0xef,0xd6,0x30},
		{0x91,0x72,0x86,0x7e,0xb6,0x00,0x17,0x88,0x4c,0x6f,0xa8,0xcc,0x88,0xeb,0xe7,0xc9},
		{0x3b,0xd0,0xe1,0x1d,0x5e,0x09,0x4c,0xb7,0x1e,0x35,0x44,0xac,0xa9,0xb8,0xbf,0xa2},
		{0x93,0x37,0x16,0x64,0x44,0xcc,0x95,0x35,0xb7,0xd5,0xb8,0x0f,0x91,0xe5,0x29,0xcb}},
		{{0x2f,0x6e,0x73,0x13,0xbf,0xbb,0xbf,0xcc,0x3a,0x2d,0xde,0x26,0x8b,0x59,0xcc,0x4d},
		{0x69,0xf6,0xca,0xff,0x40,0x25,0x36,0xd1,0x7a,0xe1,0x38,0x03,0x2c,0x0c,0x5f,0xfd},
		{0x56,0xd3,0x2b,0x6c,0x34,0x76,0x65,0xd9,0x74,0xd6,0xf7,0x5c,0x3f,0xc6,0xf0,0x40},
		{0xb8,0x02,0xb2,0x15,0x4e,0x59,0x8b,0x6f,0x87,0x60,0x56,0xc7,0x85,0x46,0x2c,0x0b},
		{0x5a,0xde,0xf4,0xbf,0xf8,0x04,0xbe,0x08,0x58,0x7e,0x94,0x41,0xcf,0x6d,0xbd,0x57},
		{0x18,0xe3,0x49,0xa5,0x24,0x44,0xb3,0x0e,0x5e,0xba,0x5a,0xdd,0xdc,0xd9,0xf1,0x8d},
		{0xf2,0xb9,0x06,0xa5,0xb8,0x4b,0x9b,0x4b,0xbe,0x95,0xed,0x32,0x56,0x4e,0xe7,0xeb}}};

	byte digest[MD5MAC::DIGESTSIZE];
	bool pass=true, fail;

	cout << "\nMD5MAC validation suite running...\n";

	for (int k=0; k<2; k++)
	{
		MD5MAC mac(keys[k]);
		cout << "\nKEY: ";
		for (int j=0;j<MD5MAC::KEYLENGTH;j++)
			cout << setw(2) << setfill('0') << hex << (int)keys[k][j];
		cout << endl << endl;
		for (int i=0;i<7;i++)
		{
			mac.Update((byte *)TestVals[i], strlen(TestVals[i]));
			mac.Final(digest);
			fail = memcmp(digest, output[k][i], MD5MAC::DIGESTSIZE)
				 || !mac.VerifyDigest(output[k][i], (byte *)TestVals[i], strlen(TestVals[i]));
			pass = pass && !fail;
			cout << (fail ? "FAILED   " : "passed   ");
			for (int j=0;j<MD5MAC::DIGESTSIZE;j++)
				cout << setw(2) << setfill('0') << hex << (int)digest[j];
			cout << "   \"" << TestVals[i] << '\"' << endl;
		}
	}

	return pass;
}
#endif

bool ValidateHMAC()
{
	return RunTestDataFile("TestVectors/hmac.txt");
}

#ifdef CRYPTOPP_REMOVED
bool ValidateXMACC()
{
	typedef XMACC<MD5> XMACC_MD5;

	const byte keys[2][XMACC_MD5::KEYLENGTH]={
		{0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb},
		{0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98}};

	const word32 counters[2]={0xccddeeff, 0x76543210};

	const char *TestVals[7]={
		"",
		"a",
		"abc",
		"message digest",
		"abcdefghijklmnopqrstuvwxyz",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
		"12345678901234567890123456789012345678901234567890123456789012345678901234567890"};

	const byte output[2][7][XMACC_MD5::DIGESTSIZE]={
		{{0xcc,0xdd,0xef,0x00,0xfa,0x89,0x54,0x92,0x86,0x32,0xda,0x2a,0x3f,0x29,0xc5,0x52,0xa0,0x0d,0x05,0x13},
		{0xcc,0xdd,0xef,0x01,0xae,0xdb,0x8b,0x7b,0x69,0x71,0xc7,0x91,0x71,0x48,0x9d,0x18,0xe7,0xdf,0x9d,0x5a},
		{0xcc,0xdd,0xef,0x02,0x5e,0x01,0x2e,0x2e,0x4b,0xc3,0x83,0x62,0xc2,0xf4,0xe6,0x18,0x1c,0x44,0xaf,0xca},
		{0xcc,0xdd,0xef,0x03,0x3e,0xa9,0xf1,0xe0,0x97,0x91,0xf8,0xe2,0xbe,0xe0,0xdf,0xf3,0x41,0x03,0xb3,0x5a},
		{0xcc,0xdd,0xef,0x04,0x2e,0x6a,0x8d,0xb9,0x72,0xe3,0xce,0x9f,0xf4,0x28,0x45,0xe7,0xbc,0x80,0xa9,0xc7},
		{0xcc,0xdd,0xef,0x05,0x1a,0xd5,0x40,0x78,0xfb,0x16,0x37,0xfc,0x7a,0x1d,0xce,0xb4,0x77,0x10,0xb2,0xa0},
		{0xcc,0xdd,0xef,0x06,0x13,0x2f,0x11,0x47,0xd7,0x1b,0xb5,0x52,0x36,0x51,0x26,0xb0,0x96,0xd7,0x60,0x81}},
		{{0x76,0x54,0x32,0x11,0xe9,0xcb,0x74,0x32,0x07,0x93,0xfe,0x01,0xdd,0x27,0xdb,0xde,0x6b,0x77,0xa4,0x56},
		{0x76,0x54,0x32,0x12,0xcd,0x55,0x87,0x5c,0xc0,0x35,0x85,0x99,0x44,0x02,0xa5,0x0b,0x8c,0xe7,0x2c,0x68},
		{0x76,0x54,0x32,0x13,0xac,0xfd,0x87,0x50,0xc3,0x8f,0xcd,0x58,0xaa,0xa5,0x7e,0x7a,0x25,0x63,0x26,0xd1},
		{0x76,0x54,0x32,0x14,0xe3,0x30,0xf5,0xdd,0x27,0x2b,0x76,0x22,0x7f,0xaa,0x90,0x73,0x6a,0x48,0xdb,0x00},
		{0x76,0x54,0x32,0x15,0xfc,0x57,0x00,0x20,0x7c,0x9d,0xf6,0x30,0x6f,0xbd,0x46,0x3e,0xfb,0x8a,0x2c,0x60},
		{0x76,0x54,0x32,0x16,0xfb,0x0f,0xd3,0xdf,0x4c,0x4b,0xc3,0x05,0x9d,0x63,0x1e,0xba,0x25,0x2b,0xbe,0x35},
		{0x76,0x54,0x32,0x17,0xc6,0xfe,0xe6,0x5f,0xb1,0x35,0x8a,0xf5,0x32,0x7a,0x80,0xbd,0xb8,0x72,0xee,0xae}}};

	byte digest[XMACC_MD5::DIGESTSIZE];
	bool pass=true, fail;

	cout << "\nXMACC/MD5 validation suite running...\n";

	for (int k=0; k<2; k++)
	{
		XMACC_MD5 mac(keys[k], counters[k]);
		cout << "\nKEY: ";
		for (int j=0;j<XMACC_MD5::KEYLENGTH;j++)
			cout << setw(2) << setfill('0') << hex << (int)keys[k][j];
		cout << "    COUNTER: 0x" << hex << counters[k] << endl << endl;
		for (int i=0;i<7;i++)
		{
			mac.Update((byte *)TestVals[i], strlen(TestVals[i]));
			mac.Final(digest);
			fail = memcmp(digest, output[k][i], XMACC_MD5::DIGESTSIZE)
				 || !mac.VerifyDigest(output[k][i], (byte *)TestVals[i], strlen(TestVals[i]));
			pass = pass && !fail;
			cout << (fail ? "FAILED   " : "passed   ");
			for (int j=0;j<XMACC_MD5::DIGESTSIZE;j++)
				cout << setw(2) << setfill('0') << hex << (int)digest[j];
			cout << "   \"" << TestVals[i] << '\"' << endl;
		}
	}

	return pass;
}
#endif

bool ValidateTTMAC()
{
	const byte key[TTMAC::KEYLENGTH]={
		0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,
		0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x01,0x23,0x45,0x67};

	const char *TestVals[8]={
		"",
		"a",
		"abc",
		"message digest",
		"abcdefghijklmnopqrstuvwxyz",
		"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
		"12345678901234567890123456789012345678901234567890123456789012345678901234567890"};

	const byte output[8][TTMAC::DIGESTSIZE]={
		{0x2d,0xec,0x8e,0xd4,0xa0,0xfd,0x71,0x2e,0xd9,0xfb,0xf2,0xab,0x46,0x6e,0xc2,0xdf,0x21,0x21,0x5e,0x4a},
		{0x58,0x93,0xe3,0xe6,0xe3,0x06,0x70,0x4d,0xd7,0x7a,0xd6,0xe6,0xed,0x43,0x2c,0xde,0x32,0x1a,0x77,0x56},
		{0x70,0xbf,0xd1,0x02,0x97,0x97,0xa5,0xc1,0x6d,0xa5,0xb5,0x57,0xa1,0xf0,0xb2,0x77,0x9b,0x78,0x49,0x7e},
		{0x82,0x89,0xf4,0xf1,0x9f,0xfe,0x4f,0x2a,0xf7,0x37,0xde,0x4b,0xd7,0x1c,0x82,0x9d,0x93,0xa9,0x72,0xfa},
		{0x21,0x86,0xca,0x09,0xc5,0x53,0x31,0x98,0xb7,0x37,0x1f,0x24,0x52,0x73,0x50,0x4c,0xa9,0x2b,0xae,0x60},
		{0x8a,0x7b,0xf7,0x7a,0xef,0x62,0xa2,0x57,0x84,0x97,0xa2,0x7c,0x0d,0x65,0x18,0xa4,0x29,0xe7,0xc1,0x4d},
		{0x54,0xba,0xc3,0x92,0xa8,0x86,0x80,0x6d,0x16,0x95,0x56,0xfc,0xbb,0x67,0x89,0xb5,0x4f,0xb3,0x64,0xfb},
		{0x0c,0xed,0x2c,0x9f,0x8f,0x0d,0x9d,0x03,0x98,0x1a,0xb5,0xc8,0x18,0x4b,0xac,0x43,0xdd,0x54,0xc4,0x84}};

	byte digest[TTMAC::DIGESTSIZE];
	bool pass=true, fail;

	cout << "\nTwo-Track-MAC validation suite running...\n";

	TTMAC mac(key, sizeof(key));
	for (unsigned int k=0; k<sizeof(TestVals)/sizeof(TestVals[0]); k++)
	{
		mac.Update((byte *)TestVals[k], strlen(TestVals[k]));
		mac.Final(digest);
		fail = memcmp(digest, output[k], TTMAC::DIGESTSIZE)
			|| !mac.VerifyDigest(output[k], (byte *)TestVals[k], strlen(TestVals[k]));
		pass = pass && !fail;
		cout << (fail ? "FAILED   " : "passed   ");
		for (int j=0;j<TTMAC::DIGESTSIZE;j++)
			cout << setw(2) << setfill('0') << hex << (int)digest[j];
		cout << "   \"" << TestVals[k] << '\"' << endl;
	}

	return true;
}

struct PBKDF_TestTuple
{
	byte purpose;
	unsigned int iterations;
	const char *hexPassword, *hexSalt, *hexDerivedKey;
};

bool TestPBKDF(PasswordBasedKeyDerivationFunction &pbkdf, const PBKDF_TestTuple *testSet, unsigned int testSetSize)
{
	bool pass = true;

	for (unsigned int i=0; i<testSetSize; i++)
	{
		const PBKDF_TestTuple &tuple = testSet[i];

		string password, salt, derivedKey;
		StringSource(tuple.hexPassword, true, new HexDecoder(new StringSink(password)));
		StringSource(tuple.hexSalt, true, new HexDecoder(new StringSink(salt)));
		StringSource(tuple.hexDerivedKey, true, new HexDecoder(new StringSink(derivedKey)));

		SecByteBlock derived(derivedKey.size());
		pbkdf.DeriveKey(derived, derived.size(), tuple.purpose, (byte *)password.data(), password.size(), (byte *)salt.data(), salt.size(), tuple.iterations);
		bool fail = memcmp(derived, derivedKey.data(), derived.size()) != 0;
		pass = pass && !fail;

		HexEncoder enc(new FileSink(cout));
		cout << (fail ? "FAILED   " : "passed   ");
		enc.Put(tuple.purpose);
		cout << " " << tuple.iterations;
		cout << " " << tuple.hexPassword << " " << tuple.hexSalt << " ";
		enc.Put(derived, derived.size());
		cout << endl;
	}

	return pass;
}

bool ValidatePBKDF()
{
	bool pass = true;

	{
	// from OpenSSL PKCS#12 Program FAQ v1.77, at http://www.drh-consultancy.demon.co.uk/test.txt
	PBKDF_TestTuple testSet[] = 
	{
		{1, 1, "0073006D006500670000", "0A58CF64530D823F", "8AAAE6297B6CB04642AB5B077851284EB7128F1A2A7FBCA3"},
		{2, 1, "0073006D006500670000", "0A58CF64530D823F", "79993DFE048D3B76"},
		{1, 1, "0073006D006500670000", "642B99AB44FB4B1F", "F3A95FEC48D7711E985CFE67908C5AB79FA3D7C5CAA5D966"},
		{2, 1, "0073006D006500670000", "642B99AB44FB4B1F", "C0A38D64A79BEA1D"},
		{3, 1, "0073006D006500670000", "3D83C0E4546AC140", "8D967D88F6CAA9D714800AB3D48051D63F73A312"},
		{1, 1000, "007100750065006500670000", "05DEC959ACFF72F7", "ED2034E36328830FF09DF1E1A07DD357185DAC0D4F9EB3D4"},
		{2, 1000, "007100750065006500670000", "05DEC959ACFF72F7", "11DEDAD7758D4860"},
		{1, 1000, "007100750065006500670000", "1682C0FC5B3F7EC5", "483DD6E919D7DE2E8E648BA8F862F3FBFBDC2BCB2C02957F"},
		{2, 1000, "007100750065006500670000", "1682C0FC5B3F7EC5", "9D461D1B00355C50"},
		{3, 1000, "007100750065006500670000", "263216FCC2FAB31C", "5EC4C7A80DF652294C3925B6489A7AB857C83476"}
	};

	PKCS12_PBKDF<SHA1> pbkdf;

	cout << "\nPKCS #12 PBKDF validation suite running...\n\n";
	pass = TestPBKDF(pbkdf, testSet, sizeof(testSet)/sizeof(testSet[0])) && pass;
	}

	{
	// from draft-ietf-smime-password-03.txt, at http://www.imc.org/draft-ietf-smime-password
	PBKDF_TestTuple testSet[] = 
	{
		{0, 5, "70617373776f7264", "1234567878563412", "D1DAA78615F287E6"},
		{0, 500, "416C6C206E2D656E746974696573206D75737420636F6D6D756E69636174652077697468206F74686572206E2d656E74697469657320766961206E2D3120656E746974656568656568656573", "1234567878563412","6A8970BF68C92CAEA84A8DF28510858607126380CC47AB2D"}
	};

	PKCS5_PBKDF2_HMAC<SHA1> pbkdf;

	cout << "\nPKCS #5 PBKDF2 validation suite running...\n\n";
	pass = TestPBKDF(pbkdf, testSet, sizeof(testSet)/sizeof(testSet[0])) && pass;
	}

	return pass;
}

struct HKDF_TestTuple
{
	const char *hexSecret, *hexSalt, *hexInfo, *hexExpected;
	size_t len;
};

bool TestHKDF(KeyDerivationFunction &kdf, const HKDF_TestTuple *testSet, unsigned int testSetSize)
{
	bool pass = true;

	for (unsigned int i=0; i<testSetSize; i++)
	{
		const HKDF_TestTuple &tuple = testSet[i];

		std::string secret, salt, info, expected;
		StringSource(tuple.hexSecret, true, new HexDecoder(new StringSink(secret)));
		StringSource(tuple.hexSalt ? tuple.hexSalt : "", true, new HexDecoder(new StringSink(salt)));
		StringSource(tuple.hexInfo ? tuple.hexInfo : "", true, new HexDecoder(new StringSink(info)));
		StringSource(tuple.hexExpected, true, new HexDecoder(new StringSink(expected)));

		SecByteBlock derived(expected.size());
		unsigned int ret = kdf.DeriveKey(derived, derived.size(),
                                         reinterpret_cast<const unsigned char*>(secret.data()), secret.size(),
                                         (tuple.hexSalt ? reinterpret_cast<const unsigned char*>(salt.data()) : NULL), salt.size(),
                                         (tuple.hexInfo ? reinterpret_cast<const unsigned char*>(info.data()) : NULL), info.size());

		bool fail = !VerifyBufsEqual(derived, reinterpret_cast<const unsigned char*>(expected.data()), derived.size());
		pass = pass && (ret == tuple.len) && !fail;

		HexEncoder enc(new FileSink(std::cout));
		std::cout << (fail ? "FAILED   " : "passed   ");
		std::cout << " " << tuple.hexSecret << " ";
		std::cout << (tuple.hexSalt ? (strlen(tuple.hexSalt) ? tuple.hexSalt : "<0-LEN SALT>") : "<NO SALT>");
		std::cout << " ";
		std::cout << (tuple.hexInfo ? (strlen(tuple.hexInfo) ? tuple.hexInfo : "<0-LEN INFO>") : "<NO INFO>");
		std::cout << " ";
		enc.Put(derived, derived.size());
		std::cout << std::endl;
	}

	return pass;
}

bool ValidateHKDF()
{
	bool pass = true;

	{
	// SHA-1 from RFC 5869, Appendix A, https://tools.ietf.org/html/rfc5869
	static const HKDF_TestTuple testSet[] =
	{
		// Test Case #4
		{"0b0b0b0b0b0b0b0b0b0b0b", "000102030405060708090a0b0c", "f0f1f2f3f4f5f6f7f8f9", "085a01ea1b10f36933068b56efa5ad81 a4f14b822f5b091568a9cdd4f155fda2 c22e422478d305f3f896", 42},
		// Test Case #5
		{"000102030405060708090a0b0c0d0e0f 101112131415161718191a1b1c1d1e1f 202122232425262728292a2b2c2d2e2f 303132333435363738393a3b3c3d3e3f 404142434445464748494a4b4c4d4e4f", "606162636465666768696a6b6c6d6e6f 707172737475767778797a7b7c7d7e7f 808182838485868788898a8b8c8d8e8f 909192939495969798999a9b9c9d9e9f a0a1a2a3a4a5a6a7a8a9aaabacadaeaf", "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf c0c1c2c3c4c5c6c7c8c9cacbcccdcecf d0d1d2d3d4d5d6d7d8d9dadbdcdddedf e0e1e2e3e4e5e6e7e8e9eaebecedeeef f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff", "0bd770a74d1160f7c9f12cd5912a06eb ff6adcae899d92191fe4305673ba2ffe 8fa3f1a4e5ad79f3f334b3b202b2173c 486ea37ce3d397ed034c7f9dfeb15c5e 927336d0441f4c4300e2cff0d0900b52 d3b4", 82},
		// Test Case #6
		{"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b", "", "", "0ac1af7002b3d761d1e55298da9d0506 b9ae52057220a306e07b6b87e8df21d0 ea00033de03984d34918", 42},
		// Test Case #7		                                                            
		{"0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c", NULL, "", "2c91117204d745f3500d636a62f64f0 ab3bae548aa53d423b0d1f27ebba6f5e5 673a081d70cce7acfc48", 42}
	};

	HKDF<SHA1> hkdf;

	std::cout << "\nRFC 5869 HKDF(SHA-1) validation suite running...\n\n";
	pass = TestHKDF(hkdf, testSet, COUNTOF(testSet)) && pass;
	}

	{
	// SHA-256 from RFC 5869, Appendix A, https://tools.ietf.org/html/rfc5869
	static const HKDF_TestTuple testSet[] =
	{
		// Test Case #1
		{"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b", "000102030405060708090a0b0c", "f0f1f2f3f4f5f6f7f8f9", "3cb25f25faacd57a90434f64d0362f2a 2d2d0a90cf1a5a4c5db02d56ecc4c5bf 34007208d5b887185865", 42},
		// Test Case #2
		{"000102030405060708090a0b0c0d0e0f 101112131415161718191a1b1c1d1e1f 202122232425262728292a2b2c2d2e2f 303132333435363738393a3b3c3d3e3f 404142434445464748494a4b4c4d4e4f", "606162636465666768696a6b6c6d6e6f 707172737475767778797a7b7c7d7e7f 808182838485868788898a8b8c8d8e8f 909192939495969798999a9b9c9d9e9f a0a1a2a3a4a5a6a7a8a9aaabacadaeaf", "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf c0c1c2c3c4c5c6c7c8c9cacbcccdcecf d0d1d2d3d4d5d6d7d8d9dadbdcdddedf e0e1e2e3e4e5e6e7e8e9eaebecedeeef f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff", "b11e398dc80327a1c8e7f78c596a4934 4f012eda2d4efad8a050cc4c19afa97c 59045a99cac7827271cb41c65e590e09 da3275600c2f09b8367793a9aca3db71 cc30c58179ec3e87c14c01d5c1f3434f 1d87", 82},
		// Test Case #3
		{"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b", "", "", "8da4e775a563c18f715f802a063c5a31 b8a11f5c5ee1879ec3454e5f3c738d2d 9d201395faa4b61a96c8", 42}
	};

	HKDF<SHA256> hkdf;

	std::cout << "\nRFC 5869 HKDF(SHA-256) validation suite running...\n\n";
	pass = TestHKDF(hkdf, testSet, COUNTOF(testSet)) && pass;
	}

	{
	// SHA-512, Crypto++ generated, based on RFC 5869, https://tools.ietf.org/html/rfc5869
	static const HKDF_TestTuple testSet[] =
	{
		// Test Case #0
		{"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b", "000102030405060708090a0b0c", "f0f1f2f3f4f5f6f7f8f9", "832390086CDA71FB47625BB5CEB168E4 C8E26A1A16ED34D9FC7FE92C14815793 38DA362CB8D9F925D7CB", 42},
		// Test Case #0
		{"000102030405060708090a0b0c0d0e0f 101112131415161718191a1b1c1d1e1f 202122232425262728292a2b2c2d2e2f 303132333435363738393a3b3c3d3e3f 404142434445464748494a4b4c4d4e4f", "606162636465666768696a6b6c6d6e6f 707172737475767778797a7b7c7d7e7f 808182838485868788898a8b8c8d8e8f 909192939495969798999a9b9c9d9e9f a0a1a2a3a4a5a6a7a8a9aaabacadaeaf", "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf c0c1c2c3c4c5c6c7c8c9cacbcccdcecf d0d1d2d3d4d5d6d7d8d9dadbdcdddedf e0e1e2e3e4e5e6e7e8e9eaebecedeeef f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff", "CE6C97192805B346E6161E821ED16567 3B84F400A2B514B2FE23D84CD189DDF1 B695B48CBD1C8388441137B3CE28F16A A64BA33BA466B24DF6CFCB021ECFF235 F6A2056CE3AF1DE44D572097A8505D9E 7A93", 82},
		// Test Case #0
		{"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b", "", "", "F5FA02B18298A72A8C23898A8703472C 6EB179DC204C03425C970E3B164BF90F FF22D04836D0E2343BAC", 42},
		// Test Case #0
		{"0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c", NULL, "", "1407D46013D98BC6DECEFCFEE55F0F90 B0C7F63D68EB1A80EAF07E953CFC0A3A 5240A155D6E4DAA965BB", 42}
	};

	HKDF<SHA512> hkdf;

	std::cout << "\nRFC 5869 HKDF(SHA-512) validation suite running...\n\n";
	pass = TestHKDF(hkdf, testSet, COUNTOF(testSet)) && pass;
	}



	{
	// Whirlpool, Crypto++ generated, based on RFC 5869, https://tools.ietf.org/html/rfc5869
	static const HKDF_TestTuple testSet[] =
	{
		// Test Case #0
		{"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b", "000102030405060708090a0b0c", "f0f1f2f3f4f5f6f7f8f9", "0D29F74CCD8640F44B0DD9638111C1B5 766EFED752AF358109E2E7C9CD4A28EF 2F90B2AD461FBA0744D4", 42},
		// Test Case #0
		{"000102030405060708090a0b0c0d0e0f 101112131415161718191a1b1c1d1e1f 202122232425262728292a2b2c2d2e2f 303132333435363738393a3b3c3d3e3f 404142434445464748494a4b4c4d4e4f", "606162636465666768696a6b6c6d6e6f 707172737475767778797a7b7c7d7e7f 808182838485868788898a8b8c8d8e8f 909192939495969798999a9b9c9d9e9f a0a1a2a3a4a5a6a7a8a9aaabacadaeaf", "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf c0c1c2c3c4c5c6c7c8c9cacbcccdcecf d0d1d2d3d4d5d6d7d8d9dadbdcdddedf e0e1e2e3e4e5e6e7e8e9eaebecedeeef f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff", "4EBE4FE2DCCEC42661699500BE279A99 3FED90351E19373B3926FAA3A410700B2 BBF77E254CF1451AE6068D64A0904D96 6F4FF25498445A501B88F50D21E3A68A8 90E09445DC5886DD00E7F4F7C58A5121 70", 82},
		// Test Case #0
		{"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b", "", "", "110632D0F7AEFAC31771FC66C22BB346 2614B81E4B04BA7F2B662E0BD694F564 58615F9A9CB56C57ECF2", 42},
		// Test Case #0
		{"0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c" /*key*/, NULL /*salt*/, "" /*info*/, "4089286EBFB23DD8A02F0C9DAA35D538 EB09CD0A8CBAB203F39083AA3E0BD313 E6F91E64F21A187510B0", 42}
	};

	HKDF<Whirlpool> hkdf;

	std::cout << "\nRFC 5869 HKDF(Whirlpool) validation suite running...\n\n";
	pass = TestHKDF(hkdf, testSet, COUNTOF(testSet)) && pass;
	}
	
	
	return pass;
}
