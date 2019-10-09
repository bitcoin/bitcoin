// Copyright (c) 2013-2018 The Talkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TALKCOIN_NOUI_H
#define TALKCOIN_NOUI_H

#include <string>

/** Non-GUI handler, which logs and prints messages. */
bool noui_ThreadSafeMessageBox(const std::string& message, const std::string& caption, unsigned int style);
/** Non-GUI handler, which logs and prints questions. */
bool noui_ThreadSafeQuestion(const std::string& /* ignored interactive message */, const std::string& message, const std::string& caption, unsigned int style);
/** Non-GUI handler, which only logs a message. */
void noui_InitMessage(const std::string& message);

/** Connect all talkcoind signal handlers */
void noui_connect();

/** Suppress all talkcoind signal handlers. Used to suppress output during test runs that produce expected errors */
void noui_suppress();

/** Reconnects the regular Non-GUI handlers after having used noui_suppress */
void noui_reconnect();

#endif // TALKCOIN_NOUI_H
