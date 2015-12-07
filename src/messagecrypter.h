#ifndef __MESSAGECRYPTER_H__
#define __MESSAGECRYPTER_H__
#include <string.h>
#include <string>
#include <vector>
using std::string;
#include <crypto++/files.h>
using CryptoPP::FileSink;
using CryptoPP::FileSource;

#include <crypto++/hex.h>
using CryptoPP::HexEncoder;

#include <crypto++/filters.h>
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::PK_EncryptorFilter;
using CryptoPP::PK_DecryptorFilter;

#include <crypto++/osrng.h>
using CryptoPP::AutoSeededRandomPool;

#include <crypto++/integer.h>
using CryptoPP::Integer;

#include <crypto++/pubkey.h>
using CryptoPP::PublicKey;
using CryptoPP::PrivateKey;

#include <crypto++/eccrypto.h>
using CryptoPP::ECP;    // Prime field
using CryptoPP::EC2N;   // Binary field
using CryptoPP::ECIES;
using CryptoPP::ECPPoint;
using CryptoPP::DL_GroupParameters_EC;
using CryptoPP::DL_GroupPrecomputation;
using CryptoPP::DL_FixedBasePrecomputation;

#include <crypto++/pubkey.h>
using CryptoPP::DL_PrivateKey_EC;
using CryptoPP::DL_PublicKey_EC;

#include <crypto++/asn.h>
#include <crypto++/oids.h>
namespace ASN1 = CryptoPP::ASN1;

#include <crypto++/cryptlib.h>
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
