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

    void SwitchMarket();
    void AddRow(bool useBuyList, bool includesMe, const std::string& price, const std::string& available, const std::string& total);
    void UpdateSellAddressBalance();
    void UpdateBuyAddressBalance();
    void UpdateOffers();
    void UpdateSellOffers();
    void UpdateBuyOffers();
    void setWalletModel(WalletModel *model);
    void setClientModel(ClientModel *model);
    void recalcTotal(bool useBuyFields);

public slots:
    void switchButtonClicked();
    void sellAddressComboBoxChanged(int idx);
    void buyAddressComboBoxChanged(int idx);
    void sellClicked(int row, int col);
    void buyClicked(int row, int col);
    void sendTrade(bool sell);
    void OrderRefresh();
    void UpdateBalances();
    void FullRefresh();

private:
    Ui::MetaDExDialog *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    uint32_t global_metadex_market;

private slots:
    void buyTrade();
    void sellTrade();
    void recalcBuyTotal();
    void recalcSellTotal();

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // METADEXDIALOG_H
