// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_HTTP_H
#define BITCOIN_COMMON_HTTP_H

#include <util/strencodings.h>

#include <optional>
#include <string>
#include <unordered_map>

class HTTPHeaders
{
public:
    std::optional<std::string> Find(const std::string& key) const;
    void Write(const std::string& key, const std::string& value);
    void Remove(const std::string& key);
    /*
     * @returns false if LineReader hits the end of the buffer before reading an
     *                \n, meaning that we are still waiting on more data from the client.
     *          true  after reading an entire HTTP headers section, terminated
     *                by an empty line and \n.
     * @throws on exceeded read limit and on bad headers syntax (e.g. no ":" in a line)
     */
    bool Read(util::LineReader& reader);
    std::string Stringify() const;

private:
    std::unordered_map<std::string, std::string, util::AsciiCaseInsensitiveHash, util::AsciiCaseInsensitiveKeyEqual> m_map;
};

#endif // BITCOIN_COMMON_HTTP_H
