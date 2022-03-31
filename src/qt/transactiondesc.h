// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TRANSACTIONDESC_H
#define BITCOIN_QT_TRANSACTIONDESC_H

#include <QObject>
#include <QString>

class WalletTxRecord;

namespace interfaces {
class Node;
class Wallet;
struct WalletTx;
struct WalletTxStatus;
using WalletOrderForm = std::vector<std::pair<std::string, std::string>>;
}

/** Provide a human-readable extended HTML description of a transaction.
 */
class TransactionDesc: public QObject
{
    Q_OBJECT

public:
    static QString toHTML(interfaces::Node& node, interfaces::Wallet& wallet, WalletTxRecord* rec, int unit);

private:
    TransactionDesc() {}

    static QString FormatTxStatus(const interfaces::WalletTx& wtx, const interfaces::WalletTxStatus& status, bool inMempool, int numBlocks);

    static QString toHTML_Addresses(interfaces::Wallet& wallet, const interfaces::WalletTx& wtx, WalletTxRecord* rec);
    static QString toHTML_Amounts(interfaces::Wallet& wallet, const interfaces::WalletTx& wtx, const interfaces::WalletTxStatus& status, int unit);
    static QString toHTML_OrderForm(const interfaces::WalletOrderForm& orderForm);
    static QString toHTML_Debug(interfaces::Node& node, interfaces::Wallet& wallet, const interfaces::WalletTx& wtx, WalletTxRecord* rec, int unit);
};

#endif // BITCOIN_QT_TRANSACTIONDESC_H
