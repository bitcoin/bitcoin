// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_INTERFACE_UI_H
#define BITCOIN_NODE_INTERFACE_UI_H

#include <util/btcsignals.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class CBlockIndex;
enum class SynchronizationState;
struct bilingual_str;

/** Signals for UI communication. */
class CClientUIInterface
{
public:
    /** Flags for CClientUIInterface::ThreadSafeMessageBox */
    enum MessageBoxFlags : uint32_t {
        ICON_INFORMATION    = 0,
        ICON_WARNING        = (1U << 0),
        ICON_ERROR          = (1U << 1),
        /**
         * Mask of all available icons in CClientUIInterface::MessageBoxFlags
         * This needs to be updated, when icons are changed there!
         */
        ICON_MASK = (ICON_INFORMATION | ICON_WARNING | ICON_ERROR),

        /** These values are taken from qmessagebox.h "enum StandardButton" to be directly usable */
        BTN_OK      = 0x00000400U, // QMessageBox::Ok
        BTN_YES     = 0x00004000U, // QMessageBox::Yes
        BTN_NO      = 0x00010000U, // QMessageBox::No
        BTN_ABORT   = 0x00040000U, // QMessageBox::Abort
        BTN_RETRY   = 0x00080000U, // QMessageBox::Retry
        BTN_IGNORE  = 0x00100000U, // QMessageBox::Ignore
        BTN_CLOSE   = 0x00200000U, // QMessageBox::Close
        BTN_CANCEL  = 0x00400000U, // QMessageBox::Cancel
        BTN_DISCARD = 0x00800000U, // QMessageBox::Discard
        BTN_HELP    = 0x01000000U, // QMessageBox::Help
        BTN_APPLY   = 0x02000000U, // QMessageBox::Apply
        BTN_RESET   = 0x04000000U, // QMessageBox::Reset
        /**
         * Mask of all available buttons in CClientUIInterface::MessageBoxFlags
         * This needs to be updated, when buttons are changed there!
         */
        BTN_MASK = (BTN_OK | BTN_YES | BTN_NO | BTN_ABORT | BTN_RETRY | BTN_IGNORE |
                    BTN_CLOSE | BTN_CANCEL | BTN_DISCARD | BTN_HELP | BTN_APPLY | BTN_RESET),

        /** Force blocking, modal message box dialog (not just OS notification) */
        MODAL               = 0x10000000U,

        /** Do not print contents of message to debug log */
        SECURE              = 0x40000000U,

        /** Predefined combinations for certain default usage cases */
        MSG_INFORMATION = ICON_INFORMATION,
        MSG_WARNING = (ICON_WARNING | BTN_OK | MODAL),
        MSG_ERROR = (ICON_ERROR | BTN_OK | MODAL)
    };

    /** Show message box. */
    btcsignals::signal<void(const bilingual_str& message, unsigned int style)> ThreadSafeMessageBox;

    /** If possible, ask the user a question. If not, falls back to ThreadSafeMessageBox(noninteractive_message, style) and returns false. */
    btcsignals::signal<bool(const bilingual_str& message, const std::string& noninteractive_message, unsigned int style), btcsignals::any_of> ThreadSafeQuestion;

    /** Progress message during initialization. */
    btcsignals::signal<void(const std::string& message)> InitMessage;

    /** Wallet loader created. */
    btcsignals::signal<void()> InitWallet;

    /** Number of network connections changed. */
    btcsignals::signal<void(int newNumConnections)> NotifyNumConnectionsChanged;

    /** Network activity state changed. */
    btcsignals::signal<void(bool networkActive)> NotifyNetworkActiveChanged;

    /**
     * Status bar alerts changed.
     */
    btcsignals::signal<void()> NotifyAlertChanged;

    /**
     * Show progress e.g. for verifychain.
     * resume_possible indicates shutting down now will result in the current progress action resuming upon restart.
     */
    btcsignals::signal<void(const std::string& title, int nProgress, bool resume_possible)> ShowProgress;

    /** New block has been accepted */
    btcsignals::signal<void(SynchronizationState, const CBlockIndex& block, double verification_progress)> NotifyBlockTip;

    /** Best header has changed */
    btcsignals::signal<void(SynchronizationState, int64_t height, int64_t timestamp, bool presync)> NotifyHeaderTip;

    /** Banlist did change. */
    btcsignals::signal<void(void)> BannedListChanged;
};

/** Show warning message **/
void InitWarning(const bilingual_str& str);

/** Show error message **/
bool InitError(const bilingual_str& str);
bool InitError(const bilingual_str& str, const std::vector<std::string>& details);

extern CClientUIInterface uiInterface;

#endif // BITCOIN_NODE_INTERFACE_UI_H
