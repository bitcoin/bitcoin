// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>

#include <iterator>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>

namespace util {
void ReplaceAll(std::string& in_out, const std::string& search, const std::string& substitute)
{
    if (search.empty()) return;
    in_out = std::regex_replace(in_out, std::regex(search), substitute);
}

LineReader::LineReader(std::string_view str, size_t max_line_length)
    : m_str{str}, m_max_line_length{max_line_length}, m_it{str.begin()} {}

std::optional<std::string_view> LineReader::ReadLine()
{
    if (m_it == m_str.end()) {
        return std::nullopt;
    }

    const auto line_start = m_it;
    while (m_it != m_str.end()) {
        // Read a character from the incoming buffer and increment the iterator
        const bool new_line{*m_it == '\n'};
        ++m_it;
        // If the character we just consumed was \n, the line is terminated.
        // The \n itself does not count against max_line_length.
        if (new_line) {
            std::string_view line{line_start, m_it - 1};
            if (!line.empty() && line.back() == '\r')
                line.remove_suffix(1);
            return line;
        }
        // If the character we just consumed gives us a line length greater
        // than max_line_length, and we are not at the end of the line (or buffer) yet,
        // that means the line we are currently reading is too long, and we throw.
        if (static_cast<size_t>(std::distance(line_start, m_it)) > m_max_line_length) {
            // Reset iterator
            m_it = line_start;
            throw std::runtime_error("max_line_length exceeded by LineReader");
        }
    }
    // End of buffer reached without finding a \n or exceeding max_line_length.
    // Reset the iterator so the rest of the buffer can be read granularly
    // with ReadLength() and return null to indicate a line was not found.
    m_it = line_start;
    return std::nullopt;
}

// Ignores max_line_length but won't overflow
std::string_view LineReader::ReadLength(size_t len)
{
    if (len == 0) return {};
    if (Remaining() < len) throw std::runtime_error("Not enough data in buffer");
    std::string_view out(std::to_address(m_it), len);
    m_it += len;
    return out;
}

size_t LineReader::Remaining() const
{
    return std::distance(m_it, m_str.end());
}

size_t LineReader::Consumed() const
{
    return std::distance(m_str.begin(), m_it);
}
} // namespace util
