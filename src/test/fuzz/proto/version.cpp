#include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#include <test/fuzz/proto/convert.h>
#include <test/fuzz/proto/version.pb.h>

#include <net.h>
#include <net_processing.h>
#include <script/script.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/validation.h>
#include <validation.h>
#include <validationinterface.h>

#include <iostream>

namespace {
const TestingSetup* g_setup;
} // namespace

// WHYYYYY
// const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;
const std::function<void(const std::string&)> G_TEST_LOG_FUN;
const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    return 0;
}

int64_t ClampTime(int64_t time)
{
    const auto* active_tip = WITH_LOCK(
        cs_main, return g_setup->m_node.chainman->ActiveChain().Tip());
    return std::min((int64_t)std::numeric_limits<decltype(active_tip->nTime)>::max(),
                    std::max(time, active_tip->GetMedianTimePast() + 1));
}

template <class Proto>
using PostProcessor =
    protobuf_mutator::libfuzzer::PostProcessorRegistration<Proto>;

static PostProcessor<common_fuzz::HandshakeMsg> post_proc_atmp_mock_time = {
    [](common_fuzz::HandshakeMsg* message, unsigned int seed) {
        // Make sure the atmp mock time lies between a sensible minimum and maximum.
        if (message->has_mock_time()) {
            message->set_mock_time(ClampTime(message->mock_time()));
        }
    }};

DEFINE_PROTO_FUZZER(const version_fuzz::VersionHandshake& version_handshake)
{
    ConnmanTestMsg& connman = *static_cast<ConnmanTestMsg*>(g_setup->m_node.connman.get());
    TestChainState& chainstate = *static_cast<TestChainState*>(&g_setup->m_node.chainman->ActiveChainstate());
    SetMockTime(1610000000); // any time to successfully reset ibd
    chainstate.ResetIbd();

    LOCK(NetEventsInterface::g_msgproc_mutex);

    FuzzedDataProvider sock_data_provider{
        (uint8_t*)version_handshake.peer().socket_data_provider().data(),
        version_handshake.peer().socket_data_provider().size()};
    auto& node = ConvertConnection(version_handshake.peer(), sock_data_provider);
    g_setup->m_node.peerman->InitializeNode(node, ConvertServiceFlags(version_handshake.our_flags()));
    connman.AddTestNode(node);

    for (const auto& msg : version_handshake.msgs()) {
        auto [type, bytes] = ConvertHandshakeMsg(msg);

        if (msg.has_mock_time()) {
            SetMockTime(msg.mock_time());
        }

        CSerializedNetMsg net_msg;
        net_msg.m_type = type;
        net_msg.data = std::move(bytes);

        (void)connman.ReceiveMsgFrom(node, net_msg);
        node.fPauseSend = false;

        try {
            connman.ProcessMessagesOnce(node);
        } catch (const std::ios_base::failure&) {
        }
        g_setup->m_node.peerman->SendMessages(&node);
    }

    g_setup->m_node.connman->StopNodes();
}
