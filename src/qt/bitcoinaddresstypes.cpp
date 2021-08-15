// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoinaddresstypes.h>

#include <QStringList>

BitcoinAddressTypes::BitcoinAddressTypes(QObject *parent):
        QAbstractListModel(parent),
        addressTypeList(availableAddressTypes())
{
}

QList<BitcoinAddressTypes::AddressType> BitcoinAddressTypes::availableAddressTypes()
{
    QList<BitcoinAddressTypes::AddressType> addressTypeList;
    addressTypeList.append(LEGACY);
    addressTypeList.append(NESTED_SEGWIT);
    addressTypeList.append(NATIVE_SEGWIT);
    return addressTypeList;
}

bool BitcoinAddressTypes::valid(int addressType)
{
    switch(addressType)
    {
    case LEGACY:
    case NESTED_SEGWIT:
    case NATIVE_SEGWIT:
        return true;
    default:
        return false;
    }
}

QString BitcoinAddressTypes::longName(int addressType)
{
    switch(addressType)
    {
    case LEGACY: return QString("Legacy");
    case NESTED_SEGWIT: return QString("Nested Segwit");
    case NATIVE_SEGWIT: return QString("Native Segwit");
    default: return QString("???");
    }
}

QString BitcoinAddressTypes::shortName(int addressType)
{
    return longName(addressType);
}

QString BitcoinAddressTypes::description(int addressType)
{
    switch(addressType)
    {
    case LEGACY: return QString("Legacy bitcoin address. P2PKH");
    case NESTED_SEGWIT: return QString("Nested Segwit address. P2SH");
    case NATIVE_SEGWIT: return QString("Native Segwit address. P2WPKH");
    default: return QString("???");
    }
}

int BitcoinAddressTypes::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return addressTypeList.size();
}

QVariant BitcoinAddressTypes::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < addressTypeList.size())
    {
        AddressType addressType = addressTypeList.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(longName(addressType));
        case Qt::ToolTipRole:
            return QVariant(description(addressType));
        case AddressTypeRole:
            return QVariant(static_cast<int>(addressType));
        }
    }
    return QVariant();
}
