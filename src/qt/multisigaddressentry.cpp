#include <qt/multisigaddressentry.h>
#include <qt/forms/ui_multisigaddressentry.h>

#include <qt/guiutil.h>
#include <qt/addressbookpage.h>
#include <qt/walletmodel.h>
#include <qt/addresstablemodel.h>

#include <utilstrencodings.h>

#include <QClipboard>

MultisigAddressEntry::MultisigAddressEntry(QWidget *parent) : QFrame(parent), ui(new Ui::MultisigAddressEntry), model(0)
{
    ui->setupUi(this);
    GUIUtil::setupAddressWidget(ui->address, this);
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
    ui->address->clear();
    ui->label->clear();
    ui->pubkey->setFocus();
}

bool MultisigAddressEntry::validate()
{
    return !ui->pubkey->text().isEmpty();
}

QString MultisigAddressEntry::getPubkey()
{
    return ui->pubkey->text();
}

void MultisigAddressEntry::setRemoveEnabled(bool enabled)
{
    ui->deleteButton->setEnabled(enabled);
}

void MultisigAddressEntry::on_pasteButton_clicked()
{
    ui->address->setText(QApplication::clipboard()->text());
}

void MultisigAddressEntry::on_deleteButton_clicked()
{
    Q_EMIT removeEntry(this);
}

void MultisigAddressEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;

    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection, AddressBookPage::ReceivingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->address->setText(dlg.getReturnValue());
    }
}

void MultisigAddressEntry::on_pubkey_textChanged(const QString &pubkey)
{
    // Compute address from public key
    std::vector<unsigned char> vchPubKey(ParseHex(pubkey.toStdString()));
    CPubKey pkey(vchPubKey);
    std::string address = EncodeDestination(pkey.GetID());
    ui->address->setText(address.c_str());

    if(!model)
        return;

    // Get label of address
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address.c_str());
    if(!associatedLabel.isEmpty())
        ui->label->setText(associatedLabel);
}

void MultisigAddressEntry::on_address_textChanged(const QString &address)
{
    if(!model)
        return;

    // Get public key of address
    CTxDestination dest = DecodeDestination(address.toStdString());
    const CKeyID *keyID = boost::get<CKeyID>(&dest);
    if(!keyID)
    {
        CPubKey vchPubKey;
        model->getPubKey(*keyID, vchPubKey);
        std::string pubkey = HexStr(vchPubKey);
        if(!pubkey.empty())
            ui->pubkey->setText(pubkey.c_str());
    }

    // Get label of address
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
        ui->label->setText(associatedLabel);
}
