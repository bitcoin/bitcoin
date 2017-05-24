// Copyright (c) 2011-2014 The Flow Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FLOW_QT_TRANSACTIONDESC_H
#define FLOW_QT_TRANSACTIONDESC_H

#include <QObject>
#include <QString>

class TransactionRecord;

class CWallet;
class CWalletTx;

/** Provide a human-readable extended HTML description of a transaction.
 */
class TransactionDesc: public QObject
{
    Q_OBJECT

public:
    static QString toHTML(CWallet *wallet, CWalletTx &wtx, TransactionRecord *rec, int unit);

private:
    TransactionDesc() {}

    static QString FormatTxStatus(const CWalletTx& wtx);
};

#endif // FLOW_QT_TRANSACTIONDESC_H
