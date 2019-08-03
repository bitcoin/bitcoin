// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_LOOKUPTXDIALOG_H
#define BITCOIN_QT_LOOKUPTXDIALOG_H

class WalletModel;

#include <QDialog>

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

namespace Ui {
    class LookupTXDialog;
}

/** Dialog for looking up Omni Layer transactions */
class LookupTXDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LookupTXDialog(QWidget *parent = 0);
    ~LookupTXDialog();

    void setWalletModel(WalletModel *model);
    void searchTX();

public Q_SLOTS:
    void searchButtonClicked();

private:
    Ui::LookupTXDialog *ui;
    WalletModel *walletModel;

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // BITCOIN_QT_LOOKUPTXDIALOG_H
