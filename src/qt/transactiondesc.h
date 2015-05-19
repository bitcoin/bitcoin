// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRANSACTIONDESC_H
#define TRANSACTIONDESC_H

#include <QObject>
#include <QString>

class Credits_TransactionRecord;

class Credits_CWallet;
class Credits_CWalletTx;

/** Provide a human-readable extended HTML description of a transaction.
 */
class Credits_TransactionDesc: public QObject
{
    Q_OBJECT

public:
    static QString toHTML(Credits_CWallet *keyholder_wallet, Credits_CWalletTx &wtx, Credits_TransactionRecord *rec, int unit);

private:
    Credits_TransactionDesc() {}

    static QString FormatTxStatus(const Credits_CWalletTx& wtx);
};

#endif // TRANSACTIONDESC_H
