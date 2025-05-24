// Copyright (c) 2010-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/interface_ui.h>

#include <util/string.h>
#include <util/translation.h>

#include <boost/signals2/optional_last_value.hpp>
#include <boost/signals2/signal.hpp>

using util::MakeUnorderedList;

CClientUIInterface uiInterface;

struct UISignals {
    boost::signals2::signal<CClientUIInterface::ThreadSafeMessageBoxSig, boost::signals2::optional_last_value<bool>> ThreadSafeMessageBox;
    boost::signals2::signal<CClientUIInterface::ThreadSafeQuestionSig, boost::signals2::optional_last_value<bool>> ThreadSafeQuestion;
    boost::signals2::signal<CClientUIInterface::InitMessageSig> InitMessage;
    boost::signals2::signal<CClientUIInterface::InitWalletSig> InitWallet;
    boost::signals2::signal<CClientUIInterface::NotifyNumConnectionsChangedSig> NotifyNumConnectionsChanged;
    boost::signals2::signal<CClientUIInterface::NotifyNetworkActiveChangedSig> NotifyNetworkActiveChanged;
    boost::signals2::signal<CClientUIInterface::NotifyAlertChangedSig> NotifyAlertChanged;
    boost::signals2::signal<CClientUIInterface::ShowProgressSig> ShowProgress;
    boost::signals2::signal<CClientUIInterface::NotifyBlockTipSig> NotifyBlockTip;
    boost::signals2::signal<CClientUIInterface::NotifyHeaderTipSig> NotifyHeaderTip;
    boost::signals2::signal<CClientUIInterface::BannedListChangedSig> BannedListChanged;
};
static UISignals g_ui_signals;

#define ADD_SIGNALS_IMPL_WRAPPER(signal_name)                                                                 \
    boost::signals2::connection CClientUIInterface::signal_name##_connect(std::function<signal_name##Sig> fn) \
    {                                                                                                         \
        return g_ui_signals.signal_name.connect(fn);                                                          \
    }

ADD_SIGNALS_IMPL_WRAPPER(ThreadSafeMessageBox);
ADD_SIGNALS_IMPL_WRAPPER(ThreadSafeQuestion);
ADD_SIGNALS_IMPL_WRAPPER(InitMessage);
ADD_SIGNALS_IMPL_WRAPPER(InitWallet);
ADD_SIGNALS_IMPL_WRAPPER(NotifyNumConnectionsChanged);
ADD_SIGNALS_IMPL_WRAPPER(NotifyNetworkActiveChanged);
ADD_SIGNALS_IMPL_WRAPPER(NotifyAlertChanged);
ADD_SIGNALS_IMPL_WRAPPER(ShowProgress);
ADD_SIGNALS_IMPL_WRAPPER(NotifyBlockTip);
ADD_SIGNALS_IMPL_WRAPPER(NotifyHeaderTip);
ADD_SIGNALS_IMPL_WRAPPER(BannedListChanged);

bool CClientUIInterface::ThreadSafeMessageBox(const bilingual_str& message, const std::string& caption, unsigned int style) { return g_ui_signals.ThreadSafeMessageBox(message, caption, style).value_or(false);}
bool CClientUIInterface::ThreadSafeQuestion(const bilingual_str& message, const std::string& non_interactive_message, const std::string& caption, unsigned int style) { return g_ui_signals.ThreadSafeQuestion(message, non_interactive_message, caption, style).value_or(false);}
void CClientUIInterface::InitMessage(const std::string& message) { return g_ui_signals.InitMessage(message); }
void CClientUIInterface::InitWallet() { return g_ui_signals.InitWallet(); }
void CClientUIInterface::NotifyNumConnectionsChanged(int newNumConnections) { return g_ui_signals.NotifyNumConnectionsChanged(newNumConnections); }
void CClientUIInterface::NotifyNetworkActiveChanged(bool networkActive) { return g_ui_signals.NotifyNetworkActiveChanged(networkActive); }
void CClientUIInterface::NotifyAlertChanged() { return g_ui_signals.NotifyAlertChanged(); }
void CClientUIInterface::ShowProgress(const std::string& title, int nProgress, bool resume_possible) { return g_ui_signals.ShowProgress(title, nProgress, resume_possible); }
void CClientUIInterface::NotifyBlockTip(SynchronizationState s, const CBlockIndex& block, double verification_progress) { return g_ui_signals.NotifyBlockTip(s, block, verification_progress); }
void CClientUIInterface::NotifyHeaderTip(SynchronizationState s, int64_t height, int64_t timestamp, bool presync) { return g_ui_signals.NotifyHeaderTip(s, height, timestamp, presync); }
void CClientUIInterface::BannedListChanged() { return g_ui_signals.BannedListChanged(); }

bool InitError(const bilingual_str& str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_ERROR);
    return false;
}

bool InitError(const bilingual_str& str, const std::vector<std::string>& details)
{
    // For now just flatten the list of error details into a string to pass to
    // the base InitError overload. In the future, if more init code provides
    // error details, the details could be passed separately from the main
    // message for rich display in the GUI. But currently the only init
    // functions which provide error details are ones that run during early init
    // before the GUI uiInterface is registered, so there's no point passing
    // main messages and details separately to uiInterface yet.
    return InitError(details.empty() ? str : str + Untranslated(strprintf(":\n%s", MakeUnorderedList(details))));
}

void InitWarning(const bilingual_str& str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_WARNING);
}
