// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METADEXCANCELDIALOG_H
#define METADEXCANCELDIALOG_H

#include "clientmodel.h"
#include "walletmodel.h"

#include <QDialog>
#include <QString>
#include <QDialogButtonBox>
#include <QTextEdit>

class OptionsModel;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class MetaDExCancelDialog;
}

/** Dialog for sending Master Protocol tokens */
class MetaDExCancelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MetaDExCancelDialog(QWidget *parent = 0);

    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    QLayout *dlgLayout;
    QTextEdit *dlgTextEdit;
    QDialogButtonBox *buttonBox;
    QPushButton *closeButton;


public slots:
    void SendCancelTransaction();
    void UpdateAddressSelector();
    void UpdateCancelCombo();
    void RefreshUI();
    void fromAddressComboBoxChanged(int);


private:
    Ui::MetaDExCancelDialog *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

private slots:

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // METADEXCANCELDIALOG_H
