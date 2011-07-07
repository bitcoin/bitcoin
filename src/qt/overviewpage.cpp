#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "guiutil.h"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage)
{
    ui->setupUi(this);

    // Balance: <balance>
    ui->labelBalance->setFont(QFont("Monospace", -1, QFont::Bold));
    ui->labelBalance->setToolTip(tr("Your current balance"));
    ui->labelBalance->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);

    // Overview page should show:
    // Balance
    // Unconfirmed balance
    // Last received transaction(s)
    // Last sent transaction(s)
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(qint64 balance)
{
    ui->labelBalance->setText(GUIUtil::formatMoney(balance) + QString(" BTC"));
}

void OverviewPage::setNumTransactions(int count)
{
    ui->labelNumTransactions->setText(QLocale::system().toString(count));
}
