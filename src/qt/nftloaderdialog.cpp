// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <qt/nftloaderdialog.h>
#include <qt/forms/ui_nftloaderdialog.h>
#include <qt/nftloaderdialogoptions.h>

#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>

#include <QAction>
#include <QCursor>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>

NftLoaderDialog::NftLoaderDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::NftLoaderDialog),
    model(nullptr),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
}


void NftLoaderDialog::setModel(WalletModel *_model)
{
    this->model = _model;
	return;
}

NftLoaderDialog::~NftLoaderDialog()
{
    delete ui;
}

void NftLoaderDialog::on_createAssetClassButton_clicked()
{
    CurrentTabSelected(NftLoaderDialogOptions::TAB_CREATE_ASSET_CLASS);
}

void NftLoaderDialog::on_createAssetButton_clicked()
{
    CurrentTabSelected(NftLoaderDialogOptions::TAB_CREATE_ASSET);
}

void NftLoaderDialog::on_sendAssetButton_clicked()
{
    CurrentTabSelected(NftLoaderDialogOptions::TAB_SEND_ASSET);
}

void NftLoaderDialog::CurrentTabSelected(NftLoaderDialogOptions::Tab tab)
{
    NftLoaderDialogOptions* dialog = new NftLoaderDialogOptions(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModel(model);
    dialog->setCurrentTab(tab);
    dialog->show();
}

