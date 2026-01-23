// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/http.h>

#include <util/string.h>

#include <stdexcept>
#include <string>
#include <string_view>

std::optional<std::string> HTTPHeaders::Find(const std::string& key) const
{
    const auto it = m_map.find(key);
    if (it == m_map.end()) return std::nullopt;
    return it->second;
}

void HTTPHeaders::Write(const std::string& key, const std::string& value)
{
    // If present, append value to list
    const auto existing_value = Find(key);
    if (existing_value) {
        m_map[key] = existing_value.value() + ", " + value;
    } else {
        m_map[key] = value;
    }
}

void HTTPHeaders::Remove(const std::string& key)
{
    m_map.erase(key);
}

bool HTTPHeaders::Read(util::LineReader& reader)
{
    // Headers https://httpwg.org/specs/rfc9110.html#rfc.section.6.3
    // A sequence of Field Lines https://httpwg.org/specs/rfc9110.html#rfc.section.5.2
    do {
        auto maybe_line = reader.ReadLine();
        if (!maybe_line) return false;
        const std::string& line = *maybe_line;

        // An empty line indicates end of the headers section https://www.rfc-editor.org/rfc/rfc2616#section-4
        if (line.length() == 0) break;

        // Header line must have at least one ":"
        // keys are not allowed to have delimiters like ":" but values are
        // https://httpwg.org/specs/rfc9110.html#rfc.section.5.6.2
        const size_t pos{line.find(':')};
        if (pos == std::string::npos) throw std::runtime_error("HTTP header missing colon (:)");

        // Whitespace is optional
        std::string key = util::TrimString(std::string_view(line).substr(0, pos));
        std::string value = util::TrimString(std::string_view(line).substr(pos + 1));
        Write(key, value);
    } while (true);

    return true;
}

std::string HTTPHeaders::Stringify() const
{
    std::string out;
    for (const auto& [key, value] : m_map) {
        out += key + ": " + value + "\r\n";
    }

    // Headers are terminated by an empty line
    out += "\r\n";

    return out;
}
