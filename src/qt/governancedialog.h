// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GOVERNANCE_DIALOG_H
#define GOVERNANCE_DIALOG_H

#include "walletmodel.h"
#include <governance/governance-vote.h>

#include <QDialog>
#include <QImage>
#include <QLabel>

class OptionsModel;
class WalletModel;

namespace Ui {
    class GovernanceDialog;
}

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE


class GovernanceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GovernanceDialog(QWidget *parent = 0);
    ~GovernanceDialog();
    void setWalletModel(WalletModel *walletModel);
    void setModel(OptionsModel *model);
    void setInfo(QString strWindowtitle, QString strQRCode, QString strTextInfo, QString strQRCodeTitle);


private:
    WalletModel *walletModel;

private Q_SLOTS:
    void update();


private:
    Ui::GovernanceDialog *ui;
    OptionsModel *model;
    uint256 gObjHash;
    QString strWindowtitle;
    QString strQRCode;
    QString strTextInfo;
    QString strQRCodeTitle;
};

#endif // GOVERNANCE_DIALOG_H
