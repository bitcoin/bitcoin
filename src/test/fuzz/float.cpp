// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <memusage.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <version.h>

#include <cassert>
#include <cstdint>

FUZZ_TARGET(float)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    {
        const double d = fuzzed_data_provider.ConsumeFloatingPoint<double>();
        (void)memusage::DynamicUsage(d);
        assert(ser_uint64_to_double(ser_double_to_uint64(d)) == d);

        CDataStream stream(SER_NETWORK, INIT_PROTO_VERSION);
        stream << d;
        double d_deserialized;
        stream >> d_deserialized;
        assert(d == d_deserialized);
    }

    {
        const float f = fuzzed_data_provider.ConsumeFloatingPoint<float>();
        (void)memusage::DynamicUsage(f);
        assert(ser_uint32_to_float(ser_float_to_uint32(f)) == f);

        CDataStream stream(SER_NETWORK, INIT_PROTO_VERSION);
        stream << f;
        float f_deserialized;
        stream >> f_deserialized;
        assert(f == f_deserialized);
    }
}
