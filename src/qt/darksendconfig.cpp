#include <qt/darksendconfig.h>
#include <qt/forms/ui_darksendconfig.h>

#include <qt/bitcoinunits.h>
#include <qt/guiconstants.h>
#include <qt/optionsmodel.h>
#include <wallet/privatesend_client.h>
#include <qt/walletmodel.h>

#include <QMessageBox>
#include <QPushButton>
#include <QKeyEvent>
#include <QSettings>

DarksendConfig::DarksendConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DarksendConfig),
    model(0)
{
    ui->setupUi(this);

    connect(ui->buttonBasic, &QPushButton::clicked, this, &DarksendConfig::clickBasic);
    connect(ui->buttonHigh, &QPushButton::clicked, this, &DarksendConfig::clickHigh);
    connect(ui->buttonMax, &QPushButton::clicked, this, &DarksendConfig::clickMax);
}

DarksendConfig::~DarksendConfig()
{
    delete ui;
}

void DarksendConfig::setModel(WalletModel *_model)
{
    this->model = _model;
}

void DarksendConfig::clickBasic()
{
    configure(true, 1000, 2);

    QString strAmount(BitcoinUnits::formatWithUnit(
        model->getOptionsModel()->getDisplayUnit(), 1000 * COIN));
    QMessageBox::information(this, tr("PrivateSend Configuration"),
        tr(
            "PrivateSend was successfully set to basic (%1 and 2 rounds). You can change this at any time by opening Chaincoin's configuration screen."
        ).arg(strAmount)
    );

    close();
}

void DarksendConfig::clickHigh()
{
    configure(true, 1000, 8);

    QString strAmount(BitcoinUnits::formatWithUnit(
        model->getOptionsModel()->getDisplayUnit(), 1000 * COIN));
    QMessageBox::information(this, tr("PrivateSend Configuration"),
        tr(
            "PrivateSend was successfully set to high (%1 and 8 rounds). You can change this at any time by opening Chaincoin's configuration screen."
        ).arg(strAmount)
    );

    close();
}

void DarksendConfig::clickMax()
{
    configure(true, 1000, 16);

    QString strAmount(BitcoinUnits::formatWithUnit(
        model->getOptionsModel()->getDisplayUnit(), 1000 * COIN));
    QMessageBox::information(this, tr("PrivateSend Configuration"),
        tr(
            "PrivateSend was successfully set to maximum (%1 and 16 rounds). You can change this at any time by opening Chaincoin's configuration screen."
        ).arg(strAmount)
    );

    close();
}

void DarksendConfig::configure(bool enabled, int coins, int rounds) {

    QSettings settings;

    settings.setValue("nPrivateSendRounds", rounds);
    settings.setValue("nPrivateSendAmount", coins);

    privateSendClient.nPrivateSendRounds = rounds;
    privateSendClient.nPrivateSendAmount = coins;
}
