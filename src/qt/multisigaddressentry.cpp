// Copyright (c) 2015-2016 Silk Network Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QClipboard>
#include <string>
#include <vector>

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "base58.h"
#include "guiutil.h"
#include "key.h"
#include "multisigaddressentry.h"
#include "ui_multisigaddressentry.h"
#include "walletmodel.h"
#include "utilstrencodings.h"

MultisigAddressEntry::MultisigAddressEntry(QWidget *parent) : QFrame(parent), ui(new Ui::MultisigAddressEntry), model(0)
{
    ui->setupUi(this);
    GUIUtil::setupAddressWidget(ui->walletaddress, this);
}

MultisigAddressEntry::~MultisigAddressEntry()
{
    delete ui;
}

void MultisigAddressEntry::setModel(WalletModel *model)
{
    this->model = model;
    clear();
}

void MultisigAddressEntry::clear()
{
    ui->pubkey->clear();
    ui->walletaddress->clear();
    ui->label->clear();
    ui->pubkey->setFocus();
}

bool MultisigAddressEntry::validate()
{
    return !ui->pubkey->text().isEmpty();
}

QString MultisigAddressEntry::getPubKey()
{
    return ui->pubkey->text();
}

void MultisigAddressEntry::setRemoveEnabled(bool enabled)
{
    ui->deleteButton->setEnabled(enabled);
}

void MultisigAddressEntry::on_pasteButton_clicked()
{
    ui->walletaddress->setText(QApplication::clipboard()->text());
}

void MultisigAddressEntry::on_deleteButton_clicked()
{
    Q_EMIT removeEntry(this);
}

void MultisigAddressEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::ReceivingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->walletaddress->setText(dlg.getReturnValue());
    }
}

void MultisigAddressEntry::on_walletaddress_textChanged(const QString &address)
{
    // Get public key of address
    CBitcoinAddress addr(address.toStdString().c_str());
    CKeyID keyID;
    if(addr.GetKeyID(keyID))
    {
        CPubKey vchPubKey;
        model->getPubKey(keyID, vchPubKey);
        std::string pubkey = HexStr(vchPubKey.Raw());
        if(!pubkey.empty())
            ui->pubkey->setText(pubkey.c_str());
    }

    // Get label of address
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
        ui->label->setText(associatedLabel);
}

void MultisigAddressEntry::on_pubkey_textChanged(const QString &pubkey)
{
    if(!model)
        return;

    // Compute address from public key
    std::vector<unsigned char> vchPubKey(ParseHex(pubkey.toStdString().c_str()));
    CPubKey pkey(vchPubKey);
    CKeyID keyID = pkey.GetID();
    CBitcoinAddress address(keyID);
    ui->walletaddress->setText(address.ToString().c_str());

    // Get label of address
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address.ToString().c_str());
    if(!associatedLabel.isEmpty())
        ui->label->setText(associatedLabel);
    else
        ui->label->setText(QString());
}
