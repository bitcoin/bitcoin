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
    VeriBlock::UtilServiceImpl util_service_impl;
    UtilServiceMock util_service_mock;

    IntegrationTestFixture();
};

} // namespace VeriBlockTest
