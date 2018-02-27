#include "obfuscationconfig.h"
#include "ui_obfuscationconfig.h"

#include "libertaunits.h"
#include "guiconstants.h"
#include "init.h"
#include "optionsmodel.h"
#include "walletmodel.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

ObfuscationConfig::ObfuscationConfig(QWidget* parent) : QDialog(parent),
                                                        ui(new Ui::ObfuscationConfig),
                                                        model(0)
{
    ui->setupUi(this);

    connect(ui->buttonBasic, SIGNAL(clicked()), this, SLOT(clickBasic()));
    connect(ui->buttonHigh, SIGNAL(clicked()), this, SLOT(clickHigh()));
    connect(ui->buttonMax, SIGNAL(clicked()), this, SLOT(clickMax()));
}

ObfuscationConfig::~ObfuscationConfig()
{
    delete ui;
}

void ObfuscationConfig::setModel(WalletModel* model)
{
    this->model = model;
}

void ObfuscationConfig::clickBasic()
{
    configure(true, 1000, 2);

    QString strAmount(LibertaUnits::formatWithUnit(
        model->getOptionsModel()->getDisplayUnit(), 1000 * COIN));
    QMessageBox::information(this, tr("Obfuscation Configuration"),
        tr(
            "Obfuscation was successfully set to basic (%1 and 2 rounds). You can change this at any time by opening LIBERTA's configuration screen.")
            .arg(strAmount));

    close();
}

void ObfuscationConfig::clickHigh()
{
    configure(true, 1000, 8);

    QString strAmount(LibertaUnits::formatWithUnit(
        model->getOptionsModel()->getDisplayUnit(), 1000 * COIN));
    QMessageBox::information(this, tr("Obfuscation Configuration"),
        tr(
            "Obfuscation was successfully set to high (%1 and 8 rounds). You can change this at any time by opening LIBERTA's configuration screen.")
            .arg(strAmount));

    close();
}

void ObfuscationConfig::clickMax()
{
    configure(true, 1000, 16);

    QString strAmount(LibertaUnits::formatWithUnit(
        model->getOptionsModel()->getDisplayUnit(), 1000 * COIN));
    QMessageBox::information(this, tr("Obfuscation Configuration"),
        tr(
            "Obfuscation was successfully set to maximum (%1 and 16 rounds). You can change this at any time by opening LIBERTA's configuration screen.")
            .arg(strAmount));

    close();
}

void ObfuscationConfig::configure(bool enabled, int coins, int rounds)
{
    QSettings settings;

    settings.setValue("nObfuscationRounds", rounds);
    settings.setValue("nAnonymizeLibertaAmount", coins);

    nObfuscationRounds = rounds;
    nAnonymizeLibertaAmount = coins;
}
