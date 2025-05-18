// Copyright (c) 2011-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_QT_GUICONSTANTS_H
#define TORTOISECOIN_QT_GUICONSTANTS_H

#include <chrono>
#include <cstdint>

using namespace std::chrono_literals;

/* A delay between model updates */
static constexpr auto MODEL_UPDATE_DELAY{250ms};

/* A delay between shutdown pollings */
static constexpr auto SHUTDOWN_POLLING_DELAY{200ms};

/* AskPassphraseDialog -- Maximum passphrase length */
static const int MAX_PASSPHRASE_SIZE = 1024;

/* TortoisecoinGUI -- Size of icons in status bar */
static const int STATUSBAR_ICONSIZE = 16;

static const bool DEFAULT_SPLASHSCREEN = true;

/* Invalid field background style */
#define STYLE_INVALID "border: 3px solid #FF8080"

/* Transaction list -- unconfirmed transaction */
#define COLOR_UNCONFIRMED QColor(128, 128, 128)
/* Transaction list -- negative amount */
#define COLOR_NEGATIVE QColor(255, 0, 0)
/* Transaction list -- bare address (without label) */
#define COLOR_BAREADDRESS QColor(140, 140, 140)
/* Transaction list -- TX status decoration - danger, tx needs attention */
#define COLOR_TX_STATUS_DANGER QColor(200, 100, 100)
/* Transaction list -- TX status decoration - default color */
#define COLOR_BLACK QColor(0, 0, 0)

/* Tooltips longer than this (in characters) are converted into rich text,
   so that they can be word-wrapped.
 */
static const int TOOLTIP_WRAP_THRESHOLD = 80;

/* Number of frames in spinner animation */
#define SPINNER_FRAMES 36

#define QAPP_ORG_NAME "Tortoisecoin"
#define QAPP_ORG_DOMAIN "tortoisecoin.org"
#define QAPP_APP_NAME_DEFAULT "Tortoisecoin-Qt"
#define QAPP_APP_NAME_TESTNET "Tortoisecoin-Qt-testnet"
#define QAPP_APP_NAME_TESTNET4 "Tortoisecoin-Qt-testnet4"
#define QAPP_APP_NAME_SIGNET "Tortoisecoin-Qt-signet"
#define QAPP_APP_NAME_REGTEST "Tortoisecoin-Qt-regtest"

/* One gigabyte (GB) in bytes */
static constexpr uint64_t GB_BYTES{1000000000};

// Default prune target displayed in GUI.
static constexpr int DEFAULT_PRUNE_TARGET_GB{2};

#endif // TORTOISECOIN_QT_GUICONSTANTS_H
