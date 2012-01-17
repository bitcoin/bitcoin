#include "optionsmodel.h"
#include "bitcoinunits.h"

#include "headers.h"
#include "init.h"

OptionsModel::OptionsModel(CWallet *wallet, QObject *parent) :
    QAbstractListModel(parent),
    wallet(wallet),
    nDisplayUnit(BitcoinUnits::BTC),
    bDisplayAddresses(false)
{
    // Read our specific settings from the wallet db
    CWalletDB walletdb(wallet->strWalletFile);
    walletdb.ReadSetting("nDisplayUnit", nDisplayUnit);
    walletdb.ReadSetting("bDisplayAddresses", bDisplayAddresses);
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
            return QVariant(GetStartOnSystemStartup());
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
            return QVariant(addrProxy.GetPort());
        case Fee:
            return QVariant(nTransactionFee);
        case DisplayUnit:
            return QVariant(nDisplayUnit);
        case DisplayAddresses:
            return QVariant(bDisplayAddresses);
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
        CWalletDB walletdb(wallet->strWalletFile);
        switch(index.row())
        {
        case StartAtStartup:
            successful = SetStartOnSystemStartup(value.toBool());
            break;
        case MinimizeToTray:
            fMinimizeToTray = value.toBool();
            walletdb.WriteSetting("fMinimizeToTray", fMinimizeToTray);
            break;
        case MapPortUPnP:
            fUseUPnP = value.toBool();
            walletdb.WriteSetting("fUseUPnP", fUseUPnP);
#ifdef USE_UPNP
            MapPort(fUseUPnP);
#endif
            break;
        case MinimizeOnClose:
            fMinimizeOnClose = value.toBool();
            walletdb.WriteSetting("fMinimizeOnClose", fMinimizeOnClose);
            break;
        case ConnectSOCKS4:
            fUseProxy = value.toBool();
            walletdb.WriteSetting("fUseProxy", fUseProxy);
            break;
        case ProxyIP:
            {
                // Use CAddress to parse and check IP
                CNetAddr addr(value.toString().toStdString());
                if (addr.IsValid())
                {
                    addrProxy.SetIP(addr);
                    walletdb.WriteSetting("addrProxy", addrProxy);
                }
                else
                {
                    successful = false;
                }
            }
            break;
        case ProxyPort:
            {
                int nPort = atoi(value.toString().toAscii().data());
                if (nPort > 0 && nPort < std::numeric_limits<unsigned short>::max())
                {
                    addrProxy.SetPort(nPort);
                    walletdb.WriteSetting("addrProxy", addrProxy);
                }
                else
                {
                    successful = false;
                }
            }
            break;
        case Fee: {
            nTransactionFee = value.toLongLong();
            walletdb.WriteSetting("nTransactionFee", nTransactionFee);
            }
            break;
        case DisplayUnit: {
            int unit = value.toInt();
            nDisplayUnit = unit;
            walletdb.WriteSetting("nDisplayUnit", nDisplayUnit);
            emit displayUnitChanged(unit);
            }
        case DisplayAddresses: {
            bDisplayAddresses = value.toBool();
            walletdb.WriteSetting("bDisplayAddresses", bDisplayAddresses);
            }
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

bool OptionsModel::getMinimizeToTray()
{
    return fMinimizeToTray;
}

bool OptionsModel::getMinimizeOnClose()
{
    return fMinimizeOnClose;
}

int OptionsModel::getDisplayUnit()
{
    return nDisplayUnit;
}

bool OptionsModel::getDisplayAddresses()
{
    return bDisplayAddresses;
}
