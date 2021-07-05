// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <qt/nftloaderdialogoptions.h>
#include <qt/forms/ui_nftloaderdialogoptions.h>
#include <primitives/dynnft_manager.h>

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
#include <primitives\dynnft_manager.cpp>
#include <QSettings>
#include <QFileDialog>

NftLoaderDialogOptions::NftLoaderDialogOptions(const PlatformStyle* _platformStyle, QWidget* parent) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::NftLoaderDialogOptions),
    model(nullptr),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
    GUIUtil::handleCloseWindowShortcut(this);

    // init transaction fee section
    ui->nftCreateAssetClassFee->setDisplayUnit(1); //1 = Atom
    ui->nftCreateAssetFee->setDisplayUnit(1);      //1 = Atom
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
    if (valid == false)
    {
        QMessageBox msgBox;
        msgBox.setText("Fee should be greater than zero atom.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
    else
    {
        // ****************************************************************************** //
        //                 When they press send a status should read:
        // ****************************************************************************** //

        // ****************************************************************************** //
        ui->labelStatus->setText("Sending transaction waiting for block confirmation.");
        // ****************************************************************************** //
        //net_processing.h and net_processing.cpp
        CNFTAssetClass* assetClass = new CNFTAssetClass();
        assetClass->txnID = "";
        assetClass->hash = "";
        assetClass->metaData = ui->nftCreateAssetClassMetadata->toPlainText().toStdString();
        assetClass->owner = "";
        assetClass->maxCount = 1;

        // ****************************************************************************** //
        ui->labelStatus->setText("Loading asset class to NFT database.");
        // ****************************************************************************** //
        cnftManager->addNFTAssetClass(assetClass);

        // ****************************************************************************** //
        //           2 - hash1 (binary) + txid (in binary) = NFTID
        // ****************************************************************************** //
        cnftManager->addAssetClassToCache(assetClass);

        ui->labelStatus->setText("Transaction mined, NFT ID is XXXXXXXXXXXX.");

        assetClassCurrent = assetClass;

        assetClassCurrent->txnID = "XXXXXXXXXXXX";
        // ****************************************************************************** //
    }
    
    return;
}

void NftLoaderDialogOptions::on_ellipsisButton_clicked()
{
    QString dir = QDir::toNativeSeparators(QFileDialog::getOpenFileName(nullptr, "Choose data file", ui->nftCreateAssetBinary->text(),"*.jpg; *.png"));
    if (!dir.isEmpty())
        ui->nftCreateAssetBinary->setText(dir);
}

QString NftLoaderDialogOptions::getAssetBinaryFile()
{
    return ui->nftCreateAssetBinary->text();
}

void NftLoaderDialogOptions::setAssetBinaryFile(const QString& dataDir)
{
    ui->nftCreateAssetBinary->setText(dataDir);
    if (dataDir == GUIUtil::getDefaultDataDirectory()) {
        ui->nftCreateAssetBinary->setEnabled(false);
        ui->ellipsisButton->setEnabled(false);
    } else {
        ui->nftCreateAssetBinary->setEnabled(true);
        ui->ellipsisButton->setEnabled(true);
    }
}

void NftLoaderDialogOptions::on_createAssetButton_clicked()
{
    //Create asset workflow is basically the same as create asset except the user needs to pick an asset class from the dropdown
    //Then the user can optionally enter a binary file location(e.g.c:\pictures\image.jpg)
    //The status update is the same steps as above

    bool valid = validateFee2();
    if (valid == false) {
        QMessageBox msgBox;
        msgBox.setText("Fee should be greater than zero atom.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    } else {
        // ****************************************************************************** //
        //                 When they press send a status should read:
        // ****************************************************************************** //

        // ****************************************************************************** //
        //ui->labelStatus->setText("Sending transaction waiting for block confirmation.");
        // ****************************************************************************** //
        CNFTAsset* asset = new CNFTAsset();

        asset->txnID = assetClassCurrent->txnID;
        asset->hash = "";
        asset->assetClassHash = assetClassCurrent->hash;
        asset->metaData = ui->nftCreateAssetMetadata->toPlainText().toStdString();
        asset->binaryData = ui->nftCreateAssetBinary->text().toStdString();
        asset->owner = "";
        asset->serial = 1;

        // ****************************************************************************** //
        //ui->labelStatus->setText("Loading asset to NFT database.");
        // ****************************************************************************** //
        cnftManager->addNFTAsset(asset);

        // ****************************************************************************** //
        //           2 - hash1 (binary) + txid (in binary) = NFTID
        // ****************************************************************************** //
        cnftManager->addAssetToCache(asset);

        //ui->labelStatus->setText("Transaction mined, NFT ID is XXXXXXXXXXXX.");
        asset->hash = "";
        assetCurrent = asset;
        // ****************************************************************************** //
    }

    return;
}

void NftLoaderDialogOptions::on_sendAssetButton_clicked()
{
    //For send asset, the user selects an asset class from the dropdown
    //Then select an asset which is filtered by the asset class(it should display the NFT ID somewhere when its picked)
    //Then enter a destination wallet address
    //When they press send it needs a warning like "this process cannot be reversed are you sure?" There's no need for status after that, the messagebox can just say "asset sent"

    // ****************************************************************************** //
    //                 When they press send a status should read:
    // ****************************************************************************** //

    // ****************************************************************************** //
    QMessageBox::StandardButton retval =
        QMessageBox::question(this, tr("Confirm sending of asset"),
                            tr("This process cannot be reversed are you sure?"),
                            QMessageBox::Yes | QMessageBox::Cancel,
                            QMessageBox::Cancel);
    if (retval == QMessageBox::Yes)
    {
        //assetClassCurrent = assetClass;
        //assetClassCurrent = assetClass;
        //ui->nftSendAssetClassBinary->currentText().toStdString();
        //ui->nftSendAssetBinary->currentText().toStdString();
        //ui->nftSendAssetClassAddress->text().toStdString();
        
        // ****************************************************************************** //

        // ****************************************************************************** //
        //When they press send it needs a warning like "this process cannot be reversed are you sure?" There's no need for status after that, the messagebox can just say "asset sent"
        // ****************************************************************************** //
        QMessageBox msgBox;
        msgBox.setText("Asset sent.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }    

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

bool NftLoaderDialogOptions::validate2(interfaces::Node& node)
{
    if (!model)
        return false;

    // Check input validity
    bool retval = validateFee2();

    return validateFee2();
}

bool NftLoaderDialogOptions::validateFee2()
{
    bool retval = true;

    if (!ui->nftCreateAssetFee->validate()) {
        retval = false;
    }

    // Sending a zero amount is invalid
    if (ui->nftCreateAssetFee->value(nullptr) <= 0) {
        ui->nftCreateAssetFee->setValid(false);
        retval = false;
    }

    return retval;
}
