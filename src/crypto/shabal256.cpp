#include <crypto/shabal256.h>

#include <crypto/shabal/sph_shabal.h>

CShabal256::CShabal256()
{
    cc = new sph_shabal256_context;
    ::sph_shabal256_init(cc);
}

CShabal256::~CShabal256()
{
    delete (sph_shabal256_context*)cc;
}

CShabal256& CShabal256::Write(const unsigned char* data, size_t len)
{
    ::sph_shabal256(cc, (const void*)data, len);
    return *this;
}

void CShabal256::Finalize(unsigned char hash[OUTPUT_SIZE])
{
    ::sph_shabal256_close(cc, hash);
}

CShabal256& CShabal256::Reset()
{
    ::sph_shabal256_init(cc);
    return *this;
}
