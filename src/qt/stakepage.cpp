// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/stakepage.h>
#include <qt/forms/ui_stakepage.h>

#include <qt/addresstablemodel.h>
#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/receiverequestdialog.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/walletmodel.h>
#include <interfaces/wallet.h>
#include <interfaces/node.h>
#include <qt/transactiondescdialog.h>
#include <qt/transactionview.h>
#include <qt/sendcoinsdialog.h>
#include <amount.h>
#include <miner.h>
#include <wallet/wallet.h>

#include <QSortFilterProxyModel>
#include <QDialog>

Q_DECLARE_METATYPE(interfaces::WalletBalances)

StakePage::StakePage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StakePage),
    clientModel(nullptr),
    walletModel(nullptr),
    platformStyle(_platformStyle),
    transactionView(nullptr),
    receivedDelegationsView(nullptr),
    myDelegationsView(nullptr),
    sendCoinsDialog(nullptr),
    m_subsidy(0),
    m_networkWeight(0),
    m_expectedAnnualROI(0)
{
    ui->setupUi(this);

    if (!_platformStyle->getImagesOnButtons()) {
        ui->newAddressButton->setIcon(QIcon());
    } else {
        ui->newAddressButton->setIcon(_platformStyle->SingleColorIcon(":/icons/receiving_addresses"));
    }

    ui->checkStake->setEnabled(gArgs.GetBoolArg("-staking", DEFAULT_STAKE));
    transactionView = new TransactionView(platformStyle, this, true);
    ui->frameStakeRecords->layout()->addWidget(transactionView);

    receivedDelegationsView = new TransactionView(platformStyle, this, true);
    ui->frameTabReceivedDelegations->layout()->addWidget(receivedDelegationsView);
    myDelegationsView = new TransactionView(platformStyle, this, true, true);
    ui->frameTabMyDelegations->layout()->addWidget(myDelegationsView);
    sendCoinsDialog = new SendCoinsDialog(platformStyle, nullptr, true);
    ui->tab_create_delegation->layout()->addWidget(sendCoinsDialog);
}

StakePage::~StakePage()
{
    delete ui;
    delete transactionView;
    delete receivedDelegationsView;
    delete myDelegationsView;
    delete sendCoinsDialog;
}

void StakePage::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    if (_clientModel) {
        connect(_clientModel, &ClientModel::numBlocksChanged, this, &StakePage::numBlocksChanged);
        int height = _clientModel->node().getNumBlocks();
        ui->labelHeight->setText(QString::number(height));
        m_subsidy = _clientModel->node().getBlockSubsidy(height);
        m_networkWeight = _clientModel->node().getNetworkStakeWeight();
        m_expectedAnnualROI = _clientModel->node().getEstimatedAnnualROI();
        updateNetworkWeight();
        updateAnnualROI();
    }

    this->sendCoinsDialog->setClientModel(_clientModel);
}

void StakePage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        transactionView->setModel(model);
        transactionView->chooseType(4); // Mined blocks

        receivedDelegationsView->setModel(model);
        receivedDelegationsView->chooseType(8); // The staker sees received delegations

        myDelegationsView->setModel(model);
        myDelegationsView->chooseType(7); // My delegations to others

        ui->checkStake->setChecked(model->wallet().getEnabledStaking());
        // Set the button to be enabled or disabled based on whether the wallet can give out new addresses.
        ui->newAddressButton->setEnabled(model->wallet().canGetAddresses());

        // Keep up to date with wallet
        interfaces::Wallet& wallet = model->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();
        setBalance(balances);
        connect(model, &WalletModel::balanceChanged, this, &StakePage::setBalance);

        // Re-emit coinsSent signal from sendCoinsDialog.
        connect(sendCoinsDialog, &SendCoinsDialog::coinsSent, this, &StakePage::coinsSent);

        // Re-emit message signal from sendCoinsDialog.
        connect(sendCoinsDialog, &SendCoinsDialog::message, this, &StakePage::message);

        connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &StakePage::updateDisplayUnit);

        // Enable/disable the receive button if the wallet is now able/unable to give out new addresses.
        connect(model, &WalletModel::canGetAddressesChanged, [this] {
            ui->newAddressButton->setEnabled(walletModel->wallet().canGetAddresses());
        });

        connect(model->getOptionsModel(), &OptionsModel::coldStakerControlFeaturesChanged, this, &StakePage::coldStakerControlFeaturesChanged);
        coldStakerControlFeaturesChanged(model->getOptionsModel()->getColdStakerControlFeatures());
    }

    this->sendCoinsDialog->setModel(model);

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void StakePage::setBalance(const interfaces::WalletBalances& balances)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    m_balances = balances;
    ui->labelAssets->setText(BitcoinUnits::formatWithUnit(unit, balances.stakeable, false, BitcoinUnits::SeparatorStyle::ALWAYS));
    ui->labelOwnedAssets->setText(BitcoinUnits::formatWithUnit(unit, balances.stakeable - balances.stakeable_delegations, false, BitcoinUnits::SeparatorStyle::ALWAYS));
    ui->labelDelegations->setText(BitcoinUnits::formatWithUnit(unit, balances.stakeable_delegations, false, BitcoinUnits::SeparatorStyle::ALWAYS));
    ui->labelAssetsImmature->setText(BitcoinUnits::formatWithUnit(unit, balances.immature_stakeable, false, BitcoinUnits::SeparatorStyle::ALWAYS));
    ui->labelAssetsImmature->setVisible(balances.immature_stakeable != 0);
    ui->labelAssetsImmatureText->setVisible(balances.immature_stakeable != 0);
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, balances.stake, false, BitcoinUnits::SeparatorStyle::ALWAYS));
    ui->labelDelegatedAssets->setText(BitcoinUnits::formatWithUnit(unit, balances.delegated + balances.immature_delegated, false, BitcoinUnits::SeparatorStyle::ALWAYS));
}

