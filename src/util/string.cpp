// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>

#include <regex>
#include <string>

namespace util {
void ReplaceAll(std::string& in_out, const std::string& search, const std::string& substitute)
{
    if (search.empty()) return;
    in_out = std::regex_replace(in_out, std::regex(search), substitute);
}

LineReader::LineReader(std::span<const std::byte> buffer, size_t max_read)
    : start(buffer.begin()), end(buffer.end()), max_read(max_read), it(buffer.begin()) {}

std::optional<std::string> LineReader::ReadLine()
{
    if (it == end) {
        return std::nullopt;
    }

    auto line_start = it;
    size_t count = 0;
    while (it != end) {
        char c = static_cast<char>(*it);
        ++it;
        ++count;
        if (c == '\n') break;
        if (count >= max_read) throw std::runtime_error("max_read exceeded by LineReader");
    }
    const std::string_view untrimmed_line(reinterpret_cast<const char *>(&*line_start), count);
    const std::string_view line = TrimStringView(untrimmed_line); // delete trailing \r and/or \n
    return std::string(line);
}

// Ignores max_read but won't overflow
std::string LineReader::ReadLength(size_t len)
{
    if (len == 0) return "";
    if (Left() < len) throw std::runtime_error("Not enough data in buffer");
    std::string out(reinterpret_cast<const char*>(&(*it)), len);
    it += len;
    return out;
}

size_t LineReader::Left() const
{
    return std::distance(it, end);
}
} // namespace util
