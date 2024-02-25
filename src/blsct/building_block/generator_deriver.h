// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_GENERATOR_DERIVER_H
#define NAVCOIN_BLSCT_GENERATOR_DERIVER_H

#include <ctokens/tokenid.h>
#include <optional>
#include <string>
#include <variant>
#include <vector>

using Bytes = std::vector<uint8_t>;

template <typename Point>
class GeneratorDeriver {
public:
    using Seed = std::variant<TokenId, Bytes>;

    GeneratorDeriver(const std::string& salt_str): salt_str{salt_str} {}

    Point Derive(
        const Point& p,
        const size_t index,
        const std::optional<Seed>& opt_token_id = TokenId()
    ) const;

private:
    const std::string salt_str;
};

#endif // NAVCOIN_BLSCT_GENERATOR_DERIVER_H

