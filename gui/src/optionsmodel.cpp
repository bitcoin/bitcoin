#include "optionsmodel.h"
#include "main.h"

#include <QDebug>

OptionsModel::OptionsModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

int OptionsModel::rowCount(const QModelIndex & parent) const
{
    return OptionIDRowCount;
}

QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    qDebug() << "OptionsModel::data" << " " << index.row() << " " << role;
    if(role == Qt::EditRole)
    {
        /* Delegate to specific column handlers */
        switch(index.row())
        {
        case StartAtStartup:
            return QVariant();
        case MinimizeToTray:
            return QVariant(fMinimizeToTray);
        case MapPortUPnP:
            return QVariant(fUseUPnP);
        case MinimizeOnClose:
            return QVariant(fMinimizeOnClose);
        case ConnectSOCKS4:
            return QVariant(fUseProxy);
        case ProxyIP:
            return QVariant(QString::fromStdString(addrProxy.ToStringIP()));
        case ProxyPort:
            return QVariant(QString::fromStdString(addrProxy.ToStringPort()));
        case Fee:
            return QVariant(QString::fromStdString(FormatMoney(nTransactionFee)));
        default:
            return QVariant();
        }
    }
    return QVariant();
}

bool OptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    qDebug() << "OptionsModel::setData" << " " << index.row() << "=" << value;
    emit dataChanged(index, index);
    return true;
}

qint64 OptionsModel::getTransactionFee()
{
    return nTransactionFee;
}
