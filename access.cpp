#include <openssl/sha.h>
#include <openssl/pem.h>

#include "access.h"

const char* ReadError::what() const throw()
{
    return "Problem reading BIO.";
}
const char* NoKeypairLoaded::what() const throw()
{
    return "No keypair loaded.";
}
const char* SignError::what() const throw()
{
    return "Error signing.";
}
const char* HashFailed::what() const throw()
{
    return "Unable to generate hash.";
}

AccessCard::AccessCard()
  : keypair(NULL)
{
    if (EVP_get_cipherbyname("aes-256-cbc") == NULL)
        OpenSSL_add_all_algorithms();
}
AccessCard::~AccessCard()
{
    RSA_free(keypair);
}
void AccessCard::CheckKey() const
{
    if (!keypair)
        throw NoKeypairLoaded();
}

void AccessCard::Generate()
{
    RSA_free(keypair);
    keypair = RSA_generate_key(2048, RSA_F4, NULL, NULL);
    CheckKey();
}

static char *LoseStringConst(const string& str)
{
    return const_cast<char*>(str.c_str());
}
static void* StringAsVoid(const string& str)
{
    return reinterpret_cast<void*>(LoseStringConst(str));
}
typedef unsigned char uchar;
template <typename T>
static T* CastString(const string& str)
{
    return reinterpret_cast<T*>(const_cast<char*>(str.c_str()));
}

BioBox::BioBox()
 : bio(NULL)
{
    buf.buf = NULL;
    buf.size = 0;
}
BioBox::~BioBox()
{
    BIO_free(bio);
    free(buf.buf);
}
void BioBox::ConstructSink(const string& str)
{
    BIO_free(bio);
    bio = BIO_new_mem_buf(StringAsVoid(str), -1);
    if (!bio)
        throw ReadError();
}
void BioBox::NewBuffer()
{
    BIO_free(bio);
    bio = BIO_new(BIO_s_mem());
    if (!bio)
        throw ReadError();
}
BIO* BioBox::Bio() const
{
    return bio;
}
BioBox::Buffer BioBox::ReadAll()
{
    buf.size = BIO_ctrl_pending(bio);
    buf.buf = malloc(buf.size);
    if (BIO_read(bio, buf.buf, buf.size) < 0) {
        //if (ERR_peek_error()) {
        //    ERR_reason_error_string(ERR_get_error());
        //    return NULL;
        //}
        throw ReadError();
    }
    return buf;
}
Base64::Base64()
{
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    NewBuffer();
    BIO_push(b64, bio);
}
Base64::~Base64()
{
    BIO_free(b64);
    bio = NULL;
}
Base64::Buffer Base64::Encode(const string& message)
{
    int ret;
    do {
        ret = BIO_write(b64, StringAsVoid(message), message.size());
        if (ret <= 0)
            if (BIO_should_retry(b64))
                continue;
            else
                throw ReadError();
    } while (ret <= 0);
    BIO_flush(b64);
    buf.size = BIO_get_mem_data(bio, &buf.buf);
    return buf;
}

EvpBox::EvpBox(RSA* keyp)
{
    evpkey = EVP_PKEY_new();
    if (!EVP_PKEY_set1_RSA(evpkey, keyp)) {
        throw ReadError();
    }
}
EvpBox::~EvpBox()
{
    EVP_PKEY_free(evpkey);
}
EVP_PKEY* EvpBox::Key()
{
    return evpkey;
}

static int pass_cb(char* buf, int size, int rwflag, void* u)
{
    const string pass = reinterpret_cast<char*>(u);
    int len = pass.size();
    // if too long, truncate
    if (len > size)
        len = size;
    pass.copy(buf, len);
    return len;
}
void AccessCard::Load(const string& pem, const string& pass)
{
    RSA_free(keypair);
    BioBox bio;
    bio.ConstructSink(pem);
    keypair = PEM_read_bio_RSAPrivateKey(bio.Bio(), NULL, pass_cb,
                                         StringAsVoid(pass));
    CheckKey();                     
}
bool AccessCard::IsLoaded() const
{
    return keypair != NULL;
}

