#include <string>
#include <exception>
using std::string;

//typedef struct RSA;
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

class AccessCard
{
public:
    AccessCard();
    ~AccessCard();
    void Generate();
    void Load(const string& pem, const string& pass);
    void PublicKey(string& pem) const;
    void PrivateKey(string& pem, const string& passphrase) const;
    string Sign(const string& msg);
    bool Verify(const string& msg, const string& sig);

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

//--------------------
#include <openssl/pem.h>

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
    return "No keypair loaded.";
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
string AccessCard::Sign(const string& msg)
{
    const uchar* m = reinterpret_cast<const uchar*>(msg.c_str());
    uchar* sigret = new uchar[RSA_size(keypair)];
    unsigned int siglen = 0;
    if (!RSA_sign(NID_sha512, m, msg.size(), sigret, &siglen, keypair)) {
        delete[] sigret;
        throw SignError();
    }
    string sig(reinterpret_cast<const char*>(sigret), siglen);
    delete[] sigret;
    return sig;
}
bool AccessCard::Verify(const string& msg, const string& sig)
{
    const uchar* m = reinterpret_cast<const uchar*>(msg.c_str());
    uchar* sigbuf = reinterpret_cast<uchar*>(LoseStringConst(sig));
    return RSA_verify(NID_sha512, m, msg.size(), sigbuf, sig.size(), keypair);
}

//#ifdef TESTACC
//-------------------------------------------------------------------
// this wont be in the final file... it's our unit test
//-------------------------------------------------------------------
#include <iostream>
#include <assert.h>
using std::cout;

int main()
{
    AccessCard acc;                                                    
    // New key
    acc.Generate();
    string pem;
    // Get public key
    acc.PublicKey(pem);
    cout << pem << "\n";
    // Get private key
    acc.PrivateKey(pem, "hello");
    cout << pem << "\n";
    // Load a private key using pass 'hello'
    acc.Load(pem, "hello");
    acc.PublicKey(pem);
    cout << pem << "\n";
    string msg = "hellothere";
    const string sig = acc.Sign(msg);
    cout << acc.Verify(msg, sig) << "\n";
    Base64 bio;
    const BioBox::Buffer& buf = bio.Encode(sig);
    pem = string(reinterpret_cast<const char*>(buf.buf), buf.size);
    cout << "signature = " << pem << "\n";
    msg = "hellother";
    cout << acc.Verify(msg, sig) << "\n";
    return 0;
}
//#endif

