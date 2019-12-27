// Copyright (c) 2012-2020 The Peercoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PEERCOIN_QT_MULTISIGINPUTENTRY_H
#define PEERCOIN_QT_MULTISIGINPUTENTRY_H

#include <uint256.h>

#include <QFrame>

class CTxIn;
class WalletModel;

namespace Ui
{
    class MultisigInputEntry;
}

class MultisigInputEntry : public QFrame
{
    Q_OBJECT;

  public:
    explicit MultisigInputEntry(QWidget *parent = 0);
    ~MultisigInputEntry();
    void setModel(WalletModel *model);
    bool validate();
    CTxIn getInput();
    int64_t getAmount();
    QString getRedeemScript();
    void setTransactionId(QString transactionId);
    void setTransactionOutputIndex(int index);

  public Q_SLOTS:
    void setRemoveEnabled(bool enabled);
    void clear();

  Q_SIGNALS:
    void removeEntry(MultisigInputEntry *entry);
    void updateAmount();

  private:
    Ui::MultisigInputEntry *ui;
    WalletModel *model;
    uint256 txHash;

  private Q_SLOTS:
    void on_transactionId_textChanged(const QString &transactionId);
    void on_pasteTransactionIdButton_clicked();
    void on_deleteButton_clicked();
    void on_transactionOutput_currentIndexChanged(int index);
    void on_pasteRedeemScriptButton_clicked();
};

#endif // PEERCOIN_QT_MULTISIGINPUTENTRY_H
