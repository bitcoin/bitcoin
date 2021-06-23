// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/nftloaderdialogoptions.h>
#include <qt/forms/ui_nftloaderdialogoptions.h>

#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/qrimagewidget.h>
#include <qt/walletmodel.h>

#include <QDialog>
#include <QString>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h> /* for USE_QRCODE */
#endif

NftLoaderDialogOptions::NftLoaderDialogOptions(const PlatformStyle* _platformStyle, QWidget* parent) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::NftLoaderDialogOptions),
    model(nullptr),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
    GUIUtil::handleCloseWindowShortcut(this);

    //ui->nftCreateAssetClassFee->setText("0");

    //setCurrentWidget(ui->SendCoins);

    // Connect signals
    //connect(ui->deleteButton, &QPushButton::clicked, this, &createAssetClassButto::deleteClicked);
    //connect(ui->nftCreateAssetClassFee, &BitcoinAmountField::valueChanged, this, &NftLoaderDialogOptions::payAmountChanged);
}

void NftLoaderDialogOptions::setModel(WalletModel* _model)
{
    this->model = _model;
    return;
}

NftLoaderDialogOptions::~NftLoaderDialogOptions()
{
    delete ui;
}

void NftLoaderDialogOptions::on_createAssetClassButton_clicked()
{
    //Create asset class will prompt the user for asset class metadata and a fee amount
    //The metadata is optional

    //The fee is numeric and must be 0 or positive whole number (in Atoms)
    bool valid = validateFee();
    if (valid == false) {
        QMessageBox msgBox;
        msgBox.setText("Fee should be greater than zero atom.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }

    // ****************************************** //
    //When they press send a status should read:
    // ****************************************** //
    //Sending transaction waiting for block confirmation
    //Loading asset class to NFT database
    //Transaction mined, NFT ID is XXXXXXXXXXXX

    
    
    return;
}

void NftLoaderDialogOptions::on_createAssetButton_clicked()
{
    //Create asset workflow is basically the same as create asset except the user needs to pick an asset class from the dropdown
    //Then the user can optionally enter a binary file location(e.g.c:\pictures\image.jpg)
    //The status update is the same steps as above

    return;
}

void NftLoaderDialogOptions::on_sendAssetButton_clicked()
{
    //For send asset, the user selects an asset class from the dropdown
    //Then select an asset which is filtered by the asset class(it should display the NFT ID somewhere when its picked)
    //Then enter a destination wallet address
    //When they press send it needs a warning like "this process cannot be reversed are you sure?" There's no need for status after that, the messagebox can just say "asset sent"

    return;
}

void NftLoaderDialogOptions::setCurrentTab(NftLoaderDialogOptions::Tab tab)
{
    QWidget* tab_widget = nullptr;
    if (tab == NftLoaderDialogOptions::Tab::TAB_CREATE_ASSET_CLASS) tab_widget = ui->tabCreateAssetClass;
    if (tab == NftLoaderDialogOptions::Tab::TAB_CREATE_ASSET) tab_widget = ui->tabCreateAsset;
    if (tab == NftLoaderDialogOptions::Tab::TAB_SEND_ASSET) tab_widget = ui->tabSendAsset;
    if (tab_widget && ui->tabWidget->currentWidget() != tab_widget) {
        ui->tabWidget->setCurrentWidget(tab_widget);
    }
}

bool NftLoaderDialogOptions::validate(interfaces::Node& node)
{
    if (!model)
        return false;

    // Check input validity
    bool retval = validateFee();
    
    return validateFee();
}

bool NftLoaderDialogOptions::validateFee()
{
    bool retval = true;

    if (!ui->nftCreateAssetClassFee->validate()) {
        retval = false;
    }

    // Sending a zero amount is invalid
    if (ui->nftCreateAssetClassFee->value(nullptr) <= 0) {
        ui->nftCreateAssetClassFee->setValid(false);
        retval = false;
    }

    return retval;
}
