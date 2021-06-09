// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <qt/nftloaderdialog.h>
#include <qt/forms/ui_nftloaderdialog.h>

#include <qt/addresstablemodel.h>
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
    return;
}

void NftLoaderDialog::clear()
{
    return;
}

void NftLoaderDialog::reject()
{
    return;
}

void NftLoaderDialog::accept()
{
    return;
}

void NftLoaderDialog::showMenu(const QPoint &point)
{
	return;
}
