// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_INIT_HPP
#define BITCOIN_SRC_VBK_INIT_HPP

#include "pop_service_impl.hpp"
#include "vbk/config.hpp"
#include "vbk/service_locator.hpp"
#include <util/system.h>

#include <utility>

namespace VeriBlock {

inline PopService& InitPopService(const fs::path& dbpath)
{
    auto& config = getService<VeriBlock::Config>();
    auto* ptr = new PopServiceImpl(config.popconfig, dbpath);
    setService<PopService>(ptr);
    return *ptr;
}

inline Config& InitConfig()
{
    auto* cfg = new Config();
    setService<Config>(cfg);
    return *cfg;
}


} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_INIT_HPP
