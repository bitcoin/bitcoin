// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_NftLoaderDialogOptions_H
#define BITCOIN_QT_NftLoaderDialogOptions_H

#include <qt/walletmodel.h>

#include <QStackedWidget>
#include <QDialog>

class WalletModel;
class PlatformStyle;

namespace Ui
{
    class NftLoaderDialogOptions;
}

class NftLoaderDialogOptions : public QDialog
{
    Q_OBJECT

public:
    explicit NftLoaderDialogOptions(const PlatformStyle* platformStyle, QWidget* parent = nullptr);
    ~NftLoaderDialogOptions();

    enum Tab {
        TAB_CREATE_ASSET_CLASS,
        TAB_CREATE_ASSET,
        TAB_SEND_ASSET,
    };

    void setModel(WalletModel* model);
    void setCurrentTab(NftLoaderDialogOptions::Tab tab);
    bool validate(interfaces::Node& node);
    bool validateFee();

private:
    Ui::NftLoaderDialogOptions* ui;
    WalletModel* model;
    const PlatformStyle* platformStyle;

private Q_SLOTS:
    void on_createAssetClassButton_clicked();
    void on_createAssetButton_clicked();
    void on_sendAssetButton_clicked();
};

#endif // BITCOIN_QT_NftLoaderDialogOptions_H
