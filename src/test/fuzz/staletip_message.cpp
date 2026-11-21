// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <staletipmessage.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <ios>

FUZZ_TARGET(staletip_message)
{
    DataStream stream{buffer};
    StaleTipMessage data;
    try {
        stream >> data;
    } catch (const std::ios_base::failure&) {
        return;
    }

    const auto decompressed{data.DecompressHeaders()};
    assert(decompressed.size() == data.headers.size());
    assert(!decompressed.empty());
    assert(decompressed.front().hashPrevBlock == data.fork_point);

    DataStream serialized;
    serialized << data;
    DataStream roundtrip_stream{serialized};
    StaleTipMessage roundtrip;
    roundtrip_stream >> roundtrip;
    assert(roundtrip == data);
}
