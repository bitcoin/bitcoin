// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_COMMON_H
#define BITCOIN_TEST_UTIL_COMMON_H

#include <addresstype.h>
#include <util/feefrac.h>

#include <chrono>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

struct AddressPosition;
class CAddress;
struct CExtKey;
struct CExtPubKey;
class CPubKey;
class CService;
struct KeyOriginInfo;
namespace txindex {
struct BlockTxPosition;
} // namespace txindex

/**
 * BOOST_CHECK_EXCEPTION predicates to check the specific validation error.
 * Use as
 * BOOST_CHECK_EXCEPTION(code that throws, exception type, HasReason("foo"));
 */
class HasReason
{
public:
    explicit HasReason(std::string_view reason) : m_reason(reason) {}
    bool operator()(std::string_view s) const { return s.find(m_reason) != std::string_view::npos; }
    bool operator()(const std::exception& e) const { return (*this)(e.what()); }

private:
    const std::string m_reason;
};

// Make types usable in BOOST_CHECK_* @{
namespace std {
template <typename Clock, typename Duration>
inline std::ostream& operator<<(std::ostream& os, const std::chrono::time_point<Clock, Duration>& tp)
{
    return os << tp.time_since_epoch().count();
}

template <typename T> requires std::is_enum_v<T>
inline std::ostream& operator<<(std::ostream& os, const T& e)
{
    return os << static_cast<std::underlying_type_t<T>>(e);
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::optional<T>& v)
{
    return v ? os << *v
             : os << "std::nullopt";
}
} // namespace std

template <typename T>
concept HasToString = requires(const T& t) { t.ToString(); };

template <HasToString T>
inline std::ostream& operator<<(std::ostream& os, const T& obj)
{
    return os << obj.ToString();
}

std::ostream& operator<<(std::ostream& os, const AddressPosition& pos);
std::ostream& operator<<(std::ostream& os, const CService& service);
std::ostream& operator<<(std::ostream& os, const CAddress& addr);
std::ostream& operator<<(std::ostream& os, const CExtKey& k);
std::ostream& operator<<(std::ostream& os, const CExtPubKey& k);
std::ostream& operator<<(std::ostream& os, const FeeFrac& ff);
std::ostream& operator<<(std::ostream& os, const CNoDestination& dest);
std::ostream& operator<<(std::ostream& os, const PubKeyDestination& dest);
std::ostream& operator<<(std::ostream& os, const PKHash& h);
std::ostream& operator<<(std::ostream& os, const ScriptHash& h);
std::ostream& operator<<(std::ostream& os, const WitnessV0KeyHash& h);
std::ostream& operator<<(std::ostream& os, const WitnessV0ScriptHash& h);
std::ostream& operator<<(std::ostream& os, const WitnessUnknown& w);
std::ostream& operator<<(std::ostream& os, const PayToAnchor& p);
std::ostream& operator<<(std::ostream& os, const WitnessV1Taproot& t);
std::ostream& operator<<(std::ostream& os, const CTxDestination& dest);
std::ostream& operator<<(std::ostream& os, const CPubKey& pk);
std::ostream& operator<<(std::ostream& os, const KeyOriginInfo& info);
std::ostream& operator<<(std::ostream& os, const ByRatioNegSize<FeeFrac>& b);

template <typename T, typename U>
    requires requires(std::ostream& os, const T& t, const U& u) { os << t; os << u; }
inline std::ostream& operator<<(std::ostream& os, const std::pair<T, U>& p)
{
    return os << "(" << p.first << ", " << p.second << ")";
}

namespace txindex {
std::ostream& operator<<(std::ostream& os, const BlockTxPosition& pos);
} // namespace txindex

// @}

#endif // BITCOIN_TEST_UTIL_COMMON_H
