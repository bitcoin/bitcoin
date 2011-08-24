#ifndef ASKPASSPHRASEDIALOG_H
#define ASKPASSPHRASEDIALOG_H

#include <QDialog>

namespace Ui {
    class AskPassphraseDialog;
}

class WalletModel;

class AskPassphraseDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        Encrypt,    // Ask passphrase x2
        Unlock,     // Ask passphrase
        ChangePass, // Ask old passphrase + new passphrase x2
        Decrypt   // Ask passphrase
    };

    explicit AskPassphraseDialog(Mode mode, QWidget *parent = 0);
    ~AskPassphraseDialog();

    void accept();

    void setModel(WalletModel *model);

private:
    Ui::AskPassphraseDialog *ui;
    Mode mode;
    WalletModel *model;

private slots:
    void textChanged();
};

#endif // ASKPASSPHRASEDIALOG_H
