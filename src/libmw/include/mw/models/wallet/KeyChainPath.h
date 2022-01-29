#pragma once

// Copyright (c) 2018-2020 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Traits.h>
#include <mw/exceptions/DeserializationException.h>
#include <cstdint>
#include <string>
#include <vector>

static const uint32_t MINIMUM_HARDENED_INDEX = 0x80000000;

class KeyChainPath : public Traits::IPrintable
{
public:
    KeyChainPath(std::vector<uint32_t> keyIndices)
        : m_keyIndices(std::move(keyIndices)) { }

    //
    // Operators
    //
    bool operator<(const KeyChainPath& other) const { return GetKeyIndices() < other.GetKeyIndices(); }
    bool operator==(const KeyChainPath& other) const { return GetKeyIndices() == other.GetKeyIndices(); }
    bool operator!=(const KeyChainPath& other) const { return GetKeyIndices() != other.GetKeyIndices(); }

    const std::vector<uint32_t>& GetKeyIndices() const { return m_keyIndices; }

    /// <summary>
    /// Example format: m/0/2'/500
    /// </summary>
    /// <returns>The formatted BIP32 keychain path.</returns>
    std::string Format() const final
    {
        std::string path = "m";
        for (const uint32_t keyIndex : m_keyIndices) {
            path += "/";
            
            if (keyIndex < MINIMUM_HARDENED_INDEX) {
                path += std::to_string(keyIndex);
            } else {
                path += std::to_string(keyIndex - MINIMUM_HARDENED_INDEX) + "'";
            }
        }

        return path;
    }

    static KeyChainPath FromString(const std::string& path)
    {
        if (path.empty() || path.at(0) != 'm') {
            ThrowDeserialization_F("Invalid path: {}", path);
        }

        std::vector<uint32_t> indices;

        std::vector<std::string> str_indices = StringUtil::Split(path, "/");
        for (size_t i = 1; i < str_indices.size(); i++) {
            uint32_t key_index = 0;

            std::string str_index = str_indices[i];
            if (str_index.empty()) {
                ThrowDeserialization_F("Invalid path: {}", path);
            }

            if (str_index.back() == '\'') {
                key_index = MINIMUM_HARDENED_INDEX;
                str_index = str_index.substr(0, str_index.size() - 1);
            }

            key_index += std::stoul(str_index);
            indices.push_back(key_index);
        }

        return KeyChainPath(std::move(indices));
    }

    KeyChainPath GetFirstChild() const
    {
        std::vector<uint32_t> keyIndicesCopy = m_keyIndices;
        keyIndicesCopy.push_back(0);
        return KeyChainPath(std::move(keyIndicesCopy));
    }

    KeyChainPath GetChild(const uint32_t childIndex) const
    {
        std::vector<uint32_t> keyIndicesCopy = m_keyIndices;
        keyIndicesCopy.push_back(childIndex);
        return KeyChainPath(std::move(keyIndicesCopy));
    }

    KeyChainPath GetNextSibling() const
    {
        assert(!m_keyIndices.empty());
        std::vector<uint32_t> keyIndicesCopy = m_keyIndices;
        keyIndicesCopy.back()++;
        return KeyChainPath(std::move(keyIndicesCopy));
    }

private:
    std::vector<uint32_t> m_keyIndices;
};