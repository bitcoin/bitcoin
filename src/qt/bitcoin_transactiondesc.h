// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TRANSACTIONDESC_H
#define BITCOIN_TRANSACTIONDESC_H

#include <QObject>
#include <QString>

class Bitcoin_TransactionRecord;

class Bitcoin_CWallet;
class Bitcoin_CWalletTx;

/** Provide a human-readable extended HTML description of a transaction.
 */
class Bitcoin_TransactionDesc: public QObject
{
    Q_OBJECT

public:
    static QString toHTML(Bitcoin_CWallet *wallet, Bitcoin_CWalletTx &wtx, Bitcoin_TransactionRecord *rec, int unit);

private:
    Bitcoin_TransactionDesc() {}

    static QString FormatTxStatus(const Bitcoin_CWalletTx& wtx);
};

#endif // BITCOIN_TRANSACTIONDESC_H
