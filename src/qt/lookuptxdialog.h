// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LOOKUPTXDIALOG_H
#define LOOKUPTXDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>
#include <QImage>
#include <QLabel>
#include <QTextEdit>
#include <QDialogButtonBox>

class OptionsModel;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class LookupTXDialog;
}

/** Dialog for looking up Master Protocol txns */
class LookupTXDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LookupTXDialog(QWidget *parent = 0);
    void searchTX();
    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);
    QDialog *txDlg;
    QLayout *dlgLayout;
    QTextEdit *dlgTextEdit;
    QDialogButtonBox *buttonBox;
    QPushButton *closeButton;

public slots:
    void searchButtonClicked();

private:
    Ui::LookupTXDialog *ui;
    WalletModel *model;

//private slots:

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // LOOKUPTXDIALOG_H
