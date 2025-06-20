// Copyright 2014 BitPay Inc.
// Copyright 2015 Bitcoin Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UNIVALUE_INCLUDE_UNIVALUE_H
#define BITCOIN_UNIVALUE_INCLUDE_UNIVALUE_H

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

// NOLINTNEXTLINE(misc-no-recursion)
class UniValue {
public:
    enum VType { VNULL, VOBJ, VARR, VSTR, VNUM, VBOOL, };

    class type_error : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    UniValue() { typ = VNULL; }
    UniValue(UniValue::VType type, std::string str = {}) : typ{type}, val{std::move(str)} {}
    template <typename Ref, typename T = std::remove_cv_t<std::remove_reference_t<Ref>>,
              std::enable_if_t<std::is_floating_point_v<T> ||                      // setFloat
                                   std::is_same_v<bool, T> ||                      // setBool
                                   std::is_signed_v<T> || std::is_unsigned_v<T> || // setInt
                                   std::is_constructible_v<std::string, T>,        // setStr
                               bool> = true>
    UniValue(Ref&& val)
    {
        if constexpr (std::is_floating_point_v<T>) {
            setFloat(val);
        } else if constexpr (std::is_same_v<bool, T>) {
            setBool(val);
        } else if constexpr (std::is_signed_v<T>) {
            setInt(int64_t{val});
        } else if constexpr (std::is_unsigned_v<T>) {
            setInt(uint64_t{val});
        } else {
            setStr(std::string{std::forward<Ref>(val)});
        }
    }

    void clear();

    void setNull();
    void setBool(bool val);
    void setNumStr(std::string str);
    void setInt(uint64_t val);
    void setInt(int64_t val);
    void setInt(int val_) { return setInt(int64_t{val_}); }
    void setFloat(double val);
    void setStr(std::string str);
    void setArray();
    void setObject();

    enum VType getType() const { return typ; }
    const std::string& getValStr() const { return val; }
    bool empty() const { return (values.size() == 0); }

    size_t size() const { return values.size(); }

    void getObjMap(std::map<std::string,UniValue>& kv) const;
    bool checkObject(const std::map<std::string,UniValue::VType>& memberTypes) const;
    const UniValue& operator[](const std::string& key) const;
    const UniValue& operator[](size_t index) const;
    bool exists(const std::string& key) const { size_t i; return findKey(key, i); }

    bool isNull() const { return (typ == VNULL); }
    bool isTrue() const { return (typ == VBOOL) && (val == "1"); }
    bool isFalse() const { return (typ == VBOOL) && (val != "1"); }
    bool isBool() const { return (typ == VBOOL); }
    bool isStr() const { return (typ == VSTR); }
    bool isNum() const { return (typ == VNUM); }
    bool isArray() const { return (typ == VARR); }
    bool isObject() const { return (typ == VOBJ); }

    void push_back(UniValue val);
    void push_backV(const std::vector<UniValue>& vec);
    template <class It>
    void push_backV(It first, It last);

    void pushKVEnd(std::string key, UniValue val);
    void pushKV(std::string key, UniValue val);
    void pushKVs(UniValue obj);

    std::string write(unsigned int prettyIndent = 0,
                      unsigned int indentLevel = 0) const;

    bool read(std::string_view raw);

private:
    UniValue::VType typ;
    std::string val;                       // numbers are stored as C++ strings
    std::vector<std::string> keys;
    std::vector<UniValue> values;

    void checkType(const VType& expected) const;
    bool findKey(const std::string& key, size_t& retIdx) const;
    void writeArray(unsigned int prettyIndent, unsigned int indentLevel, std::string& s) const;
    void writeObject(unsigned int prettyIndent, unsigned int indentLevel, std::string& s) const;

public:
    // Strict type-specific getters, these throw std::runtime_error if the
    // value is of unexpected type
    const std::vector<std::string>& getKeys() const;
    const std::vector<UniValue>& getValues() const;
    template <typename Int>
    Int getInt() const;
    bool get_bool() const;
    const std::string& get_str() const;
    double get_real() const;
    const UniValue& get_obj() const;
    const UniValue& get_array() const;

    enum VType type() const { return getType(); }
    const UniValue& find_value(std::string_view key) const;
};

template <class It>
void UniValue::push_backV(It first, It last)
{
    checkType(VARR);
    values.insert(values.end(), first, last);
}

template <typename Int>
Int UniValue::getInt() const
{
    static_assert(std::is_integral_v<Int>);
    checkType(VNUM);
    Int result;
    const auto [first_nonmatching, error_condition] = std::from_chars(val.data(), val.data() + val.size(), result);
    if (first_nonmatching != val.data() + val.size() || error_condition != std::errc{}) {
        throw std::runtime_error("JSON integer out of range");
    }
    return result;
}

enum jtokentype {
    JTOK_ERR        = -1,
    JTOK_NONE       = 0,                           // eof
    JTOK_OBJ_OPEN,
    JTOK_OBJ_CLOSE,
    JTOK_ARR_OPEN,
    JTOK_ARR_CLOSE,
    JTOK_COLON,
    JTOK_COMMA,
    JTOK_KW_NULL,
    JTOK_KW_TRUE,
    JTOK_KW_FALSE,
    JTOK_NUMBER,
    JTOK_STRING,
};

extern enum jtokentype getJsonToken(std::string& tokenVal,
                                    unsigned int& consumed, const char *raw, const char *end);
extern const char *uvTypeName(UniValue::VType t);

static inline bool jsonTokenIsValue(enum jtokentype jtt)
{
    switch (jtt) {
    case JTOK_KW_NULL:
    case JTOK_KW_TRUE:
    case JTOK_KW_FALSE:
    case JTOK_NUMBER:
    case JTOK_STRING:
        return true;

    default:
        return false;
    }

    // not reached
}

static inline bool json_isspace(int ch)
{
    switch (ch) {
    case 0x20:
    case 0x09:
    case 0x0a:
    case 0x0d:
        return true;

    default:
        return false;
    }

    // not reached
}

extern const UniValue NullUniValue;

#endif // BITCOIN_UNIVALUE_INCLUDE_UNIVALUE_H
