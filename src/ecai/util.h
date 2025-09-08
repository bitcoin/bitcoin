#ifndef BITCOIN_ECAI_UTIL_H
#define BITCOIN_ECAI_UTIL_H
#include <string>
#include <vector>

inline bool ParseHexStr(const std::string& hex, std::vector<unsigned char>& out)
{
    out.clear();
    if (hex.size() % 2) return false;
    out.reserve(hex.size()/2);
    for (size_t i=0;i<hex.size();i+=2) {
        unsigned int b;
        if (sscanf(hex.c_str()+i, "%2x", &b)!=1) return false;
        out.push_back((unsigned char)b);
    }
    return true;
}

#endif
