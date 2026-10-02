// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include "pq_shield_core.h"

#include <cstdio>

static int Usage()
{
    std::fprintf(stderr, "Usage: pq-shield-audit <pub|secret:path>\n");
    return 1;
}

int main(int argc, char** argv)
{
    if (argc != 2) return Usage();
    std::string arg = argv[1];
    auto pos = arg.find(':');
    if (pos == std::string::npos) return Usage();
    std::string kind = arg.substr(0, pos);
    std::string path = arg.substr(pos + 1);
    bool expect_secret = (kind == "secret");
    if (kind != "secret" && kind != "pub") return Usage();
    std::string err;
    if (!pq_shield::AuditFile(path, expect_secret, err)) {
        std::fprintf(stderr, "audit FAILED: %s\n", err.c_str());
        return 1;
    }
    std::printf("audit OK\n");
    return 0;
}
