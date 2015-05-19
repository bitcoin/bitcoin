// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addresstablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "base58.h"
#include "wallet.h"

#include <QFont>
#include <QDebug>

const QString Bitcredit_AddressTableModel::Send = "S";
const QString Bitcredit_AddressTableModel::Receive = "R";

struct Bitcredit_AddressTableEntry
{
    enum Type {
        Sending,
        Receiving,
        Hidden /* QSortFilterProxyModel will filter these out */
    };

    Type type;
    QString label;
    QString address;

    Bitcredit_AddressTableEntry() {}
    Bitcredit_AddressTableEntry(Type type, const QString &label, const QString &address):
        type(type), label(label), address(address) {}
};

struct Bitcredit_AddressTableEntryLessThan
{
    bool operator()(const Bitcredit_AddressTableEntry &a, const Bitcredit_AddressTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const Bitcredit_AddressTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const Bitcredit_AddressTableEntry &b) const
    {
        return a < b.address;
    }
};

/* Determine address type from address purpose */
static Bitcredit_AddressTableEntry::Type translateTransactionType(const QString &strPurpose, bool isMine)
{
    Bitcredit_AddressTableEntry::Type addressType = Bitcredit_AddressTableEntry::Hidden;
    // "refund" addresses aren't shown, and change addresses aren't in mapAddressBook at all.
    if (strPurpose == "send")
        addressType = Bitcredit_AddressTableEntry::Sending;
    else if (strPurpose == "receive")
        addressType = Bitcredit_AddressTableEntry::Receiving;
    else if (strPurpose == "unknown" || strPurpose == "") // if purpose not set, guess
        addressType = (isMine ? Bitcredit_AddressTableEntry::Receiving : Bitcredit_AddressTableEntry::Sending);
    return addressType;
}

// Private implementation
class Bitcredit_AddressTablePriv
{
public:
    Credits_CWallet *wallet;
    QList<Bitcredit_AddressTableEntry> cachedAddressTable;
    Bitcredit_AddressTableModel *parent;

    Bitcredit_AddressTablePriv(Credits_CWallet *wallet, Bitcredit_AddressTableModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshAddressTable()
    {
        cachedAddressTable.clear();
        {
            LOCK(wallet->cs_wallet);
            BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, wallet->mapAddressBook)
            {
                const CBitcoinAddress& address = item.first;
                bool fMine = IsMine(*wallet, address.Get());
                Bitcredit_AddressTableEntry::Type addressType = translateTransactionType(
                        QString::fromStdString(item.second.purpose), fMine);
                const std::string& strName = item.second.name;
                cachedAddressTable.append(Bitcredit_AddressTableEntry(addressType,
                                  QString::fromStdString(strName),
                                  QString::fromStdString(address.ToString())));
            }
        }
        // qLowerBound() and qUpperBound() require our cachedAddressTable list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        qSort(cachedAddressTable.begin(), cachedAddressTable.end(), Bitcredit_AddressTableEntryLessThan());
    }

    void updateEntry(const QString &address, const QString &label, bool isMine, const QString &purpose, int status)
    {
        // Find address / label in model
        QList<Bitcredit_AddressTableEntry>::iterator lower = qLowerBound(
            cachedAddressTable.begin(), cachedAddressTable.end(), address, Bitcredit_AddressTableEntryLessThan());
        QList<Bitcredit_AddressTableEntry>::iterator upper = qUpperBound(
            cachedAddressTable.begin(), cachedAddressTable.end(), address, Bitcredit_AddressTableEntryLessThan());
        int lowerIndex = (lower - cachedAddressTable.begin());
        int upperIndex = (upper - cachedAddressTable.begin());
        bool inModel = (lower != upper);
        Bitcredit_AddressTableEntry::Type newEntryType = translateTransactionType(purpose, isMine);

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qDebug() << "Bitcredit_AddressTablePriv::updateEntry : Warning: Got CT_NEW, but entry is already in model";
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedAddressTable.insert(lowerIndex, Bitcredit_AddressTableEntry(newEntryType, label, address));
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qDebug() << "Bitcredit_AddressTablePriv::updateEntry : Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            lower->type = newEntryType;
            lower->label = label;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qDebug() << "Bitcredit_AddressTablePriv::updateEntry : Warning: Got CT_DELETED, but entry is not in model";
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedAddressTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedAddressTable.size();
    }

