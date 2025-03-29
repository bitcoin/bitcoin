// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/util.h>
#include <test/fuzz/util/threadinterrupt.h>

FuzzedThreadInterrupt::FuzzedThreadInterrupt(FuzzedDataProvider& fuzzed_data_provider)
    : m_fuzzed_data_provider{fuzzed_data_provider}
{
}

bool FuzzedThreadInterrupt::interrupted() const
{
    return m_fuzzed_data_provider.ConsumeBool();
}

bool FuzzedThreadInterrupt::sleep_for(Clock::duration)
{
    SetMockTime(ConsumeTime(m_fuzzed_data_provider)); // Time could go backwards.
    return m_fuzzed_data_provider.ConsumeBool();
}
