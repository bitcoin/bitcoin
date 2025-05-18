// Copyright (c) 2020-2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <signet.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

#include <cstdint>
#include <optional>
#include <vector>

void initialize_signet()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::SIGNET);
}

FUZZ_TARGET(signet, .init = initialize_signet)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::optional<CBlock> block = ConsumeDeserializable<CBlock>(fuzzed_data_provider, TX_WITH_WITNESS);
    if (!block) {
        return;
    }
    (void)CheckSignetBlockSolution(*block, Params().GetConsensus());
    (void)SignetTxs::Create(*block, ConsumeScript(fuzzed_data_provider));
}
