#include "optionsmodel.h"
#include "bitcoinunits.h"
#include <QSettings>

#include "init.h"
#include "walletdb.h"
#include "guiutil.h"

OptionsModel::OptionsModel(QObject *parent) :
    QAbstractListModel(parent)
{
    Init();
}

bool static ApplyProxySettings()
{
    QSettings settings;
    CService addrProxy(settings.value("addrProxy", "127.0.0.1:9050").toString().toStdString());
    int nSocksVersion(settings.value("nSocksVersion", 5).toInt());
    if (!settings.value("fUseProxy", false).toBool()) {
        addrProxy = CService();
        nSocksVersion = 0;
        return false;
    }
    if (nSocksVersion && !addrProxy.IsValid())
        return false;

    if (!IsLimited(NET_IPV4))
        SetProxy(NET_IPV4, addrProxy, nSocksVersion);
    if (nSocksVersion > 4) {
#ifdef USE_IPV6
        if (!IsLimited(NET_IPV6))
            SetProxy(NET_IPV6, addrProxy, nSocksVersion);
#endif
    }

    SetNameProxy(addrProxy, nSocksVersion);

    return true;
}

bool static ApplyTorSettings()
{
    QSettings settings;
    CService addrTor(settings.value("addrTor", "127.0.0.1:9050").toString().toStdString());
    if (!settings.value("fUseTor", false).toBool()) {
        addrTor = CService();
        return false;
    }
    if (!addrTor.IsValid())
        return false;

    SetProxy(NET_TOR, addrTor, 5);
    SetReachable(NET_TOR);

    return true;
}

void OptionsModel::Init()
{
    QSettings settings;

    // These are Qt-only settings:
    nDisplayUnit = settings.value("nDisplayUnit", BitcoinUnits::BTC).toInt();
    bDisplayAddresses = settings.value("bDisplayAddresses", false).toBool();
    if (!settings.contains("strThirdPartyTxUrls")) {
        if(fTestNet)
            settings.setValue("strThirdPartyTxUrls", "");
        else
            settings.setValue("strThirdPartyTxUrls", "https://bitinfocharts.com/novacoin/tx/%s|https://coinplorer.com/NVC/Transactions/%s|https://explorer.novaco.in/tx/%s|https://bchain.info/NVC/tx/%s");
    }
    strThirdPartyTxUrls = settings.value("strThirdPartyTxUrls", "https://bitinfocharts.com/novacoin/tx/%s|https://coinplorer.com/NVC/Transactions/%s|https://explorer.novaco.in/tx/%s|https://bchain.info/NVC/tx/%s").toString();
    fMinimizeToTray = settings.value("fMinimizeToTray", false).toBool();
    fMinimizeOnClose = settings.value("fMinimizeOnClose", false).toBool();
    fCoinControlFeatures = settings.value("fCoinControlFeatures", false).toBool();
    nTransactionFee = settings.value("nTransactionFee").toLongLong();
    language = settings.value("language", "").toString();

    // These are shared with core Bitcoin; we want
    // command-line options to override the GUI settings:
    if ( !(settings.value("fTorOnly").toBool() && settings.contains("addrTor")) ) {
        if (settings.contains("addrProxy") && settings.value("fUseProxy").toBool())
            SoftSetArg("-proxy", settings.value("addrProxy").toString().toStdString());
        if (settings.contains("nSocksVersion") && settings.value("fUseProxy").toBool())
            SoftSetArg("-socks", settings.value("nSocksVersion").toString().toStdString());
    }

    if (settings.contains("addrTor") && settings.value("fUseTor").toBool()) {
        SoftSetArg("-tor", settings.value("addrTor").toString().toStdString());
        if (settings.value("fTorOnly").toBool())
            SoftSetArg("-onlynet", "tor");

        if (settings.value("TorName").toString().length() == 22) {
            std::string strTorName = settings.value("TorName").toString().toStdString();

            CService addrTorName(strTorName, GetListenPort());
            if (addrTorName.IsValid())
                SoftSetArg("-torname", strTorName);
        }
    }

    if (!fTestNet && settings.contains("externalSeeder") && settings.value("externalSeeder").toString() != "") {
        SoftSetArg("-peercollector", settings.value("externalSeeder").toString().toStdString());
    }

    if (settings.contains("detachDB"))
        SoftSetBoolArg("-detachdb", settings.value("detachDB").toBool());
    if (!language.isEmpty())
        SoftSetArg("-lang", language.toStdString());
}

int OptionsModel::rowCount(const QModelIndex & parent) const
{
    return OptionIDRowCount;
}

QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            return QVariant(GUIUtil::GetStartOnSystemStartup());
        case MinimizeToTray:
            return QVariant(fMinimizeToTray);
        case MinimizeOnClose:
            return QVariant(fMinimizeOnClose);
        case ProxyUse:
            return settings.value("fUseProxy", false);
        case ProxyIP: {
            proxyType proxy;
            if (GetProxy(NET_IPV4, proxy))
                return QVariant(QString::fromStdString(proxy.first.ToStringIP()));
            else
                return QVariant(QString::fromStdString("127.0.0.1"));
        }
        case ProxyPort: {
            proxyType proxy;
            if (GetProxy(NET_IPV4, proxy))
                return QVariant(proxy.first.GetPort());
            else
                return QVariant(nSocksDefault);
        }
        case ProxySocksVersion:
            return settings.value("nSocksVersion", 5);
        case TorUse:
            return settings.value("fUseTor", false);
        case TorIP: {
            proxyType proxy;
            if (GetProxy(NET_TOR, proxy))
                return QVariant(QString::fromStdString(proxy.first.ToStringIP()));
            else
                return QVariant(QString::fromStdString("127.0.0.1"));
        }
        case TorPort: {
            proxyType proxy;
            if (GetProxy(NET_TOR, proxy))
                return QVariant(proxy.first.GetPort());
            else
                return QVariant(nSocksDefault);
        }
        case TorOnly:
            return settings.value("fTorOnly", false);
        case TorName:
            return settings.value("TorName", "");
        case ExternalSeeder:
            return settings.value("externalSeeder", "");
        case Fee:
            return QVariant(static_cast<qlonglong>(nTransactionFee));
        case DisplayUnit:
            return QVariant(nDisplayUnit);
        case DisplayAddresses:
            return QVariant(bDisplayAddresses);
        case ThirdPartyTxUrls:
            return QVariant(strThirdPartyTxUrls);
        case DetachDatabases:
            return QVariant(bitdb.GetDetach());
        case Language:
            return settings.value("language", "");
        case CoinControlFeatures:
            return QVariant(fCoinControlFeatures);
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
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            successful = GUIUtil::SetStartOnSystemStartup(value.toBool());
            break;
        case MinimizeToTray:
            fMinimizeToTray = value.toBool();
            settings.setValue("fMinimizeToTray", fMinimizeToTray);
            break;
        case MinimizeOnClose:
            fMinimizeOnClose = value.toBool();
            settings.setValue("fMinimizeOnClose", fMinimizeOnClose);
            break;
        case ProxyUse:
            settings.setValue("fUseProxy", value.toBool());
            ApplyProxySettings();
            break;
        case ProxyIP: {
            proxyType proxy;
            proxy.first = CService("127.0.0.1", nSocksDefault);
            GetProxy(NET_IPV4, proxy);

            CNetAddr addr(value.toString().toStdString());
            proxy.first.SetIP(addr);
            settings.setValue("addrProxy", proxy.first.ToStringIPPort().c_str());
            successful = ApplyProxySettings();
        }
        break;
        case ProxyPort: {
            proxyType proxy;
            proxy.first = CService("127.0.0.1", nSocksDefault);
            GetProxy(NET_IPV4, proxy);

            proxy.first.SetPort(value.toInt());
            settings.setValue("addrProxy", proxy.first.ToStringIPPort().c_str());
            successful = ApplyProxySettings();
        }
        break;
        case ProxySocksVersion: {
            proxyType proxy;
            proxy.second = 5;
            GetProxy(NET_IPV4, proxy);

            proxy.second = value.toInt();
            settings.setValue("nSocksVersion", proxy.second);
            successful = ApplyProxySettings();
        }
        break;
        case TorUse: {
            settings.setValue("fUseTor", value.toBool());
            ApplyTorSettings();
        }
        break;
        case TorIP: {
            proxyType proxy;
            proxy.first = CService("127.0.0.1", nSocksDefault);
            GetProxy(NET_TOR, proxy);

            CNetAddr addr(value.toString().toStdString());
            proxy.first.SetIP(addr);
            settings.setValue("addrTor", proxy.first.ToStringIPPort().c_str());
            successful = ApplyTorSettings();
        }
        break;
        case TorPort: {
            proxyType proxy;
            proxy.first = CService("127.0.0.1", nSocksDefault);
            GetProxy(NET_TOR, proxy);

            proxy.first.SetPort((uint16_t)value.toUInt());
            settings.setValue("addrTor", proxy.first.ToStringIPPort().c_str());
            successful = ApplyTorSettings();
        }
        break;
        case TorOnly: {
            settings.setValue("fTorOnly", value.toBool());
            ApplyTorSettings();
        }
        case TorName: {
            settings.setValue("TorName", value.toString());
        }
        break;
        case ExternalSeeder:
            settings.setValue("externalSeeder", value.toString());
        break;
        case Fee:
            nTransactionFee = value.toLongLong();
            settings.setValue("nTransactionFee", static_cast<qlonglong>(nTransactionFee));
            emit transactionFeeChanged(nTransactionFee);
            break;
        case DisplayUnit:
            nDisplayUnit = value.toInt();
            settings.setValue("nDisplayUnit", nDisplayUnit);
            emit displayUnitChanged(nDisplayUnit);
            break;
        case DisplayAddresses:
            bDisplayAddresses = value.toBool();
            settings.setValue("bDisplayAddresses", bDisplayAddresses);
            break;
        case DetachDatabases: {
            bool fDetachDB = value.toBool();
            bitdb.SetDetach(fDetachDB);
            settings.setValue("detachDB", fDetachDB);
            }
            break;
        case ThirdPartyTxUrls:
            if (strThirdPartyTxUrls != value.toString()) {
                strThirdPartyTxUrls = value.toString();
                settings.setValue("strThirdPartyTxUrls", strThirdPartyTxUrls);
            }
            break;
        case Language:
            settings.setValue("language", value);
            break;
        case CoinControlFeatures: {
            fCoinControlFeatures = value.toBool();
            settings.setValue("fCoinControlFeatures", fCoinControlFeatures);
            emit coinControlFeaturesChanged(fCoinControlFeatures);
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

bool OptionsModel::getCoinControlFeatures()
{
    return fCoinControlFeatures;
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