void AccessCard::PublicKey(string& pem) const
{
    CheckKey();
    BioBox bio;
    bio.NewBuffer();
    int ret = PEM_write_bio_RSA_PUBKEY(bio.Bio(), keypair);
    if (!ret)
        throw ReadError();
    const BioBox::Buffer& buf = bio.ReadAll();
    pem = string(reinterpret_cast<const char*>(buf.buf), buf.size);
}

void AccessCard::PrivateKey(string& pem, const string& passphrase) const
{
    CheckKey();
    BioBox bio;
    bio.NewBuffer();

    EvpBox evp(keypair);
    int ret = PEM_write_bio_PKCS8PrivateKey(bio.Bio(), evp.Key(),
                                            EVP_aes_256_cbc(),
                                            LoseStringConst(passphrase),
                                            passphrase.size(), NULL, NULL);
    if (!ret)
        throw ReadError();
    const BioBox::Buffer& buf = bio.ReadAll();
    pem = string(reinterpret_cast<const char*>(buf.buf), buf.size);
}

static void Hash(const char* data, size_t datlen, uchar* digest)
{
    SHA512_CTX sha_ctx = { 0 };

    if (SHA512_Init(&sha_ctx) != 1)
        throw HashFailed();

    if (SHA512_Update(&sha_ctx, data, datlen) != 1)
        throw HashFailed();

    if (SHA512_Final(digest, &sha_ctx) != 1)
        throw HashFailed();
}

string AccessCard::Sign(const string& msg)
{
    uchar *sig = NULL;
    unsigned int sig_len = 0;

    uchar digest[SHA512_DIGEST_LENGTH];
    Hash((const char*)msg.c_str(), msg.size(), digest);

    sig = new uchar[RSA_size(keypair)];
    if (!sig)
        throw SignError();

    if (RSA_sign(NID_sha512, digest, sizeof digest, sig, &sig_len, keypair) != 1) {
        delete [] sig;
        throw SignError();
    }

    string sigret(reinterpret_cast<char*>(sig), sig_len);
    delete [] sig;

    // now base64 encode the signature
    Base64 b64;
    Base64::Buffer b = b64.Encode(sigret);
    string encsig(reinterpret_cast<char*>(b.buf), b.size);
    return encsig;
}
#if 0
bool AccessCard::Verify(const string& msg, const string& sig)
{
    //uchar* digest = Hash(msg.c_str(), msg.size());
    uchar* digest;
    uchar* sigbuf = CastString<uchar>(msg);
    const uchar* msgbuf = CastString<uchar>(msg);
    return RSA_verify(NID_sha512, msgbuf, msg.size(), sigbuf, sig.size(), keypair);
}
#endif

#ifdef TEST_ACCESS
//-------------------------------------------------------------------
// this wont be in the final file... it's our unit test
//-------------------------------------------------------------------
#include <iostream>
using std::cout;
// http://www.bmt-online.org/geekisms/RSA_verify

int main()
{
    void* pemarr = malloc(1900);
    FILE* pemf = fopen("privkey.pem", "r");
    size_t pemlen = fread(pemarr, sizeof(char), 1900, pemf);
    string pemstr(reinterpret_cast<char*>(pemarr), pemlen);

    AccessCard acc;                                                    
    acc.Load(pemstr, "hello");
    // New key
    //acc.Generate();
    string pem;
    // Get public key
    acc.PublicKey(pem);
    //cout << pem << "\n";
    // Get private key
    acc.PrivateKey(pem, "hello");
    //cout << pem << "\n";
    // Load a private key using pass 'hello'
    acc.PublicKey(pem);
    //cout << pem << "\n";
    string msg = "hello";

    string sign = acc.Sign("hello");
    std::cout << sign;
    return 0;
}
#endif