    Bitcredit_AddressTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedAddressTable.size())
        {
            return &cachedAddressTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

Bitcredit_AddressTableModel::Bitcredit_AddressTableModel(Credits_CWallet *wallet, Bitcredit_WalletModel *parent) :
    QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0)
{
    columns << tr("Label") << tr("Address");
    priv = new Bitcredit_AddressTablePriv(wallet, this);
    priv->refreshAddressTable();
}

Bitcredit_AddressTableModel::~Bitcredit_AddressTableModel()
{
    delete priv;
}

int Bitcredit_AddressTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int Bitcredit_AddressTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant Bitcredit_AddressTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    Bitcredit_AddressTableEntry *rec = static_cast<Bitcredit_AddressTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Label:
            if(rec->label.isEmpty() && role == Qt::DisplayRole)
            {
                return tr("(no label)");
            }
            else
            {
                return rec->label;
            }
        case Address:
            return rec->address;
        }
    }
    else if (role == Qt::FontRole)
    {
        QFont font;
        if(index.column() == Address)
        {
            font = GUIUtil::bitcoinAddressFont();
        }
        return font;
    }
    else if (role == TypeRole)
    {
        switch(rec->type)
        {
        case Bitcredit_AddressTableEntry::Sending:
            return Send;
        case Bitcredit_AddressTableEntry::Receiving:
            return Receive;
        default: break;
        }
    }
    return QVariant();
}

bool Bitcredit_AddressTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    Bitcredit_AddressTableEntry *rec = static_cast<Bitcredit_AddressTableEntry*>(index.internalPointer());
    std::string strPurpose = (rec->type == Bitcredit_AddressTableEntry::Sending ? "send" : "receive");
    editStatus = OK;

    if(role == Qt::EditRole)
    {
        LOCK(wallet->cs_wallet); /* For SetAddressBook / DelAddressBook */
        CTxDestination curAddress = CBitcoinAddress(rec->address.toStdString()).Get();
        if(index.column() == Label)
        {
            // Do nothing, if old label == new label
            if(rec->label == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            wallet->SetAddressBook(curAddress, value.toString().toStdString(), strPurpose);
        } else if(index.column() == Address) {
            CTxDestination newAddress = CBitcoinAddress(value.toString().toStdString()).Get();
            // Refuse to set invalid address, set error status and return false
            if(boost::get<CNoDestination>(&newAddress))
            {
                editStatus = INVALID_ADDRESS;
                return false;
            }
            // Do nothing, if old address == new address
            else if(newAddress == curAddress)
            {
                editStatus = NO_CHANGES;
                return false;
            }
            // Check for duplicate addresses to prevent accidental deletion of addresses, if you try
            // to paste an existing address over another address (with a different label)
            else if(wallet->mapAddressBook.count(newAddress))
            {
                editStatus = DUPLICATE_ADDRESS;
                return false;
            }
            // Double-check that we're not overwriting a receiving address
            else if(rec->type == Bitcredit_AddressTableEntry::Sending)
            {
                // Remove old entry
                wallet->DelAddressBook(curAddress);
                // Add new entry with new address
                wallet->SetAddressBook(newAddress, rec->label.toStdString(), strPurpose);
            }
        }
        return true;
    }
    return false;
}

QVariant Bitcredit_AddressTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
    }
    return QVariant();
}

