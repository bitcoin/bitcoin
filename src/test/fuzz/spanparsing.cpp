// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/spanparsing.h>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const size_t query_size = fuzzed_data_provider.ConsumeIntegral<size_t>();
    const std::string query = fuzzed_data_provider.ConsumeBytesAsString(std::min<size_t>(query_size, 1024 * 1024));
    const std::string span_str = fuzzed_data_provider.ConsumeRemainingBytesAsString();
    const Span<const char> const_span{span_str};

    Span<const char> mut_span = const_span;
    (void)spanparsing::Const(query, mut_span);

    mut_span = const_span;
    (void)spanparsing::Func(query, mut_span);

    mut_span = const_span;
    (void)spanparsing::Expr(mut_span);

    if (!query.empty()) {
        mut_span = const_span;
        (void)spanparsing::Split(mut_span, query.front());
    }
}
