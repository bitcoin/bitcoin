// Copyright (c) 2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_UTIL_REF_H
#define WIDECOIN_UTIL_REF_H

#include <util/check.h>

#include <typeindex>

namespace util {

/**
 * Type-safe dynamic reference.
 *
 * This implements a small subset of the functionality in C++17's std::any
 * class, and can be dropped when the project updates to C++17
 * (https://github.com/widecoin/widecoin/issues/16684)
 */
class Ref
{
public:
    Ref() = default;
    template<typename T> Ref(T& value) { Set(value); }
    template<typename T> T& Get() const { CHECK_NONFATAL(Has<T>()); return *static_cast<T*>(m_value); }
    template<typename T> void Set(T& value) { m_value = &value; m_type = std::type_index(typeid(T)); }
    template<typename T> bool Has() const { return m_value && m_type == std::type_index(typeid(T)); }
    void Clear() { m_value = nullptr; m_type = std::type_index(typeid(void)); }

private:
    void* m_value = nullptr;
    std::type_index m_type = std::type_index(typeid(void));
};

} // namespace util

#endif // WIDECOIN_UTIL_REF_H
