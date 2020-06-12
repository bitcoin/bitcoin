// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_JSON_HPP
#define INTEGRATION_REFERENCE_BTC_JSON_HPP

#include <univalue.h>
#include <veriblock/json.hpp>

/// contains partial specialization of ToJSON, which allows to write
/// UniValue v = ToJSON<UniValue>(vbk entity);

namespace altintegration {

template <>
inline UniValue ToJSON(const std::string& t)
{
    return UniValue(t);
}

namespace json {

template <>
inline UniValue makeEmptyObject()
{
    return UniValue(UniValue::VOBJ);
}

template <>
inline UniValue makeEmptyArray()
{
    return UniValue(UniValue::VARR);
}

template <>
inline void putKV(UniValue& object,
    const std::string& key,
    const UniValue& val)
{
    object.pushKV(key, val);
}

template <>
inline void putStringKV(UniValue& object,
    const std::string& key,
    const std::string& value)
{
    object.pushKV(key, value);
}

template <>
inline void putIntKV(UniValue& object,
    const std::string& key,
    int64_t value)
{
    object.pushKV(key, value);
}

template <>
inline void putNullKV(UniValue& object, const std::string& key)
{
    object.pushKV(key, UniValue(UniValue::VNULL));
}

template <>
inline void arrayPushBack(UniValue& array, const UniValue& val)
{
    array.push_back(val);
}

template <>
inline void putBoolKV(UniValue& object,
    const std::string& key,
    bool value)
{
    object.pushKV(key, value);
}

} // namespace json
} // namespace altintegration

#endif //INTEGRATION_REFERENCE_BTC_JSON_HPP
