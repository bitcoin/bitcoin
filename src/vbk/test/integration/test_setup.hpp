// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/interpreter.h>
#include <test/util/setup_common.h>
#include <chain.h>

#include <vbk/init.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/integration/utils.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/tx.hpp>

#include <gmock/gmock.h>

namespace VeriBlockTest {

struct IntegrationTestFixture : public TestChain100Setup {
    IntegrationTestFixture();
};

} // namespace VeriBlockTest
