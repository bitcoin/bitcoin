#ifndef MULTISIGDIALOG_H
#define MULTISIGDIALOG_H

#include <QDialog>

#include "multisigaddressentry.h"
#include "multisiginputentry.h"
#include "sendcoinsentry.h"
#include "walletmodel.h"


namespace Ui
{
    class MultisigDialog;
}

class MultisigDialog : public QDialog
{
    Q_OBJECT;

  public:
    explicit MultisigDialog(QWidget *parent);
    MultisigDialog();
    void setModel(WalletModel *model);

  public slots:
    MultisigAddressEntry * addPubKey();
    void clear();
    void updateRemoveEnabled();
    MultisigInputEntry * addInput();
    SendCoinsEntry * addOutput();

  private:
    Ui::MultisigDialog *ui;
    WalletModel *model;
    ~MultisigDialog();

  private slots:
    void on_createAddressButton_clicked();
    void on_copyMultisigAddressButton_clicked();
    void on_copyRedeemScriptButton_clicked();
    void on_saveRedeemScriptButton_clicked();
    void on_saveMultisigAddressButton_clicked();
    void removeEntry(MultisigAddressEntry *entry);
    void on_createTransactionButton_clicked();
    void on_transaction_textChanged();
    void on_copyTransactionButton_clicked();
    void on_pasteTransactionButton_clicked();
    void on_signTransactionButton_clicked();
    void on_copySignedTransactionButton_clicked();
    void on_sendTransactionButton_clicked();
    void removeEntry(MultisigInputEntry *entry);
    void removeEntry(SendCoinsEntry *entry);
    void updateAmounts();
};

#endif // MULTISIGDIALOG_H
