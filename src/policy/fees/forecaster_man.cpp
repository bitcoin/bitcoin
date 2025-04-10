// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/forecaster.h>
#include <policy/fees/forecaster_man.h>
#include <policy/fees/forecaster_util.h>

#include <utility>

void FeeRateForecasterManager::RegisterForecaster(std::shared_ptr<Forecaster> forecaster)
{
    forecasters.emplace(forecaster->GetForecastType(), forecaster);
}
