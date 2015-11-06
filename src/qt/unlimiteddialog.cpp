// Copyright (c) 2011-2015 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "unlimiteddialog.h"

#include "bitcoinunits.h"
#include "guiutil.h"

#include "main.h" // for DEFAULT_SCRIPTCHECK_THREADS and MAX_SCRIPTCHECK_THREADS
#include "netbase.h"
#include "net.h"  // for access to the network traffic shapers
#include "txdb.h" // for -dbcache defaults

#ifdef ENABLE_WALLET
#include "wallet/wallet.h" // for CWallet::minTxFee
#endif

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

#include <QDataWidgetMapper>
#include <QDir>
#include <QIntValidator>
#include <QLocale>
#include <QMessageBox>
#include <QTimer>


UnlimitedDialog::UnlimitedDialog(QWidget* parent):
  QDialog(parent)  
{
  ui.setupUi(this);  
}

UnlimitedDialog::~UnlimitedDialog()
{
}
