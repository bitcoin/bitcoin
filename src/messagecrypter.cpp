#include "messagecrypter.h"
bool CMessageCrypter::Encrypt(const string& vchPubKey, const string& strPlaintext, string& strCiphertext)
{
    try
    {
        AutoSeededRandomPool prng;
        StringSource ss(vchPubKey, true);
		ECIES<ECP>::Encryptor encryptor;

        //curve used is secp256k1
        encryptor.AccessKey().AccessGroupParameters().Initialize(ASN1::secp256k1());

        //get point on the used curve
        ECP::Point point;
        encryptor.GetKey().GetGroupParameters().GetCurve().DecodePoint(point, ss, ss.MaxRetrievable());

        //set encryptor's public element
        encryptor.AccessKey().SetPublicElement(point);

        //check whether the encryptor's access key thus formed is valid or not
        encryptor.AccessKey().ThrowIfInvalid(prng, 3);

        // encrypted message
        StringSource ss1(strPlaintext, true, new PK_EncryptorFilter(prng, encryptor, new StringSink(strCiphertext) ) );
    }
    catch(const CryptoPP::Exception& ex)
    {
		return false;
    }

	return true;
}

bool CMessageCrypter::Decrypt(const string& vchPrivKey, const string& strCiphertext, string& strPlaintext)
{
    try
    {
        AutoSeededRandomPool prng;

        StringSource ss(vchPrivKey, true /*pumpAll*/);

        Integer x;
        x.Decode(ss, ss.MaxRetrievable(), Integer::UNSIGNED);

        ECIES<ECP>::Decryptor decryptor;

		decryptor.AccessKey().AccessGroupParameters().Initialize(ASN1::secp256k1());	
        //make decryptor's access key using decoded private exponent's value
        decryptor.AccessKey().SetPrivateExponent(x);

        //check whether decryptor's access key is valid or not
        bool valid = decryptor.AccessKey().Validate(prng, 3);
        if(!valid)
           decryptor.AccessKey().ThrowIfInvalid(prng, 3);

        //decrypt the message using private key
        StringSource ss2 (strCiphertext, true, new PK_DecryptorFilter(prng, decryptor, new StringSink(strPlaintext) ) );

    }
    catch(const CryptoPP::Exception& ex)
    {
		return false;
    }
    return true;
}
