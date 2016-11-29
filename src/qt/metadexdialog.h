// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METADEXDIALOG_H
#define METADEXDIALOG_H

#include <stdint.h>
#include <string>

#include <QDialog>

class WalletModel;
class ClientModel;

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

namespace Ui {
    class MetaDExDialog;
}

/** Dialog for looking up Master Protocol tokens */
class MetaDExDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MetaDExDialog(QWidget *parent = 0);
    ~MetaDExDialog();

    void AddRow(bool includesMe, const std::string& txid, const std::string& seller, const std::string& price, const std::string& available, const std::string& desired);
    void UpdateProperties();
    void setWalletModel(WalletModel *model);
    void setClientModel(ClientModel *model);
    void PopulateAddresses();
    uint32_t GetPropForSale();
    uint32_t GetPropDesired();

public Q_SLOTS:
    void SwitchMarket();
    void sendTrade();
    void UpdateBalance();
    void UpdateOffers();
    void BalanceOrderRefresh();
    void FullRefresh();
    void RecalcSellValues();
    void InvertPair();
    void ShowDetails();
    void ShowHistory();

private:
    Ui::MetaDExDialog *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    uint32_t global_metadex_market;

private Q_SLOTS:
    // none

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // METADEXDIALOG_H