void StakePage::on_checkStake_clicked(bool checked)
{
    if(!walletModel)
        return;

    this->walletModel->wallet().setEnabledStaking(checked);

    if(checked && WalletModel::Locked == walletModel->getEncryptionStatus())
        Q_EMIT requireUnlock(true);
}

void StakePage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if (m_balances.balance != -1) {
            setBalance(m_balances);
        }
        updateSubsidy();
    }
}

void StakePage::numBlocksChanged(int count, const QDateTime &, double, bool headers)
{
    if(!headers && clientModel && walletModel)
    {
        ui->labelHeight->setText(BitcoinUnits::formatInt(count));
        m_subsidy = clientModel->node().getBlockSubsidy(count);
        m_networkWeight = clientModel->node().getNetworkStakeWeight();
        m_expectedAnnualROI = clientModel->node().getEstimatedAnnualROI();
        updateSubsidy();
        updateNetworkWeight();
        updateAnnualROI();
    }
}

void StakePage::updateSubsidy()
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    QString strSubsidy = BitcoinUnits::formatWithUnit(unit, m_subsidy, false, BitcoinUnits::SeparatorStyle::ALWAYS) + "/Block";
    ui->labelReward->setText(strSubsidy);
}

void StakePage::updateNetworkWeight()
{
    ui->labelWeight->setText(BitcoinUnits::formatInt(m_networkWeight / COIN));
}

void StakePage::updateAnnualROI()
{
    ui->labelROI->setText(QString::number(m_expectedAnnualROI, 'f', 2) + "%");
}

void StakePage::updateEncryptionStatus()
{
    if(!walletModel)
        return;

    int status = walletModel->getEncryptionStatus();
    switch(status)
    {
    case WalletModel::Locked:
        bool checked = ui->checkStake->isChecked();
        if(checked) ui->checkStake->onStatusChanged();
        break;
    }
}

void StakePage::on_newAddressButton_clicked()
{
    if(!walletModel || !walletModel->getOptionsModel() || !walletModel->getAddressTableModel() || !walletModel->getRecentRequestsTableModel())
        return;

    QString address;
    QString label = ui->newAddressLabel->text();
    /* Generate new staking address */
    OutputType address_type = OutputType::LEGACY;
    address = walletModel->getAddressTableModel()->addRow(AddressTableModel::Receive, label, "", address_type);

    switch(walletModel->getAddressTableModel()->getEditStatus())
    {
    case AddressTableModel::EditStatus::OK: {
        // Success
        SendCoinsRecipient info(address, label, 0, "");
        ReceiveRequestDialog *dialog = new ReceiveRequestDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setModel(walletModel);
        dialog->setInfo(info);
        dialog->show();
        break;
    }
    case AddressTableModel::EditStatus::WALLET_UNLOCK_FAILURE:
        QMessageBox::critical(this, windowTitle(),
            tr("Could not unlock wallet."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case AddressTableModel::EditStatus::KEY_GENERATION_FAILURE:
        QMessageBox::critical(this, windowTitle(),
            tr("Could not generate new %1 address").arg(QString::fromStdString(FormatOutputType(address_type))),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    // These aren't valid return values for our action
    case AddressTableModel::EditStatus::INVALID_ADDRESS:
    case AddressTableModel::EditStatus::DUPLICATE_ADDRESS:
    case AddressTableModel::EditStatus::NO_CHANGES:
        assert(false);
    }
    ui->newAddressLabel->setText("");
}

// ColdStake Control: settings menu - cold staker control enabled/disabled by user
void StakePage::coldStakerControlFeaturesChanged(bool checked)
{
    int index = ui->tabWidget->indexOf(ui->tab_received_delegations);

    if (checked && index == -1)
    {
        ui->tabWidget->addTab(ui->tab_received_delegations, "Received Delegations");
    }
    else if (!checked && index != -1)
    {
        ui->tabWidget->removeTab(index);
    }
}
