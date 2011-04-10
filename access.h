#ifndef __ACCESS_H__
#define __ACCESS_H__

#include <string>
#include <exception>
using std::string;

#include <openssl/rsa.h>

class ReadError : public std::exception
{
public:
    virtual const char* what() const throw();
};

class NoKeypairLoaded : public std::exception
{
public:
    virtual const char* what() const throw();
};

class SignError : public std::exception
{
public:
    virtual const char* what() const throw();
};

class HashFailed : public std::exception
{
public:
    virtual const char* what() const throw();
};

class AccessCard
{
public:
    AccessCard();
    ~AccessCard();
    // new private key
    void Generate();
    // load a private key in PEM format
    void Load(const string& pem, const string& pass);
    void PublicKey(string& pem) const;
    void PrivateKey(string& pem, const string& passphrase) const;
    // sign a message and return it base64 encoded.
    string Sign(const string& msg);

private:
    void CheckKey() const;

    RSA* keypair;
};

class BioBox
{
public:
    struct Buffer
    {
        void* buf;
        int size;
    };

    BioBox();
    ~BioBox();
    void ConstructSink(const string& str);
    void NewBuffer();
    BIO* Bio() const;
    Buffer ReadAll();
protected:
    BIO* bio;
    Buffer buf;
};

class Base64 : public BioBox
{
public:
    Base64();
    ~Base64();
    Buffer Encode(const string& message);
private:
    BIO* b64;
};

class EvpBox
{
public:
    EvpBox(RSA* keyp);
    ~EvpBox();
    EVP_PKEY* Key();
private:
    EVP_PKEY* evpkey;
};

#endif

