// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_NFTLOADERDIALOG_H
#define BITCOIN_QT_NFTLOADERDIALOG_H

#include <qt/guiutil.h>

#include <QDialog>
#include <QHeaderView>
#include <QItemSelection>
#include <QKeyEvent>
#include <QMenu>
#include <QPoint>
#include <QVariant>

#include <qt/nftloaderdialogoptions.h>

class PlatformStyle;
class WalletModel;

namespace Ui {
    class NftLoaderDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog for requesting payment of bitcoins */
class NftLoaderDialog : public QDialog
{
    Q_OBJECT

public:

    explicit NftLoaderDialog(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~NftLoaderDialog();

    void setModel(WalletModel *model);

public Q_SLOTS:
    void on_createAssetClassButton_clicked();
    void on_createAssetButton_clicked();
    void on_sendAssetButton_clicked();
    void CurrentTabSelected(NftLoaderDialogOptions::Tab tab);

private:
    Ui::NftLoaderDialog *ui;
    WalletModel *model;
    QMenu *contextMenu;
    const PlatformStyle *platformStyle;

};

#endif // BITCOIN_QT_NftLoaderDialog_H
