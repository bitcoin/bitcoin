// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/help.h"

#include <vector>
#include <sstream>
#include <algorithm>

static const std::string spaces = "                                             \
                                                                                \
                                                                                ";

inline std::string lpad(uint8_t width, const std::string& s)
{
    size_t len = s.size();
    return len >= width ? s : s + spaces.substr(0, width - len);
}

std::string Tabulated(const std::string content)
{
    std::vector<std::string> v;
    uint8_t maxWidth = 0;
    size_t pos = 0;
    size_t i = content.find('\n');
    while (i != std::string::npos) {
        std::string line = content.substr(pos, i - pos);
        std::string left = line;
        std::string right = "";
        size_t tabpoint = line.find(" || ");
        if (tabpoint != std::string::npos) {
            left = line.substr(0, tabpoint);
            right = line.substr(tabpoint + 4);
            size_t trimpos = left.find_last_not_of(" ");
            if (trimpos != std::string::npos) {
                left = left.substr(0, 1 + trimpos);
            }
            maxWidth = std::max(maxWidth, (uint8_t)left.length());
        }
        v.push_back(left);
        v.push_back(right);
        pos = i + 1;
        i = content.find('\n', pos);
    }

    std::stringstream ss;
    for (size_t j = 0; j + 1 < v.size(); j += 2) {
        if (v[j + 1].length() == 0)
            ss << v[j] << std::endl;
        else
            ss << lpad(maxWidth, v[j]) << " " << v[j + 1] << std::endl;
    }
    return ss.str();
}
