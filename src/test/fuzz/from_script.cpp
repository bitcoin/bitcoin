// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/miniscript.h>
#include <script/script.h>
#include <test/fuzz/fuzz.h>

#include <initializer_list>
#include <string>

struct DummyKeyConverter {
    typedef CPubKey Key;
    bool return_value{false};

    explicit DummyKeyConverter(bool return_value_) : return_value(return_value_)
    {
    }

    template <typename I>
    bool FromPKBytes(I first, I last, Key& key) const
    {
        return return_value;
    }

    template <typename I>
    bool FromPKHBytes(I first, I last, Key& key) const
    {
        return return_value;
    }
};

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const CScript script(buffer.begin(), buffer.end());
    for (const bool return_value : {true, false}) {
        const DummyKeyConverter ctx{return_value};
        (void)miniscript::FromScript(script, ctx);
    }
}
