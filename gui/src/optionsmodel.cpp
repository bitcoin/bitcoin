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
    if(role == Qt::EditRole)
    {
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
    bool successful = true; /* set to false on parse error */
    if(role == Qt::EditRole)
    {
        switch(index.row())
        {
        case StartAtStartup:
            successful = false; /*TODO*/
            break;
        case MinimizeToTray:
            fMinimizeToTray = value.toBool();
            break;
        case MapPortUPnP:
            fUseUPnP = value.toBool();
            break;
        case MinimizeOnClose:
            fMinimizeOnClose = value.toBool();
            break;
        case ConnectSOCKS4:
            fUseProxy = value.toBool();
            break;
        case ProxyIP:
            {
                /* Use CAddress to parse IP */
                CAddress addr(value.toString().toStdString() + ":1");
                if (addr.ip != INADDR_NONE)
                {
                    addrProxy.ip = addr.ip;
                } else {
                    successful = false;
                }
            }
            break;
        case ProxyPort:
            {
                int nPort = atoi(value.toString().toAscii().data());
                if (nPort > 0 && nPort < USHRT_MAX)
                {
                    addrProxy.port = htons(nPort);
                } else {
                    successful = false;
                }
            }
            break;
        case Fee: {
                int64 retval;
                if(ParseMoney(value.toString().toStdString(), retval))
                {
                    nTransactionFee = retval;
                } else {
                    successful = false; /* parse error */
                }
            }
            break;
        default:
            break;
        }
    }
    emit dataChanged(index, index);

    return successful;
}

qint64 OptionsModel::getTransactionFee()
{
    return nTransactionFee;
}

bool getMinimizeToTray()
{
    return fMinimizeToTray;
}

bool getMinimizeOnClose()
{
    return fMinimizeOnClose;
}
