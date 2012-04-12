#include "bitcoingui.h"
#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "coincontrolpage.h"
#include "optionsmodel.h"
#include "sendcoinsentry.h"
#include "guiutil.h"
#include "askpassphrasedialog.h"


#include "headers.h"
#include "db.h"
#include "net.h"
#include "init.h"

#include <QApplication>
#include <QMessageBox>
#include <QLocale>
#include <QTextDocument>
#include <QTableWidget>

using namespace std;

CoinControlPage::CoinControlPage(QWidget *parent) :
    QWidget(parent)
{
    gui = (BitcoinGUI *)parent;

    QHBoxLayout *hlayout = new QHBoxLayout(this);

    QStringList headers;
    headers << "Address" << "Label" << "Balance" << "Group Balance";
    table = new QTableWidget(this);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    connect(table, SIGNAL(itemDoubleClicked(QTableWidgetItem *)), this, SLOT(sendFromSelectedAddress(QTableWidgetItem *)));

    hlayout->addWidget(table);
}

std::string CoinControlPage::selectedAddresses()
{
    std::string addresses;
    BOOST_FOREACH(QTableWidgetItem *i, table->selectedItems())
    {
        if (i->column() != 0)
            continue;

        std::string text = ((QString)i->text()).toStdString();
        if (CBitcoinAddress(text).IsValid())
            addresses += text + ";";
    }

    if (addresses.size() > 0)
        addresses.resize(addresses.size()-1);

    return addresses;
}

void CoinControlPage::clearSelection()
{
    table->clearSelection();
}

void CoinControlPage::sendFromSelectedAddress(QTableWidgetItem *item)
{
    if (selectedAddresses().size() == 0)
        return;

    gui->gotoSendCoinsPage();
}

void CoinControlPage::UpdateTable()
{
    map<string, int64> balances = pwalletMain->GetAddressBalances();
    set< set<string> > groupings = pwalletMain->GetAddressGroupings();
    vector< PAIRTYPE(int64, set<string>) > nonZeroGroupings;

    BOOST_FOREACH(set<string> addresses, groupings)
    {
        int64 balance = 0;
        BOOST_FOREACH(string address, addresses)
            balance += balances[address];
        if (balance > 0)
            nonZeroGroupings.push_back(make_pair(balance, addresses));
    }
    sort(nonZeroGroupings.begin(), nonZeroGroupings.end());

    table->setRowCount(0);

    bool first = true;
    BOOST_FOREACH(PAIRTYPE(int64, set<string>) grouping, nonZeroGroupings)
    {
        int64 group_balance = grouping.first;
        set<string> addresses = grouping.second;
        if (first)
            first = false;
        else
            table->insertRow(0);

        vector<string> sortedAddresses(addresses.begin(), addresses.end());
        sort(sortedAddresses.begin(), sortedAddresses.end(), boost::lambda::var(balances)[boost::lambda::_1] < boost::lambda::var(balances)[boost::lambda::_2]);

        int remaining = addresses.size();
        BOOST_FOREACH(string address, sortedAddresses)
        {
            int64 balance = balances[address];
            remaining--;

            table->insertRow(0);
            table->setItem(0, 0, new QTableWidgetItem(QString::fromStdString(address)));

            {
                LOCK(pwalletMain->cs_wallet);
                if (pwalletMain->mapAddressBook.find(CBitcoinAddress(address)) != pwalletMain->mapAddressBook.end())
                    table->setItem(0, 1, new QTableWidgetItem(QString::fromStdString(pwalletMain->mapAddressBook.find(CBitcoinAddress(address))->second)));
            }

            if (balance > 0)
            {
                table->setItem(0, 2, new QTableWidgetItem(QString::fromStdString(strprintf("%"PRI64d".%08"PRI64d, balance/COIN, balance%COIN))));
                if (!remaining)
                    table->setItem(0, 3, new QTableWidgetItem(QString::fromStdString(strprintf("%"PRI64d".%08"PRI64d, group_balance/COIN, group_balance%COIN))));
            }
            else
                table->setItem(0, 2, new QTableWidgetItem(QString::fromStdString("-")));
        }
    }
}
