#ifndef __BITCOIN_EXTSERVICES_H__
#define __BITCOIN_EXTSERVICES_H__

#include <stdint.h>
#include <string>
#include <vector>
#include "json/json_spirit_value.h"
#include "serialize.h"

class CKeyValue {
public:
    std::string key;
    std::string value;

    CKeyValue()
    {
        SetNull();
    }
    CKeyValue(std::string key_, std::string value_)
    {
        key = key_;
	value = value_;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(key);
        READWRITE(value);
    )

    void SetNull()
    {
        key.clear();
        value.clear();
    }
};

class CExtService {
public:
    std::string name;
    int32_t port;
    std::vector<CKeyValue> attrib;

    CExtService(const std::string& name_ = "", int32_t port_ = -1) {
        name = name_;
        port = port_;
    }
    bool parseValue(const json_spirit::Value& val);

    IMPLEMENT_SERIALIZE
    (
        READWRITE(name);
        READWRITE(port);
        READWRITE(attrib);
    )
};

extern bool ReadExtServices();
extern json_spirit::Array ListExtServices();
extern void GetExtServicesVec(std::vector<CExtService>& vec);

#endif // __BITCOIN_EXTSERVICES_H__
