// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "uneditablecoinsentry.h"
#include "ui_uneditablecoinsentry.h"

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"

#include <QApplication>
#include <QClipboard>

UneditableCoinsEntry::UneditableCoinsEntry(QWidget *parent) :
    QStackedWidget(parent),
    ui(new Ui::UneditableCoinsEntry),
    model(0)
{
    ui->setupUi(this);

    ui->payAmount->setReadOnly(true);

    setCurrentWidget(ui->SendCoins);
}

UneditableCoinsEntry::~UneditableCoinsEntry()
{
    delete ui;
}

void UneditableCoinsEntry::on_pasteButton_clicked()
{
}

void UneditableCoinsEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;
    Credits_AddressBookPage dlg(Credits_AddressBookPage::ForSelection, Credits_AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->payAmount->setFocus();
    }
}

void UneditableCoinsEntry::setModel(Bitcredit_WalletModel *model)
{
    this->model = model;

    if (model && model->getOptionsModel())
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    connect(ui->payAmount, SIGNAL(textChanged()), this, SIGNAL(payAmountChanged()));

    clear();
}
void UneditableCoinsEntry::setValue(qint64 value)
{
    ui->payAmount->setValue(value);
}

void UneditableCoinsEntry::clear()
{
    // clear UI elements for normal payment
    ui->payAmount->clear();
    ui->payAmount_is->clear();
    ui->payAmount_s->clear();

    // update the display unit, to not use the default ("CRE")
    updateDisplayUnit();
}

void UneditableCoinsEntry::deleteClicked()
{
    emit removeEntry(this);
}

bool UneditableCoinsEntry::validate()
{
    if (!model)
        return false;

    // Check input validity
    bool retval = true;

    if (!ui->payAmount->validate())
    {
        retval = false;
    }

    // Reject dust outputs:
    if (retval && GUIUtil::isDust(ui->payAmount->value())) {
        ui->payAmount->setValid(false);
        retval = false;
    }

    return retval;
}

Bitcredit_SendCoinsRecipient UneditableCoinsEntry::getValue()
{
    // Normal payment
    recipient.amount = ui->payAmount->value();

    return recipient;
}

QWidget *UneditableCoinsEntry::setupTabChain(QWidget *prev)
{
	return prev;
}

void UneditableCoinsEntry::setValue(const Bitcredit_SendCoinsRecipient &value)
{
    recipient = value;

	ui->payAmount->setValue(recipient.amount);
}

void UneditableCoinsEntry::setAddress(const QString &address)
{
    ui->payAmount->setFocus();
}

bool UneditableCoinsEntry::isClear()
{
	return true;
}

void UneditableCoinsEntry::setFocus()
{
}

void UneditableCoinsEntry::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update payAmount with the current unit
        ui->payAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_is->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_s->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }
}
