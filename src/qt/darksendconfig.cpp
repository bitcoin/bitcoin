#include "darksendconfig.h"
#include "ui_darksendconfig.h"

#include "syscoinunits.h"
#include "guiconstants.h"
#include "optionsmodel.h"
#include "privatesend-client.h"
#include "walletmodel.h"

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

    connect(ui->buttonBasic, SIGNAL(clicked()), this, SLOT(clickBasic()));
    connect(ui->buttonHigh, SIGNAL(clicked()), this, SLOT(clickHigh()));
    connect(ui->buttonMax, SIGNAL(clicked()), this, SLOT(clickMax()));
}

DarksendConfig::~DarksendConfig()
{
    delete ui;
}

void DarksendConfig::setModel(WalletModel *model)
{
    this->model = model;
}

void DarksendConfig::clickBasic()
{
    configure(true, 1000, 2);

    QString strAmount(SyscoinUnits::formatWithUnit(
        model->getOptionsModel()->getDisplayUnit(), 100000 * COIN));
    QMessageBox::information(this, tr("PrivateSend Configuration"),
        tr(
            "PrivateSend was successfully set to basic (%1 and 2 rounds). You can change this at any time by opening Syscoin's configuration screen."
        ).arg(strAmount)
    );

    close();
}

void DarksendConfig::clickHigh()
{
    configure(true, 1000, 8);

    QString strAmount(SyscoinUnits::formatWithUnit(
        model->getOptionsModel()->getDisplayUnit(), 100000 * COIN));
    QMessageBox::information(this, tr("PrivateSend Configuration"),
        tr(
            "PrivateSend was successfully set to high (%1 and 8 rounds). You can change this at any time by opening Syscoin's configuration screen."
        ).arg(strAmount)
    );

    close();
}

void DarksendConfig::clickMax()
{
    configure(true, 1000, 16);

    QString strAmount(SyscoinUnits::formatWithUnit(
        model->getOptionsModel()->getDisplayUnit(), 100000 * COIN));
    QMessageBox::information(this, tr("PrivateSend Configuration"),
        tr(
            "PrivateSend was successfully set to maximum (%1 and 16 rounds). You can change this at any time by opening Syscoin's configuration screen."
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
