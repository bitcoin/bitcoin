#ifndef MULTISIGINPUTENTRY_H
#define MULTISIGINPUTENTRY_H

#include <QFrame>

#include "uint256.h"
#include "amount.h"

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
    CAmount getAmount();
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

#endif // MULTISIGINPUTENTRY_H
