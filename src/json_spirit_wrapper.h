#ifndef __JSON_SPIRIT_WRAPPER_H__
#define __JSON_SPIRIT_WRAPPER_H__

#include "univalue/univalue.h"

namespace json_spirit {

typedef UniValue Value;
typedef UniValue Array;
typedef UniValue Object;
typedef UniValue::VType Value_type;

}

#define find_value(val,key) (val[key])

#endif // __JSON_SPIRIT_WRAPPER_H__
