// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ASKPASSPHRASEDIALOG_H
#define BITCOIN_ASKPASSPHRASEDIALOG_H

#include <QDialog>

class Bitcoin_WalletModel;

namespace Ui {
    class Bitcoin_AskPassphraseDialog;
}

/** Multifunctional dialog to ask for passphrases. Used for encryption, unlocking, and changing the passphrase.
 */
class Bitcoin_AskPassphraseDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        Encrypt,    /**< Ask passphrase twice and encrypt */
        Unlock,     /**< Ask passphrase and unlock */
        ChangePass, /**< Ask old passphrase + new passphrase twice */
        Decrypt     /**< Ask passphrase and decrypt wallet */
    };

    explicit Bitcoin_AskPassphraseDialog(Mode mode, QWidget *parent);
    ~Bitcoin_AskPassphraseDialog();

    void accept();

    void setModel(Bitcoin_WalletModel *model);

private:
    Ui::Bitcoin_AskPassphraseDialog *ui;
    Mode mode;
    Bitcoin_WalletModel *model;
    bool fCapsLock;

private slots:
    void textChanged();

protected:
    bool event(QEvent *event);
    bool eventFilter(QObject *object, QEvent *event);
};

#endif // ASKPASSPHRASEDIALOG_H
