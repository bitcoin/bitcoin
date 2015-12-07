#ifndef __MESSAGECRYPTER_H__
#define __MESSAGECRYPTER_H__
#include <string.h>
#include <string>
#include <vector>
using std::string;
#include "/usr/include/cryptopp/files.h"
using CryptoPP::FileSink;
using CryptoPP::FileSource;

#include "/usr/include/cryptopp/hex.h"
using CryptoPP::HexEncoder;

#include "/usr/include/cryptopp/filters.h"
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::PK_EncryptorFilter;
using CryptoPP::PK_DecryptorFilter;

#include "/usr/include/cryptopp/osrng.h"
using CryptoPP::AutoSeededRandomPool;

#include "/usr/include/cryptopp/integer.h"
using CryptoPP::Integer;

#include "/usr/include/cryptopp/pubkey.h"
using CryptoPP::PublicKey;
using CryptoPP::PrivateKey;

#include "/usr/include/cryptopp/eccrypto.h"
using CryptoPP::ECP;    // Prime field
using CryptoPP::EC2N;   // Binary field
using CryptoPP::ECIES;
using CryptoPP::ECPPoint;
using CryptoPP::DL_GroupParameters_EC;
using CryptoPP::DL_GroupPrecomputation;
using CryptoPP::DL_FixedBasePrecomputation;

#include "/usr/include/cryptopp/pubkey.h"
using CryptoPP::DL_PrivateKey_EC;
using CryptoPP::DL_PublicKey_EC;

#include "/usr/include/cryptopp/asn.h"
#include "/usr/include/cryptopp/oids.h"
namespace ASN1 = CryptoPP::ASN1;

#include "/usr/include/cryptopp/cryptlib.h"
using CryptoPP::PK_Encryptor;
using CryptoPP::PK_Decryptor;
using CryptoPP::g_nullNameValuePairs;
class CMessageCrypter
{

public:

    bool Encrypt(const string& vchPubKey, const string& vchPlaintext, string& vchCiphertext);
    bool Decrypt(const string& vchPrivKey, const string& vchCiphertext, string& vchPlaintext);

    CMessageCrypter()
    {

    }

    ~CMessageCrypter()
    {

    }
};
#endif
