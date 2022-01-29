#pragma once

#include "DBEntry.h"

#include <memory>
#include <string>

class DBTable
{
public:
    DBTable(const char prefix)
        : m_prefix(prefix) { }

    std::string BuildKey(const std::string& itemKey) const noexcept { return m_prefix + itemKey; }

    template<typename T,
        typename SFINAE = typename std::enable_if_t<std::is_base_of<Traits::ISerializable, T>::value>>
    std::string BuildKey(const DBEntry<T>& item) const noexcept { return BuildKey(item.key); }

    char GetPrefix() const noexcept { return m_prefix; }

private:
    char m_prefix;
};