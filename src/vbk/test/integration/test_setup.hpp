#include <script/interpreter.h>
#include <test/setup_common.h>

#include <vbk/init.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/integration/grpc_integration_service.hpp>
#include <vbk/test/integration/utils.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/tx.hpp>
#include <vbk/util_service.hpp>
#include <vbk/util_service/util_service_impl.hpp>

#include <fakeit.hpp>

namespace VeriBlockTest {

struct IntegrationTestFixture : public TestChain100Setup {
    VeriBlock::UtilServiceImpl util_service_impl;
    fakeit::Mock<VeriBlock::UtilService> util_service_mock;

    IntegrationTestFixture();
};

} // namespace VeriBlockTest
