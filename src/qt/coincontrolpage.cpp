#include "coincontrolpage.h"
#include "ui_coincontrolpage.h"

#include "base58.h"
#include "bitcoingui.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "init.h"
#include "optionsmodel.h"
#include "uint256.h"
#include "wallet.h"

#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>

#include <QFont>
#include <QTableWidget>

CoinControlPage::CoinControlPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CoinControlPage),
    gui((BitcoinGUI *)parent),
    model(0)
{
    ui->setupUi(this);

    // Address
    ui->table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    // Label
    ui->table->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
    // Balance
    ui->table->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
    // Group Balance
    ui->table->horizontalHeader()->setResizeMode(3, QHeaderView::ResizeToContents);

    connect(ui->table, SIGNAL(itemDoubleClicked(QTableWidgetItem *)), this, SLOT(sendFromSelectedAddress(QTableWidgetItem *)));
}

CoinControlPage::~CoinControlPage()
{
    delete ui;
}

void CoinControlPage::setModel(OptionsModel *model)
{
    this->model = model;

    if (model)
        connect(model, SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
}

std::string CoinControlPage::selectedAddresses()
{
    std::string addresses;
    BOOST_FOREACH(QTableWidgetItem *i, ui->table->selectedItems())
    {
        if (i->column() != 0)
            continue;

        std::string text = ((QString)i->text()).toStdString();
        if (CBitcoinAddress(text).IsValid())
            addresses += text + ";";
    }

    if (addresses.size() > 0)
        addresses.resize(addresses.size() - 1);

    return addresses;
}

void CoinControlPage::clearSelection()
{
    ui->table->clearSelection();
}

void CoinControlPage::sendFromSelectedAddress(QTableWidgetItem *item)
{
    if (selectedAddresses().size() == 0)
        return;

    gui->gotoSendCoinsPage();
}

void CoinControlPage::UpdateTable()
{
    QFont bcAddressFont(GUIUtil::bitcoinAddressFont());
    int unit = 0;
    if (model)
        unit = model->getDisplayUnit();

    std::map<std::string, int64> balances = pwalletMain->GetAddressBalances();
    std::set< std::set<std::string> > groupings = pwalletMain->GetAddressGroupings();
    std::vector< PAIRTYPE(int64, std::set<std::string>) > nonZeroGroupings;

    BOOST_FOREACH(std::set<std::string> addresses, groupings)
    {
        int64 balance = 0;
        BOOST_FOREACH(std::string address, addresses)
            balance += balances[address];
        if (balance > 0)
            nonZeroGroupings.push_back(make_pair(balance, addresses));
    }
    sort(nonZeroGroupings.begin(), nonZeroGroupings.end());

    ui->table->setRowCount(0);

    bool first = true;
    BOOST_FOREACH(PAIRTYPE(int64, std::set<std::string>) grouping, nonZeroGroupings)
    {
        int64 group_balance = grouping.first;
        std::set<std::string> addresses = grouping.second;
        if (first)
            first = false;
        else
            ui->table->insertRow(0);

        std::vector<std::string> sortedAddresses(addresses.begin(), addresses.end());
        sort(sortedAddresses.begin(), sortedAddresses.end(), boost::lambda::var(balances)[boost::lambda::_1] < boost::lambda::var(balances)[boost::lambda::_2]);

        int remaining = addresses.size();
        BOOST_FOREACH(std::string address, sortedAddresses)
        {
            int64 balance = balances[address];
            remaining--;

            ui->table->insertRow(0);
            ui->table->setItem(0, 0, new QTableWidgetItem(QString::fromStdString(address)));
            ui->table->item(0, 0)->setFont(bcAddressFont);

            {
                LOCK(pwalletMain->cs_wallet);
                if (pwalletMain->mapAddressBook.find(CBitcoinAddress(address).Get()) != pwalletMain->mapAddressBook.end())
                {
                    QString strLabel = QString::fromStdString(pwalletMain->mapAddressBook.find(CBitcoinAddress(address).Get())->second);
                    // if no label is set use the same default label tha's used in addresstablemodel.cpp
                    if (strLabel.isEmpty())
                        strLabel = tr("(no label)");
                    ui->table->setItem(0, 1, new QTableWidgetItem(strLabel));
                }
            }

            if (balance > 0)
            {
                ui->table->setItem(0, 2, new QTableWidgetItem(BitcoinUnits::formatWithUnit(unit, balance)));
                ui->table->item(0, 2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                if (!remaining)
                {
                    ui->table->setItem(0, 3, new QTableWidgetItem(BitcoinUnits::formatWithUnit(unit, group_balance)));
                    ui->table->item(0, 3)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                }
            }
            else
                ui->table->setItem(0, 2, new QTableWidgetItem(QString::fromStdString("-")));
        }
    }
}

void CoinControlPage::updateDisplayUnit()
{
    // Update table to update Balance and Group Balance columns with the current unit
    UpdateTable();
}
