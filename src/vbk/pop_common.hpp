// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_POP_COMMON_HPP
#define BITCOIN_SRC_VBK_POP_COMMON_HPP

#include <veriblock/pop_context.hpp>

namespace VeriBlock {

altintegration::PopContext& GetPop();

void SetPopConfig(const altintegration::Config& config);

void SetPop(std::shared_ptr<altintegration::PayloadsProvider>& db);

std::string toPrettyString(const altintegration::PopContext& pop);

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_POP_COMMON_HPP