Qt::ItemFlags Bitcredit_AddressTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    Bitcredit_AddressTableEntry *rec = static_cast<Bitcredit_AddressTableEntry*>(index.internalPointer());

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    // Can edit address and label for sending addresses,
    // and only label for receiving addresses.
    if(rec->type == Bitcredit_AddressTableEntry::Sending ||
      (rec->type == Bitcredit_AddressTableEntry::Receiving && index.column()==Label))
    {
        retval |= Qt::ItemIsEditable;
    }
    return retval;
}

QModelIndex Bitcredit_AddressTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    Bitcredit_AddressTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void Bitcredit_AddressTableModel::updateEntry(const QString &address,
        const QString &label, bool isMine, const QString &purpose, int status)
{
    // Update address book model from Credits Core
    priv->updateEntry(address, label, isMine, purpose, status);
}

QString Bitcredit_AddressTableModel::addRow(const QString &type, const QString &label, const QString &address)
{
    std::string strLabel = label.toStdString();
    std::string strAddress = address.toStdString();

    editStatus = OK;

    if(type == Send)
    {
        if(!walletModel->validateAddress(address))
        {
            editStatus = INVALID_ADDRESS;
            return QString();
        }
        // Check for duplicate addresses
        {
            LOCK(wallet->cs_wallet);
            if(wallet->mapAddressBook.count(CBitcoinAddress(strAddress).Get()))
            {
                editStatus = DUPLICATE_ADDRESS;
                return QString();
            }
        }
    }
    else if(type == Receive)
    {
        // Generate a new address to associate with given label
        CPubKey newKey;
        if(!wallet->GetKeyFromPool(newKey))
        {
            Bitcredit_WalletModel::UnlockContext ctx(walletModel->requestUnlock());
            if(!ctx.isValid())
            {
                // Unlock wallet failed or was cancelled
                editStatus = WALLET_UNLOCK_FAILURE;
                return QString();
            }
            if(!wallet->GetKeyFromPool(newKey))
            {
                editStatus = KEY_GENERATION_FAILURE;
                return QString();
            }
        }
        strAddress = CBitcoinAddress(newKey.GetID()).ToString();
    }
    else
    {
        return QString();
    }

    // Add entry
    {
        LOCK(wallet->cs_wallet);
        wallet->SetAddressBook(CBitcoinAddress(strAddress).Get(), strLabel,
                               (type == Send ? "send" : "receive"));
    }
    return QString::fromStdString(strAddress);
}

bool Bitcredit_AddressTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);
    Bitcredit_AddressTableEntry *rec = priv->index(row);
    if(count != 1 || !rec || rec->type == Bitcredit_AddressTableEntry::Receiving)
    {
        // Can only remove one row at a time, and cannot remove rows not in model.
        // Also refuse to remove receiving addresses.
        return false;
    }
    {
        LOCK(wallet->cs_wallet);
        wallet->DelAddressBook(CBitcoinAddress(rec->address.toStdString()).Get());
    }
    return true;
}

/* Look up label for address in address book, if not found return empty string.
 */
QString Bitcredit_AddressTableModel::labelForAddress(const QString &address) const
{
    {
        LOCK(wallet->cs_wallet);
        CBitcoinAddress address_parsed(address.toStdString());
        std::map<CTxDestination, CAddressBookData>::iterator mi = wallet->mapAddressBook.find(address_parsed.Get());
        if (mi != wallet->mapAddressBook.end())
        {
            return QString::fromStdString(mi->second.name);
        }
    }
    return QString();
}

int Bitcredit_AddressTableModel::lookupAddress(const QString &address) const
{
    QModelIndexList lst = match(index(0, Address, QModelIndex()),
                                Qt::EditRole, address, 1, Qt::MatchExactly);
    if(lst.isEmpty())
    {
        return -1;
    }
    else
    {
        return lst.at(0).row();
    }
}

void Bitcredit_AddressTableModel::emitDataChanged(int idx)
{
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
