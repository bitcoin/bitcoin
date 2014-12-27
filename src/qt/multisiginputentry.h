#ifndef MULTISIGINPUTENTRY_H
#define MULTISIGINPUTENTRY_H

#include <QFrame>

#include "uint256.h"


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

  public slots:
    void setRemoveEnabled(bool enabled);
    void clear();

  signals:
    void removeEntry(MultisigInputEntry *entry);
    void updateAmount();

  private:
    Ui::MultisigInputEntry *ui;
    WalletModel *model;
    uint256 txHash;

  private slots:
    void on_transactionId_textChanged(const QString &transactionId);
    void on_pasteTransactionIdButton_clicked();
    void on_deleteButton_clicked();
    void on_transactionOutput_currentIndexChanged(int index);
    void on_pasteRedeemScriptButton_clicked();
};

#endif // MULTISIGINPUTENTRY_H