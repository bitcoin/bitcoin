#include <sethash.h>

#include <openssl/sha.h>

void CSetHash::addRaw(const unsigned char *pch, size_t size) {
    uint64 buf[8];
    SHA512(pch, size, (unsigned char*)buf);
    for (int i=0; i<8; i++) {
        state[i] += buf[i];
    }
}

void CSetHash::removeRaw(const unsigned char *pch, size_t size) {
    uint64 buf[8];
    SHA512(pch, size, (unsigned char*)buf);
    for (int i=0; i<8; i++) {
        state[i] -= buf[i];
    }
}

uint256 CSetHash::GetHash() const {
    uint256 ret = 0;
    SHA256((unsigned char*)state, 64, (unsigned char*)&ret);
    return ret;
}
