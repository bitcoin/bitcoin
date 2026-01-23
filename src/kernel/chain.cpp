// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/chain.h>

#include <kernel/types.h>

using kernel::ChainstateRole;

namespace kernel {
std::ostream& operator<<(std::ostream& os, const ChainstateRole& role) {
    if (!role.validated) {
        os << "assumedvalid";
    } else if (role.historical) {
        os << "background";
    } else {
        os << "normal";
    }
    return os;
}
} // namespace kernel
