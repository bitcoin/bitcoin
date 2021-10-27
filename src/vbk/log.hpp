// Copyright (c) 2019-2021 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_LOG_HPP
#define INTEGRATION_REFERENCE_BTC_LOG_HPP

#include <logging.h>
#include <veriblock/pop.hpp>

namespace VeriBlock {

struct BTCSQLogger : public altintegration::Logger {
    ~BTCSQLogger() override = default;

    void log(altintegration::LogLevel l, const std::string& msg) override
    {
        LogPrint(BCLog::POP, "[alt-cpp] [%s] %s\n", altintegration::LevelToString(l), msg);
    }
};

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_LOG_HPP
