// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include "pq_shield_core.h"

#include <cstdio>
#include <cstring>
#include <string>

using namespace pq_shield;

static int Usage()
{
    std::fprintf(stderr,
        "Usage:\n"
        "  pq-shield keygen <dir>           Generate keypair into <dir>/pq.pub, <dir>/pq.key\n"
        "  pq-shield sign <key> <msg> <sig>\n"
        "  pq-shield verify <pub> <msg> <sig>\n");
    return 1;
}

int main(int argc, char** argv)
{
    if (argc < 2) return Usage();
    std::string cmd = argv[1];

    if (cmd == "keygen" && argc == 3) {
        std::vector<uint8_t> pk, sk;
        if (!Keygen(pk, sk)) { std::fprintf(stderr, "keygen failed\n"); return 1; }
        std::string dir = argv[2];
        if (!WriteFile(dir + "/pq.pub", pk, false) || !WriteFile(dir + "/pq.key", sk, true)) {
            std::fprintf(stderr, "write failed\n"); return 1;
        }
        std::printf("generated %s pair in %s/\n", ALG, dir.c_str());
        return 0;
    }
    if (cmd == "sign" && argc == 5) {
        std::vector<uint8_t> sk, msg, sig;
        if (!ReadFile(argv[2], sk) || !ReadFile(argv[3], msg)) return 1;
        if (!Sign(sig, msg, sk) || !WriteFile(argv[4], sig, false)) return 1;
        std::printf("signed (%zu bytes)\n", sig.size());
        return 0;
    }
    if (cmd == "verify" && argc == 5) {
        std::vector<uint8_t> pk, msg, sig;
        if (!ReadFile(argv[2], pk) || !ReadFile(argv[3], msg) || !ReadFile(argv[4], sig)) return 1;
        if (!Verify(msg, sig, pk)) { std::fprintf(stderr, "INVALID signature\n"); return 1; }
        std::printf("valid signature\n");
        return 0;
    }
    return Usage();
}
