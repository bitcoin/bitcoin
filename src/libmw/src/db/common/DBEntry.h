#pragma once

#include <mw/common/Traits.h>
#include <string>
#include <memory>

template<typename T,
    typename SFINAE = typename std::enable_if_t<std::is_base_of<Traits::ISerializable, T>::value>>
struct DBEntry
{
    DBEntry(const std::string& _key, const std::shared_ptr<const T>& _item)
        : key(_key), item(_item) { }

    DBEntry(const std::string& _key, T _item)
        : key(_key), item(std::make_shared<const T>(std::move(_item))) { }

    std::string key;
    std::shared_ptr<const T> item;
};