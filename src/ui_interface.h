// Copyright (c) 2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_UI_INTERFACE_H
#define BITCOIN_UI_INTERFACE_H

#include <string>
#include "util.h" // for int64
#include <boost/signals2/signal.hpp>
#include <boost/signals2/last_value.hpp>

class CBasicKeyStore;
class CWallet;
class uint256;

/** Flags for CClientUIInterface::ThreadSafeMessageBox */
enum MessageBoxFlags
{
    MF_YES                   = 0x00000002,
    MF_OK                    = 0x00000004,
    MF_NO                    = 0x00000008,
    MF_YES_NO                = (MF_YES|MF_NO),
    MF_CANCEL                = 0x00000010,
    MF_APPLY                 = 0x00000020,
    MF_CLOSE                 = 0x00000040,
    MF_OK_DEFAULT            = 0x00000000,
    MF_YES_DEFAULT           = 0x00000000,
    MF_NO_DEFAULT            = 0x00000080,
    MF_CANCEL_DEFAULT        = 0x80000000,
    MF_ICON_EXCLAMATION      = 0x00000100,
    MF_ICON_HAND             = 0x00000200,
    MF_ICON_WARNING          = MF_ICON_EXCLAMATION,
    MF_ICON_ERROR            = MF_ICON_HAND,
    MF_ICON_QUESTION         = 0x00000400,
    MF_ICON_INFORMATION      = 0x00000800,
    MF_ICON_STOP             = MF_ICON_HAND,
    MF_ICON_ASTERISK         = MF_ICON_INFORMATION,
    MF_ICON_MASK             = (0x00000100|0x00000200|0x00000400|0x00000800),
    MF_FORWARD               = 0x00001000,
    MF_BACKWARD              = 0x00002000,
    MF_RESET                 = 0x00004000,
    MF_HELP                  = 0x00008000,
    MF_MORE                  = 0x00010000,
    MF_SETUP                 = 0x00020000,
// Force blocking, modal message box dialog (not just OS notification)
    MF_MODAL                 = 0x00040000
};

/** General change type (added, updated, removed). */
enum ChangeType
{
    CT_NEW,
    CT_UPDATED,
    CT_DELETED
};

/** Signals for UI communication. */
class CClientUIInterface
{
public:
    /** Show message box. */
    boost::signals2::signal<void (const std::string& message, const std::string& caption, int style)> ThreadSafeMessageBox;

    /** Ask the user whether he want to pay a fee or not. */
    boost::signals2::signal<bool (int64 nFeeRequired, const std::string& strCaption), boost::signals2::last_value<bool> > ThreadSafeAskFee;

    /** Handle an URL passed on the command line. */
    boost::signals2::signal<void (const std::string& strURI)> ThreadSafeHandleURI;

    /** Progress message during initialization. */
    boost::signals2::signal<void (const std::string &message)> InitMessage;

    /** Initiate client shutdown. */
    boost::signals2::signal<void ()> QueueShutdown;

    /** Translate a message to the native language of the user. */
    boost::signals2::signal<std::string (const char* psz)> Translate;

    /** Block chain changed. */
    boost::signals2::signal<void ()> NotifyBlocksChanged;

    /** Number of network connections changed. */
    boost::signals2::signal<void (int newNumConnections)> NotifyNumConnectionsChanged;

    /**
     * New, updated or cancelled alert.
     * @note called with lock cs_mapAlerts held.
     */
    boost::signals2::signal<void (const uint256 &hash, ChangeType status)> NotifyAlertChanged;
};

extern CClientUIInterface uiInterface;

/**
 * Translation function: Call Translate signal on UI interface, which returns a boost::optional result.
 * If no translation slot is registered, nothing is returned, and simply return the input.
 */
inline std::string _(const char* psz)
{
    boost::optional<std::string> rv = uiInterface.Translate(psz);
    return rv ? (*rv) : psz;
}

#endif
