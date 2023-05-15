// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_GENERATOR_DERIVER_H
#define NAVCOIN_BLSCT_GENERATOR_DERIVER_H

#include <blsct/arith/mcl/mcl.h>
#include <ctokens/tokenid.h>
#include <optional>
#include <string>

using Point = Mcl::Point;

class GeneratorDeriver {
public:
    GeneratorDeriver(const std::string& salt_str): salt_str{salt_str} {}

    Point Derive(
        const Point& p,
        const size_t index,
        const std::optional<TokenId>& token_id = std::nullopt
    ) const;

private:
    const std::string salt_str;
};

#endif // NAVCOIN_BLSCT_GENERATOR_DERIVER_H