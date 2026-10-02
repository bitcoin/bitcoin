// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include "pq_shield_core.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

using namespace pq_shield;

int main()
{
    // Round-trip
    std::vector<uint8_t> pk, sk;
    assert(Keygen(pk, sk));
    std::string msg_str = "bitcoin: pq migration test vector";
    std::vector<uint8_t> msg(msg_str.begin(), msg_str.end());
    std::vector<uint8_t> sig;
    assert(Sign(sig, msg, sk));
    assert(Verify(msg, sig, pk));

    // Tamper resistance
    auto bad = sig;
    bad[0] ^= 0x01;
    assert(!Verify(msg, bad, pk));
    auto bad_msg = msg;
    bad_msg[0] ^= 0x01;
    assert(!Verify(bad_msg, sig, pk));

    // Permissions on secret keys
    char tmpl[] = "/tmp/pq_shield_test_XXXXXX";
    std::string dir = mkdtemp(tmpl);
    std::string keypath = dir + "/pq.key";
    assert(WriteFile(keypath, sk, true));
    struct stat st;
    assert(stat(keypath.c_str(), &st) == 0);
    assert((st.st_mode & 0777) == 0600);
    std::string err;
    assert(AuditFile(keypath, true, err));
    chmod(keypath.c_str(), 0644);
    assert(!AuditFile(keypath, true, err));
    remove(keypath.c_str());
    rmdir(dir.c_str());

    std::printf("all tests passed\n");
    return 0;
}
