#ifndef IMPORTPRIVATEKEYDIALOG_H
#define IMPORTPRIVATEKEYDIALOG_H

#include <QDialog>
#include <QValidator>

#include "bitcoinrpc.h"

namespace Ui {
    class ImportPrivateKeyDialog;
}
class WalletModel;

class PrivateKeyValidator : public QValidator
{
    Q_OBJECT
    public:
        PrivateKeyValidator(QObject * parent);
        QValidator::State validate(QString &valueToValidate, int &pos) const;
    private:
        std::string sBase58;
        QObject *parentObject;
};

/** "Import Private Key" dialog box */
class ImportPrivateKeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportPrivateKeyDialog(QWidget *parent = 0);
    ~ImportPrivateKeyDialog();

    void setModel(WalletModel *walletmodel);
private:
    Ui::ImportPrivateKeyDialog *ui;
    WalletModel *model;

private slots:
    void on_buttonBox_accepted();


    void on_privateKeyEdit1_textChanged(const QString &arg1);
};

#endif // IMPORTPRIVATEKEYDIALOG_H

