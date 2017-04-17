// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interface/node.h>

#include <chainparams.h>
#include <init.h>
#include <interface/handler.h>
#include <interface/wallet.h>
#include <net.h>
#include <netaddress.h>
#include <netbase.h>
#include <scheduler.h>
#include <ui_interface.h>
#include <util.h>
#include <warnings.h>

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif
#ifdef ENABLE_WALLET
#define CHECK_WALLET(x) x
#else
#define CHECK_WALLET(x) throw std::logic_error("Wallet function called in non-wallet build.")
#endif

#include <boost/thread/thread.hpp>

class CWallet;

namespace interface {
namespace {

class NodeImpl : public Node
{
    void parseParameters(int argc, const char* const argv[]) override
    {
        gArgs.ParseParameters(argc, argv);
    }
    void readConfigFile(const std::string& conf_path) override { gArgs.ReadConfigFile(conf_path); }
    bool softSetArg(const std::string& arg, const std::string& value) override { return gArgs.SoftSetArg(arg, value); }
    bool softSetBoolArg(const std::string& arg, bool value) override { return gArgs.SoftSetBoolArg(arg, value); }
    void selectParams(const std::string& network) override { SelectParams(network); }
    void initLogging() override { InitLogging(); }
    void initParameterInteraction() override { InitParameterInteraction(); }
    std::string getWarnings(const std::string& type) override { return GetWarnings(type); }
    bool baseInitialize() override
    {
        return AppInitBasicSetup() && AppInitParameterInteraction() && AppInitSanityChecks() &&
               AppInitLockDataDirectory();
    }
    bool appInitMain() override { return AppInitMain(); }
    void appShutdown() override
    {
        Interrupt();
        Shutdown();
    }
    void startShutdown() override { StartShutdown(); }
    bool shutdownRequested() override { return ShutdownRequested(); }
    void mapPort(bool use_upnp) override
    {
        if (use_upnp) {
            StartMapPort();
        } else {
            InterruptMapPort();
            StopMapPort();
        }
    }
    std::string helpMessage(HelpMessageMode mode) override { return HelpMessage(mode); }
    bool getProxy(Network net, proxyType& proxy_info) override { return GetProxy(net, proxy_info); }
    std::unique_ptr<Handler> handleInitMessage(InitMessageFn fn) override
    {
        return MakeHandler(::uiInterface.InitMessage.connect(fn));
    }
    std::unique_ptr<Handler> handleMessageBox(MessageBoxFn fn) override
    {
        return MakeHandler(::uiInterface.ThreadSafeMessageBox.connect(fn));
    }
    std::unique_ptr<Handler> handleQuestion(QuestionFn fn) override
    {
        return MakeHandler(::uiInterface.ThreadSafeQuestion.connect(fn));
    }
    std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) override
    {
        return MakeHandler(::uiInterface.ShowProgress.connect(fn));
    }
    std::unique_ptr<Handler> handleLoadWallet(LoadWalletFn fn) override
    {
        CHECK_WALLET(
            return MakeHandler(::uiInterface.LoadWallet.connect([fn](CWallet* wallet) { fn(MakeWallet(*wallet)); })));
    }
};

} // namespace

std::unique_ptr<Node> MakeNode() { return MakeUnique<NodeImpl>(); }

} // namespace interface
