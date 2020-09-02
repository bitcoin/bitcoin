// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pop_common.hpp"

namespace VeriBlock {

static std::shared_ptr<altintegration::Altintegration> app = nullptr;
static std::shared_ptr<altintegration::Config> config = nullptr;

altintegration::Altintegration& GetPop()
{
    assert(app && "Altintegration is not initialized. Invoke SetPop.");
    return *app;
}

void SetPopConfig(const altintegration::Config& newConfig)
{
    config = std::make_shared<altintegration::Config>(newConfig);
}

void SetPop(std::shared_ptr<altintegration::PayloadsProvider>& db)
{
    assert(config && "Config is not initialized. Invoke SetPopConfig.");
    app = altintegration::Altintegration::create(config, db);
}

std::string toPrettyString(const altintegration::Altintegration& pop)
{
    return pop.altTree->toPrettyString();
}

} // namespace VeriBlock
