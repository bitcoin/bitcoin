// Copyright (c) 2019-present The Bitcoin Core developers
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

LineReader::LineReader(std::span<const std::byte> buffer, size_t max_line_length)
    : start(buffer.begin()), end(buffer.end()), max_line_length(max_line_length), it(buffer.begin()) {}

std::optional<std::string> LineReader::ReadLine()
{
    if (it == end) {
        return std::nullopt;
    }

    auto line_start = it;
    size_t count = 0;
    while (it != end) {
        // Read a character from the incoming buffer and increment the iterator
        auto c = static_cast<char>(*it);
        ++it;
        ++count;
        // If the character we just consumed was \n, the line is terminated.
        // The \n itself does not count against max_line_length.
        if (c == '\n') {
            const std::string_view untrimmed_line(reinterpret_cast<const char*>(std::to_address(line_start)), count);
            const std::string_view line = TrimStringView(untrimmed_line); // delete leading and trailing whitespace including \r and \n
            return std::string(line);
        }
        // If the character we just consumed gives us a line length greater
        // than max_line_length, and we are not at the end of the line (or buffer) yet,
        // that means the line we are currently reading is too long, and we throw.
        if (count > max_line_length) {
            // Reset iterator
            it = line_start;
            throw std::runtime_error("max_line_length exceeded by LineReader");
        }
    }
    // End of buffer reached without finding a \n or exceeding max_line_length.
    // Reset the iterator so the rest of the buffer can be read granularly
    // with ReadLength() and return null to indicate a line was not found.
    it = line_start;
    return std::nullopt;
}

// Ignores max_line_length but won't overflow
std::string LineReader::ReadLength(size_t len)
{
    if (len == 0) return "";
    if (Remaining() < len) throw std::runtime_error("Not enough data in buffer");
    std::string out(reinterpret_cast<const char*>(std::to_address(it)), len);
    it += len;
    return out;
}

size_t LineReader::Remaining() const
{
    return std::distance(it, end);
}
} // namespace util
