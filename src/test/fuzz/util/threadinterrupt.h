// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_THREADINTERRUPT_H
#define BITCOIN_TEST_FUZZ_UTIL_THREADINTERRUPT_H

#include <test/fuzz/FuzzedDataProvider.h>
#include <util/threadinterrupt.h>

#include <memory>

/**
 * Mocked CThreadInterrupt that returns "randomly" whether it is interrupted and never sleeps.
 */
class FuzzedThreadInterrupt : public CThreadInterrupt
{
public:
    explicit FuzzedThreadInterrupt(FuzzedDataProvider& fuzzed_data_provider);

    virtual bool interrupted() const override;
    virtual bool sleep_for(Clock::duration) override;

private:
    FuzzedDataProvider& m_fuzzed_data_provider;
};

[[nodiscard]] inline std::shared_ptr<CThreadInterrupt> ConsumeThreadInterrupt(FuzzedDataProvider& fuzzed_data_provider)
{
    return std::make_shared<FuzzedThreadInterrupt>(fuzzed_data_provider);
}

#endif // BITCOIN_TEST_FUZZ_UTIL_THREADINTERRUPT_H
