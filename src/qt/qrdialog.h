// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_QRDIALOG_H
#define BITCOIN_QT_QRDIALOG_H

#include <qt/walletmodel.h>

#include <QDialog>

namespace Ui {
    class QRDialog;
}

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

class QRDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QRDialog(QWidget *parent = 0);
    ~QRDialog();

    void setInfo(QString strWindowtitle, QString strQRCode, QString strTextInfo, QString strQRCodeTitle);

private Q_SLOTS:
    void update();

private:
    Ui::QRDialog *ui;
    QString strWindowtitle;
    QString strQRCode;
    QString strTextInfo;
    QString strQRCodeTitle;
};

#endif // BITCOIN_QT_QRDIALOG_H
