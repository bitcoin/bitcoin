// Rust functions which are exposed to C++ (ie are #[no_mangle] pub extern "C")
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RUSTY_H
#define BITCOIN_RUSTY_H

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <new>

namespace rust_hello_world_example {

extern "C" {

extern int32_t RUST_CONSTANT;

void hello_world();

} // extern "C"

} // namespace rust_hello_world_example

#endif // BITCOIN_RUSTY_H
