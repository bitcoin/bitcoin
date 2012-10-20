#ifndef _SETHASH_H_
#define _SETHASH_H_ 1

#include "serialize.h"
#include "uint256.h"
#include "version.h"

class CSetHash
{
private:
    uint64 state[8];

protected:
    void addRaw(const unsigned char *pch, size_t size);
    void removeRaw(const unsigned char *pch, size_t size);

public:
    CSetHash() {
        memset(state, 0, 64);
    }

    template<typename D> void add(const D& input) {
        CDataStream ss(SER_DISK, CLIENT_VERSION);
        ss << input;
        addRaw((const unsigned char*)&ss[0], ss.size());
    }
    template<typename D> void remove(const D& input) {
        CDataStream ss(SER_DISK, CLIENT_VERSION);
        ss << input;
        removeRaw((const unsigned char*)&ss[0], ss.size());
    }

    uint256 GetHash() const;
};

#endif
