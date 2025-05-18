// Copyright (c) 2020-present The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockfilter.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/messages.h>
#include <common/settings.h>
#include <common/system.h>
#include <common/url.h>
#include <netbase.h>
#include <outputtype.h>
#include <rpc/client.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

enum class FeeEstimateMode;

using common::AmountErrMsg;
using common::AmountHighWarn;
using common::FeeModeFromString;
using common::ResolveErrMsg;
using util::ContainsNoNUL;
using util::Join;
using util::RemovePrefix;
using util::SplitString;
using util::TrimString;

FUZZ_TARGET(string)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string random_string_1 = fuzzed_data_provider.ConsumeRandomLengthString(32);
    const std::string random_string_2 = fuzzed_data_provider.ConsumeRandomLengthString(32);
    const std::vector<std::string> random_string_vector = ConsumeRandomLengthStringVector(fuzzed_data_provider);

    (void)AmountErrMsg(random_string_1, random_string_2);
    (void)AmountHighWarn(random_string_1);
    BlockFilterType block_filter_type;
    (void)BlockFilterTypeByName(random_string_1, block_filter_type);
    (void)Capitalize(random_string_1);
    (void)CopyrightHolders(random_string_1);
    FeeEstimateMode fee_estimate_mode;
    (void)FeeModeFromString(random_string_1, fee_estimate_mode);
    const auto width{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 1000)};
    (void)FormatParagraph(random_string_1, width, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, width));
    (void)FormatSubVersion(random_string_1, fuzzed_data_provider.ConsumeIntegral<int>(), random_string_vector);
    (void)GetDescriptorChecksum(random_string_1);
    (void)HelpExampleCli(random_string_1, random_string_2);
    (void)HelpExampleRpc(random_string_1, random_string_2);
    (void)HelpMessageGroup(random_string_1);
    (void)HelpMessageOpt(random_string_1, random_string_2);
    (void)IsDeprecatedRPCEnabled(random_string_1);
    (void)Join(random_string_vector, random_string_1);
    (void)JSONRPCError(fuzzed_data_provider.ConsumeIntegral<int>(), random_string_1);
    const common::Settings settings;
    (void)OnlyHasDefaultSectionSetting(settings, random_string_1, random_string_2);
    (void)ParseNetwork(random_string_1);
    (void)ParseOutputType(random_string_1);
    (void)RemovePrefix(random_string_1, random_string_2);
    (void)ResolveErrMsg(random_string_1, random_string_2);
    try {
        (void)RPCConvertNamedValues(random_string_1, random_string_vector);
    } catch (const std::runtime_error&) {
    }
    try {
        (void)RPCConvertValues(random_string_1, random_string_vector);
    } catch (const std::runtime_error&) {
    }
    (void)SanitizeString(random_string_1);
    (void)SanitizeString(random_string_1, fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 3));
#ifndef WIN32
    (void)ShellEscape(random_string_1);
#endif // WIN32
    uint16_t port_out;
    std::string host_out;
    SplitHostPort(random_string_1, port_out, host_out);
    (void)TimingResistantEqual(random_string_1, random_string_2);
    (void)ToLower(random_string_1);
    (void)ToUpper(random_string_1);
    (void)TrimString(random_string_1);
    (void)TrimString(random_string_1, random_string_2);
    (void)UrlDecode(random_string_1);
    (void)ContainsNoNUL(random_string_1);
    try {
        throw scriptnum_error{random_string_1};
    } catch (const std::runtime_error&) {
    }

    {
        DataStream data_stream{};
        std::string s;
        auto limited_string = LIMITED_STRING(s, 10);
        data_stream << random_string_1;
        try {
            data_stream >> limited_string;
            assert(data_stream.empty());
            assert(s.size() <= random_string_1.size());
            assert(s.size() <= 10);
            if (!random_string_1.empty()) {
                assert(!s.empty());
            }
        } catch (const std::ios_base::failure&) {
        }
    }
    {
        DataStream data_stream{};
        const auto limited_string = LIMITED_STRING(random_string_1, 10);
        data_stream << limited_string;
        std::string deserialized_string;
        data_stream >> deserialized_string;
        assert(data_stream.empty());
        assert(deserialized_string == random_string_1);
    }
    {
        int64_t amount_out;
        (void)ParseFixedPoint(random_string_1, fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 1024), &amount_out);
    }
    {
        const auto single_split{SplitString(random_string_1, fuzzed_data_provider.ConsumeIntegral<char>())};
        assert(single_split.size() >= 1);
        const auto any_split{SplitString(random_string_1, random_string_2)};
        assert(any_split.size() >= 1);
    }
    {
        (void)Untranslated(random_string_1);
        const bilingual_str bs1{random_string_1, random_string_2};
        const bilingual_str bs2{random_string_2, random_string_1};
        (void)(bs1 + bs2);
    }
}
