// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <fs.h>
#include <util/asmap.h>
#include <util/strencodings.h>
#include <netaddress.h>
#include <netbase.h>
#include <streams.h>
#include <serialize.h>
#include <tinyformat.h>

#include <iostream>
#include <memory>
#include <stdio.h>
#include <string>
#include <vector>

namespace {

bool LoadFile(const std::string& arg, std::vector<bool>& bits) {
    FILE *filestr = fsbridge::fopen(arg, "rb");
    CAutoFile file(filestr, SER_DISK, 0);
    if (file.IsNull()) {
        tfm::format(std::cerr, "Failed to open asmap file '%s'\n", arg);
        return false;
    }
    fseek(filestr, 0, SEEK_END);
    int length = ftell(filestr);
    fseek(filestr, 0, SEEK_SET);

    bits.clear();
    bits.reserve(8 * length);
    char cur_byte;
    for (int i = 0; i < length; ++i) {
        file >> cur_byte;
        for (int bit = 0; bit < 8; ++bit) {
            bits.push_back((cur_byte >> bit) & 1);
        }
    }

    if (!SanityCheckASMap(bits)) {
        tfm::format(std::cerr, "Provided asmap file '%s' is invalid\n", arg);
        return false;
    }

    return true;
}

bool SaveFile(const std::string& arg, const std::vector<bool>& bits) {
    FILE *filestr = fsbridge::fopen(arg, "wb");
    CAutoFile file(filestr, SER_DISK, 0);
    if (file.IsNull()) {
        tfm::format(std::cerr, "Failed to open asmap file '%s'\n", arg);
        return false;
    }
    uint8_t cur_byte = 0;
    for (size_t i = 0; i < bits.size(); ++i) {
        cur_byte |= (bits[i] << (i & 7));
        if ((i & 7) == 7) {
            file << cur_byte;
            cur_byte = 0;
        }
    }
    if (bits.size() & 7) file << cur_byte;

    return true;
}

bool ExecuteDecode(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        tfm::format(std::cerr, "Incorrect number of arguments provided to decode (needs exactly 1)\n");
        return false;
    }

    std::vector<bool> bits;
    if (!LoadFile(args[0], bits)) return false;

    for (const auto& item : DecodeASMap(bits)) {
        tfm::format(std::cout, "%s AS%lu\n", item.first.ToString(), item.second);
    }
    return true;
}

bool ExecuteLookup(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        tfm::format(std::cerr, "Incorrect number of arguments provide to lookup (needs at least 2)\n");
        return false;
    }

    std::vector<bool> bits;
    if (!LoadFile(args[0], bits)) return false;

    std::vector<std::pair<CNetAddr, uint32_t>> results;
    for (size_t i = 1; i < args.size(); ++i) {
        CNetAddr addr;
        if (!LookupHost(args[i], addr, false)) {
            tfm::format(std::cerr, "Cannot parse IP address '%s'\n", args[i]);
            return false;
        }
        uint32_t asn = addr.GetMappedAS(bits);
        if (asn != 0) results.emplace_back(addr, asn);
    }

    for (const auto& item : results) {
        tfm::format(std::cout, "%s AS%lu\n", item.first.ToString(), item.second);
    }

    return true;
}

bool ExecuteEncode(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        tfm::format(std::cerr, "Incorrect number of arguments provided to encode (needs exactly 1)\n");
        return false;
    }

    std::vector<std::pair<CSubNet, uint32_t>> mapping;
    for (std::string line; std::getline(std::cin, line);) {
        size_t pos = line.find(" AS");
        if (pos == std::string::npos) {
            tfm::format(std::cerr, "Cannot parse '%s'\n", line.c_str());
            return false;
        }
        CSubNet subnet;
        std::string subnetstr(line.begin(), line.begin() + pos);
        if (!LookupSubNet(subnetstr, subnet) || subnet.GetCIDR().second < 0) {
            tfm::format(std::cerr, "Invalid subnet '%s'\n", subnetstr);
            return false;
        }
        uint32_t asn;
        std::string asnstr(line.begin() + pos +3, line.end());
        if (!ParseUInt32(asnstr, &asn) || asn == 0 || asn == 0xFFFFFFFF) {
            tfm::format(std::cerr, "Invalid ASN 'AS%s'\n", asnstr);
            return false;
        }
        mapping.emplace_back(subnet, asn);
    }

    auto asmap = EncodeASMap(mapping);
    return SaveFile(args[0], asmap);
}

}

int main(int argc, char** argv)
{
    std::vector<std::string> args;
    while (*argv != nullptr) args.emplace_back(*(argv++));

    if (args.size() >= 2 && args[1] == "decode") {
        return !ExecuteDecode(std::vector<std::string>(args.begin() + 2, args.end()));
    } else if (args.size() >= 2 && args[1] == "encode") {
        return !ExecuteEncode(std::vector<std::string>(args.begin() + 2, args.end()));
    } else if (args.size() >= 2 && args[1] == "lookup") {
        return !ExecuteLookup(std::vector<std::string>(args.begin() + 2, args.end()));
    } else {
        tfm::format(std::cerr, "Usage: %s decode <file>          Decode provided asmap file to stdout\n", args[0]);
        tfm::format(std::cerr, "       %s encode <file>          Encode stdin into provided file\n", args[0]);
        tfm::format(std::cerr, "       %s lookup <file> <IP>...  Look up ASN for provided IPs in file\n", args[0]);
        tfm::format(std::cerr, "       %s help                   Print this message\n", args[0]);
        return (args.size() != 2 || args[1] == "help");
    }

    return 0;
}
