#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"

#include <QDebug>

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    currentBalance(-1),
    currentUnconfirmedBalance(-1)
{
    ui->setupUi(this);

    // Balance: <balance>
    ui->labelBalance->setFont(QFont("Monospace", -1, QFont::Bold));
    ui->labelBalance->setToolTip(tr("Your current balance"));
    ui->labelBalance->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);

    // Balance: <balance>
    ui->labelUnconfirmed->setFont(QFont("Monospace", -1, QFont::Bold));
    ui->labelUnconfirmed->setToolTip(tr("Balance of transactions that have yet to be confirmed"));
    ui->labelUnconfirmed->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);

    ui->labelNumTransactions->setToolTip(tr("Total number of transactions in wallet"));

    // Overview page should show:
    // Last received transaction(s)
    // Last sent transaction(s)
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 unconfirmedBalance)
{
    int unit = model->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
}

void OverviewPage::setNumTransactions(int count)
{
    ui->labelNumTransactions->setText(QLocale::system().toString(count));
}

void OverviewPage::setModel(WalletModel *model)
{
    this->model = model;

    // Keep up to date with wallet
    setBalance(model->getBalance(), model->getUnconfirmedBalance());
    connect(model, SIGNAL(balanceChanged(qint64, qint64)), this, SLOT(setBalance(qint64, qint64)));

    setNumTransactions(model->getNumTransactions());
    connect(model, SIGNAL(numTransactionsChanged(int)), this, SLOT(setNumTransactions(int)));

    connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(displayUnitChanged()));
}

void OverviewPage::displayUnitChanged()
{
    if(currentBalance != -1)
        setBalance(currentBalance, currentUnconfirmedBalance);
}
