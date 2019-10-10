// Rust functions which are exposed to C++ (ie are #[no_mangle] pub extern "C")
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RUSTY_SRC_RUST_BRIDGE_H
#define BITCOIN_RUSTY_SRC_RUST_BRIDGE_H

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <new>

namespace rust_block_fetch {

extern "C" {

bool init_fetch_dns_headers(const char *domain);
bool stop_fetch_dns_headers();

} // extern "C"

} // namespace rust_block_fetch

#endif // BITCOIN_RUSTY_SRC_RUST_BRIDGE_H
