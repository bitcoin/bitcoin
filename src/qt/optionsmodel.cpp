// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/optionsmodel.h>

#include <qt/bitcoinunits.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>

#include <interfaces/node.h>
#include <validation.h> // For DEFAULT_SCRIPTCHECK_THREADS
#include <wallet/walletconstants.h>
#include <net.h>
#include <netbase.h>
#include <txdb.h> // for -dbcache defaults

#include <QDebug>
#include <QSettings>
#include <QStringList>

const char *DEFAULT_GUI_PROXY_HOST = "127.0.0.1";

static const QString GetDefaultProxyAddress();

OptionsModel::OptionsModel(interfaces::Node& node, QObject *parent, bool resetSettings) :
    QAbstractListModel(parent), m_node(node)
{
    Init(resetSettings);
}

// Writes all missing QSettings with their default values
void OptionsModel::Init(bool resetSettings)
{
    if (resetSettings)
        Reset();

    checkAndMigrate();

    QSettings settings;

    // Ensure restart flag is unset on client startup
    setRestartRequired(false);

    // These are Qt-only settings:

    // Window
    if (!settings.contains("fHideTrayIcon"))
        settings.setValue("fHideTrayIcon", false);
    fHideTrayIcon = settings.value("fHideTrayIcon").toBool();
    Q_EMIT hideTrayIconChanged(fHideTrayIcon);

    if (!settings.contains("fMinimizeToTray"))
        settings.setValue("fMinimizeToTray", false);
    fMinimizeToTray = settings.value("fMinimizeToTray").toBool() && !fHideTrayIcon;

    if (!settings.contains("fMinimizeOnClose"))
        settings.setValue("fMinimizeOnClose", false);
    fMinimizeOnClose = settings.value("fMinimizeOnClose").toBool();

    // Display
    if (!settings.contains("nDisplayUnit"))
        settings.setValue("nDisplayUnit", BitcoinUnits::BTC);
    nDisplayUnit = settings.value("nDisplayUnit").toInt();

    if (!settings.contains("strThirdPartyTxUrls"))
        settings.setValue("strThirdPartyTxUrls", "");
    strThirdPartyTxUrls = settings.value("strThirdPartyTxUrls", "").toString();

    if (!settings.contains("fCoinControlFeatures"))
        settings.setValue("fCoinControlFeatures", false);
    fCoinControlFeatures = settings.value("fCoinControlFeatures", false).toBool();

    // These are shared with the core.
    // If setting doesn't exist create it with defaults.

    // Main

    if (!settings.contains("strDataDir"))
        settings.setValue("strDataDir", GUIUtil::getDefaultDataDirectory());
}

/** Helper function to copy contents from one QSettings to another.
 * By using allKeys this also covers nested settings in a hierarchy.
 */
static void CopySettings(QSettings& dst, const QSettings& src)
{
    for (const QString& key : src.allKeys()) {
        dst.setValue(key, src.value(key));
    }
}

/** Back up a QSettings to an ini-formatted file. */
static void BackupSettings(const fs::path& filename, const QSettings& src)
{
    qInfo() << "Backing up GUI settings to" << GUIUtil::boostPathToQString(filename);
    QSettings dst(GUIUtil::boostPathToQString(filename), QSettings::IniFormat);
    dst.clear();
    CopySettings(dst, src);
}

void OptionsModel::Reset()
{
    QSettings settings;

    // Backup old settings to chain-specific datadir for troubleshooting
    BackupSettings(GetDataDir(true) / "guisettings.ini.bak", settings);

    // Save the strDataDir setting
    QString dataDir = GUIUtil::getDefaultDataDirectory();
    dataDir = settings.value("strDataDir", dataDir).toString();

    // Remove rw config file
    gArgs.EraseRWConfigFile();

    // Remove all entries from our QSettings object
    settings.clear();

    // Set strDataDir
    settings.setValue("strDataDir", dataDir);

    // Set that this was reset
    settings.setValue("fReset", true);

    // default setting for OptionsModel::StartAtStartup - disabled
    if (GUIUtil::GetStartOnSystemStartup())
        GUIUtil::SetStartOnSystemStartup(false);
}

int OptionsModel::rowCount(const QModelIndex & parent) const
{
    return OptionIDRowCount;
}

struct ProxySetting {
    bool is_set;
    QString ip;
    QString port;
};

static ProxySetting GetProxySetting(QSettings &settings, const QString &name)
{
    static const ProxySetting default_val = {false, DEFAULT_GUI_PROXY_HOST, QString("%1").arg(DEFAULT_GUI_PROXY_PORT)};
    // Handle the case that the setting is not set at all
    if (!settings.contains(name)) {
        return default_val;
    }
    // contains IP at index 0 and port at index 1
    QStringList ip_port = settings.value(name).toString().split(":", QString::SkipEmptyParts);
    if (ip_port.size() == 2) {
        return {true, ip_port.at(0), ip_port.at(1)};
    } else { // Invalid: return default
        return default_val;
    }
}

static void SetProxySetting(QSettings &settings, const QString &name, const ProxySetting &ip_port)
{
    settings.setValue(name, ip_port.ip + ":" + ip_port.port);
}

static const QString GetDefaultProxyAddress()
{
    return QString("%1:%2").arg(DEFAULT_GUI_PROXY_HOST).arg(DEFAULT_GUI_PROXY_PORT);
}

void OptionsModel::SetPrune(bool prune, bool force)
{
    QSettings settings;
    settings.setValue("bPrune", prune);
    // Convert prune size from GB to MiB:
    const uint64_t nPruneSizeMiB = (settings.value("nPruneSize").toInt() * GB_BYTES) >> 20;
    std::string prune_val = prune ? std::to_string(nPruneSizeMiB) : "0";
    if (force) {
        m_node.forceSetArg("-prune", prune_val);
        return;
    }
}

// read QSettings values and return them
QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    int64_t prune = -1;
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            return GUIUtil::GetStartOnSystemStartup();
        case HideTrayIcon:
            return fHideTrayIcon;
        case MinimizeToTray:
            return fMinimizeToTray;
        case MapPortUPnP:
#ifdef USE_UPNP
            return gArgs.GetBoolArg("-upnp", DEFAULT_UPNP);
#else
            return false;
#endif
        case MinimizeOnClose:
            return fMinimizeOnClose;

        // default proxy
        case ProxyUse:
            return gArgs.GetArg("-proxy", "") != "";
        case ProxyIP:
            return GetProxySetting(settings, "addrProxy").ip;
        case ProxyPort:
            return GetProxySetting(settings, "addrProxy").port;

        // separate Tor proxy
        case ProxyUseTor:
            return gArgs.GetArg("-onion", "") != "";
        case ProxyIPTor:
            return GetProxySetting(settings, "addrSeparateProxyTor").ip;
        case ProxyPortTor:
            return GetProxySetting(settings, "addrSeparateProxyTor").port;

#ifdef ENABLE_WALLET
        case SpendZeroConfChange:
            return gArgs.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);
#endif
        case DisplayUnit:
            return nDisplayUnit;
        case ThirdPartyTxUrls:
            return strThirdPartyTxUrls;
        case Language:
            return QString::fromStdString(gArgs.GetArg("-lang", ""));
        case CoinControlFeatures:
            return fCoinControlFeatures;
        case Prune:
            prune = gArgs.GetArg("-prune", 0);
            // prune=0  is the default, prune=1 is set if the chain has actually been pruned
            return !(prune == 0 || prune == 1);
        case PruneSize:
            prune = gArgs.GetArg("-prune", 0);
            if (prune == 0 || prune == 1) {
              // When automatic pruning is disabled, fall back to settings in order to remember the last used value
              return settings.value("nPruneSize");
            } else {
              return qlonglong(prune / 1000);
            }
        case DatabaseCache:
            // (int64_t) is required for QT4
            return (qlonglong)gArgs.GetArg("-dbcache", (qint64)nDefaultDbCache);
        case ThreadsScriptVerif:
            return (qlonglong)gArgs.GetArg("-par", DEFAULT_SCRIPTCHECK_THREADS);
        case Listen:
            return gArgs.GetBoolArg("-listen", DEFAULT_LISTEN);
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// write QSettings values
bool OptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    bool successful = true; /* set to false on parse error */
    uint64_t prune = 0; // in GB
    uint64_t nPruneSizeMiB = 0;
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case StartAtStartup:
            successful = GUIUtil::SetStartOnSystemStartup(value.toBool());
            break;
        case HideTrayIcon:
            fHideTrayIcon = value.toBool();
            settings.setValue("fHideTrayIcon", fHideTrayIcon);
    		Q_EMIT hideTrayIconChanged(fHideTrayIcon);
            break;
        case MinimizeToTray:
            fMinimizeToTray = value.toBool();
            settings.setValue("fMinimizeToTray", fMinimizeToTray);
            break;
        case MapPortUPnP:
            if (gArgs.GetBoolArg("-upnp", DEFAULT_UPNP) != value) {
                gArgs.ModifyRWConfigFile("upnp", value.toBool() ? "1" : "0");
                gArgs.ForceSetArg("-upnp", value.toBool() ? "1" : "0");
            }
            m_node.mapPort(value.toBool());
            break;
        case MinimizeOnClose:
            fMinimizeOnClose = value.toBool();
            settings.setValue("fMinimizeOnClose", fMinimizeOnClose);
            break;

        // default proxy
        case ProxyUse:
            if (value.toBool() ^ (gArgs.GetArg("-proxy", "") != "")) {
                if (value.toBool()) {
                    // Turn on proxy
                    gArgs.ModifyRWConfigFile("proxy", settings.value("addrProxy").toString().toStdString());
                    gArgs.ForceSetArg("-proxy", settings.value("addrProxy").toString().toStdString());
                } else {
                    // Turn off proxy
                    gArgs.ModifyRWConfigFile("proxy", "");
                    gArgs.ForceSetArg("-proxy", "");
                }
                setRestartRequired(true);
            }
            break;
        case ProxyIP: {
            auto ip_port = GetProxySetting(settings, "addrProxy");
            if (!ip_port.is_set || ip_port.ip != value.toString()) {
                ip_port.ip = value.toString();
                SetProxySetting(settings, "addrProxy", ip_port);
                if (gArgs.GetArg("-proxy", "") != "") {
                    gArgs.ModifyRWConfigFile("proxy", settings.value("addrProxy").toString().toStdString());
                    gArgs.ForceSetArg("-proxy", settings.value("addrProxy").toString().toStdString());
                }
                setRestartRequired(true);
            }
        }
        break;
        case ProxyPort: {
            auto ip_port = GetProxySetting(settings, "addrProxy");
            if (!ip_port.is_set || ip_port.port != value.toString()) {
                ip_port.port = value.toString();
                SetProxySetting(settings, "addrProxy", ip_port);
                if (gArgs.GetArg("-proxy", "") != "") {
                    gArgs.ModifyRWConfigFile("proxy", settings.value("addrProxy").toString().toStdString());
                    gArgs.ForceSetArg("-proxy", settings.value("addrProxy").toString().toStdString());
                }
                setRestartRequired(true);
            }
        }
        break;

        // separate Tor proxy
        case ProxyUseTor:
            if (value.toBool() ^ (gArgs.GetArg("-onion", "") != "")) {
                if (value.toBool()) {
                    // Turn on proxy
                    gArgs.ModifyRWConfigFile("onion", settings.value("addrSeparateProxyTor").toString().toStdString());
                    gArgs.ForceSetArg("-onion", settings.value("addrSeparateProxyTor").toString().toStdString());
                } else {
                    // Turn off proxy
                    gArgs.ModifyRWConfigFile("onion", "");
                    gArgs.ForceSetArg("-onion", "");
                }
                setRestartRequired(true);
            }
            break;
        case ProxyIPTor: {
            auto ip_port = GetProxySetting(settings, "addrSeparateProxyTor");
            if (!ip_port.is_set || ip_port.ip != value.toString()) {
                ip_port.ip = value.toString();
                SetProxySetting(settings, "addrSeparateProxyTor", ip_port);
                if (gArgs.GetArg("-onion", "") != "") {
                    gArgs.ModifyRWConfigFile("onion", settings.value("addrSeparateProxyTor").toString().toStdString());
                    gArgs.ForceSetArg("-onion", settings.value("addrSeparateProxyTor").toString().toStdString());
                }
                setRestartRequired(true);
            }
        }
        break;
        case ProxyPortTor: {
            auto ip_port = GetProxySetting(settings, "addrSeparateProxyTor");
            if (!ip_port.is_set || ip_port.port != value.toString()) {
                ip_port.port = value.toString();
                SetProxySetting(settings, "addrSeparateProxyTor", ip_port);
                if (gArgs.GetArg("-onion", "") != "") {
                    gArgs.ModifyRWConfigFile("onion", settings.value("addrSeparateProxyTor").toString().toStdString());
                    gArgs.ForceSetArg("-onion", settings.value("addrSeparateProxyTor").toString().toStdString());
                }
                setRestartRequired(true);
            }
        }
        break;

#ifdef ENABLE_WALLET
        case SpendZeroConfChange:
            if (gArgs.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE) != value) {
                gArgs.ModifyRWConfigFile("spendzeroconfchange", value.toBool() ? "1" : "0");
                gArgs.ForceSetArg("-spendzeroconfchange", value.toBool() ? "1" : "0");
                setRestartRequired(true);
            }
            break;
#endif
        case DisplayUnit:
            setDisplayUnit(value);
            break;
        case ThirdPartyTxUrls:
            if (strThirdPartyTxUrls != value.toString()) {
                strThirdPartyTxUrls = value.toString();
                settings.setValue("strThirdPartyTxUrls", strThirdPartyTxUrls);
                setRestartRequired(true);
            }
            break;
        case Language:
            if (gArgs.GetArg("-lang", "") != value.toString().toStdString()) {
                gArgs.ModifyRWConfigFile("lang", value.toString().toStdString());
                gArgs.ForceSetArg("-lang", value.toString().toStdString());
                setRestartRequired(true);
            }
            break;
        case CoinControlFeatures:
            fCoinControlFeatures = value.toBool();
            settings.setValue("fCoinControlFeatures", fCoinControlFeatures);
            Q_EMIT coinControlFeaturesChanged(fCoinControlFeatures);
            break;
        case Prune:
            if (value.toBool()) {
              // When user enables pruning, store the prune size:
              nPruneSizeMiB = settings.value("nPruneSize").toInt() * 1000;
              gArgs.ModifyRWConfigFile("prune", std::to_string(nPruneSizeMiB));
              gArgs.ForceSetArg("-prune", std::to_string(nPruneSizeMiB));
            } else {
              // When user disables pruning, set prune=1 and store last used prune
              // size in settings.
              nPruneSizeMiB = gArgs.GetArg("-prune", (qint64)nDefaultDbCache);
              settings.setValue("nPruneSize", qint64(nPruneSizeMiB / 1000));
              // Check if the chain has actually been pruned
              if (m_node.havePruned()) {
                  gArgs.ModifyRWConfigFile("prune", "1");
                  gArgs.ForceSetArg("-prune", "1");
              } else {
                  // This permits indexes like -txindex
                  gArgs.ModifyRWConfigFile("prune", "0");
                  gArgs.ForceSetArg("-prune", "0");
              }
            }
            setRestartRequired(true);
            break;
        case PruneSize:
            // Convert prune size to MiB:
            prune = gArgs.GetArg("-prune", 0);
            nPruneSizeMiB = value.toInt() * 1000;
            if (uint64_t(gArgs.GetArg("-prune", 0)) != nPruneSizeMiB) {
                // Don't update rw_conf is prune is disabled:
                if (!(prune == 0 || prune == 1)) {
                  gArgs.ModifyRWConfigFile("prune", std::to_string(nPruneSizeMiB));
                  gArgs.ForceSetArg("-prune", std::to_string(nPruneSizeMiB));
                  setRestartRequired(true);
                }
                // Always update the setting, in case user toggles Prune:
                settings.setValue("nPruneSize", value);
            }
            break;
        case DatabaseCache:
            if (gArgs.GetArg("-dbcache", (qint64)nDefaultDbCache) != value.toLongLong()) {
                gArgs.ModifyRWConfigFile("dbcache", value.toString().toStdString());
                gArgs.ForceSetArg("-dbcache", value.toString().toStdString());
                setRestartRequired(true);
            }
            break;
        case ThreadsScriptVerif:
            if (gArgs.GetArg("-par", DEFAULT_SCRIPTCHECK_THREADS) != value.toLongLong()) {
                gArgs.ModifyRWConfigFile("par", value.toString().toStdString());
                gArgs.ForceSetArg("-par", value.toString().toStdString());
                setRestartRequired(true);
            }
            break;
        case Listen:
            if (gArgs.GetBoolArg("-listen", DEFAULT_LISTEN) != value) {
                gArgs.ModifyRWConfigFile("listen", value.toBool() ? "1" : "0");
                gArgs.ForceSetArg("-listen", value.toBool() ? "1" : "0");
                setRestartRequired(true);
            }
            break;
        default:
            break;
        }
    }

    Q_EMIT dataChanged(index, index);

    return successful;
}

/** Updates current unit in memory, settings and emits displayUnitChanged(newUnit) signal */
void OptionsModel::setDisplayUnit(const QVariant &value)
{
    if (!value.isNull())
    {
        QSettings settings;
        nDisplayUnit = value.toInt();
        settings.setValue("nDisplayUnit", nDisplayUnit);
        Q_EMIT displayUnitChanged(nDisplayUnit);
    }
}

void OptionsModel::setRestartRequired(bool fRequired)
{
    QSettings settings;
    return settings.setValue("fRestartRequired", fRequired);
}

bool OptionsModel::isRestartRequired() const
{
    QSettings settings;
    return settings.value("fRestartRequired", false).toBool();
}

void OptionsModel::checkAndMigrate()
{
    // Migration of default values
    // Check if the QSettings container was already loaded with this client version
    QSettings settings;
    static const char strSettingsVersionKey[] = "nSettingsVersion";
    int settingsVersion = settings.contains(strSettingsVersionKey) ? settings.value(strSettingsVersionKey).toInt() : 0;
    if (settingsVersion < CLIENT_VERSION)
    {
        // -dbcache was bumped from 100 to 300 in 0.13
        // see https://github.com/bitcoin/bitcoin/pull/8273
        // force people to upgrade to the new value if they are using 100MB
        if (settingsVersion < 130000 && settings.contains("nDatabaseCache") && settings.value("nDatabaseCache").toLongLong() == 100)
            settings.setValue("nDatabaseCache", (qint64)nDefaultDbCache);

        settings.setValue(strSettingsVersionKey, CLIENT_VERSION);
    }

    // Overwrite the 'addrProxy' setting in case it has been set to an illegal
    // default value (see issue #12623; PR #12650).
    if (settings.contains("addrProxy") && settings.value("addrProxy").toString().endsWith("%2")) {
        settings.setValue("addrProxy", GetDefaultProxyAddress());
    }

    // Overwrite the 'addrSeparateProxyTor' setting in case it has been set to an illegal
    // default value (see issue #12623; PR #12650).
    if (settings.contains("addrSeparateProxyTor") && settings.value("addrSeparateProxyTor").toString().endsWith("%2")) {
        settings.setValue("addrSeparateProxyTor", GetDefaultProxyAddress());
    }

    // Move the following QSettings to bitcoin_rw.conf, unless they are already set:
    if (!gArgs.IsArgSet("-lang")) {
        QString lang_territory_qsettings = settings.value("language", "").toString();
        if (!lang_territory_qsettings.isEmpty()) {
            gArgs.ModifyRWConfigFile("lang", lang_territory_qsettings.toStdString());
            // This is currently too late in the initialization process, so
            // GetLangTerritory() also checks QSettings.
            gArgs.ForceSetArg("-lang", lang_territory_qsettings.toStdString());
            settings.remove("language");
        }
    }

    if (!gArgs.IsArgSet("-prune")) {
        if (settings.contains("bPrune") && settings.value("bPrune").toBool()) {
            const uint64_t nPruneSizeMiB = settings.value("nPruneSize").toInt() * 1000;
            gArgs.ModifyRWConfigFile("prune", std::to_string(nPruneSizeMiB));
            gArgs.ForceSetArg("-prune", std::to_string(nPruneSizeMiB));
            settings.remove("bPrune");
            // Do not remove nPruneSize, because we need track this when the user
            // toggles the prune setting.
        }
    }

    if (!gArgs.IsArgSet("-dbcache")) {
        if (settings.contains("nDatabaseCache") && settings.value("nDatabaseCache") != (qint64)nDefaultDbCache) {
            gArgs.ModifyRWConfigFile("dbcache", settings.value("nDatabaseCache").toString().toStdString());
            gArgs.ForceSetArg("-dbcache", settings.value("nDatabaseCache").toString().toStdString());
            settings.remove("nDatabaseCache");
        }
    }

    if (!gArgs.IsArgSet("-par")) {
        if (settings.contains("nThreadsScriptVerif") && settings.value("nThreadsScriptVerif") != DEFAULT_SCRIPTCHECK_THREADS) {
            gArgs.ModifyRWConfigFile("par", settings.value("nThreadsScriptVerif").toString().toStdString());
            gArgs.ForceSetArg("-par", settings.value("nThreadsScriptVerif").toString().toStdString());
            settings.remove("nThreadsScriptVerif");
        }
    }

    if (!gArgs.IsArgSet("-spendzeroconfchange")) {
        const bool bSpendZeroConfChange = settings.value("bSpendZeroConfChange").toBool();
        if (settings.contains("bSpendZeroConfChange") && bSpendZeroConfChange != DEFAULT_SPEND_ZEROCONF_CHANGE) {
            gArgs.ModifyRWConfigFile("spendzeroconfchange", bSpendZeroConfChange ? "1" : "0");
            gArgs.ForceSetArg("-spendzeroconfchange", bSpendZeroConfChange ? "1" : "0");
            settings.remove("bSpendZeroConfChange");
        }
    }

    if (!gArgs.IsArgSet("-upnp")) {
        const bool upnp = settings.value("fUseUPnP").toBool();
        if (settings.contains("fUseUPnP") && upnp != DEFAULT_UPNP) {
            gArgs.ModifyRWConfigFile("upnp", upnp ? "1" : "0");
            gArgs.ForceSetArg("-upnp", upnp ? "1" : "0");
            settings.remove("fUseUPnP");
        }
    }

    if (!gArgs.IsArgSet("-listen")) {
        const bool listen = settings.value("fListen").toBool();
        if (settings.contains("fListen") && listen != DEFAULT_LISTEN) {
            gArgs.ModifyRWConfigFile("listen", listen ? "1" : "0");
            gArgs.ForceSetArg("-listen", listen ? "1" : "0");
            settings.remove("fListen");
        }
    }

    if (gArgs.GetArg("-proxy", "") == "") {
        // If proxy is enabled for the GUI (fUseProxy), set -proxy
        // based on addrProxy.
        const bool fUseProxy = settings.value("fUseProxy").toBool();
        if (settings.contains("fUseProxy") && fUseProxy) {
            QString address_proxy = settings.value("addrProxy", "").toString();

            gArgs.ModifyRWConfigFile("proxy", address_proxy.toStdString());
            gArgs.ForceSetArg("-proxy", address_proxy.toStdString());
            settings.remove("fUseProxy");
        }
    }

    if (gArgs.GetArg("-onion", "") == "") {
        // If Tor is enabled for the GUI (fUseSeparateProxyTor), set -onion
        // based on addrSeparateProxyTor.
        const bool fUseSeparateProxyTor = settings.value("fUseSeparateProxyTor").toBool();
        if (settings.contains("fUseSeparateProxyTor") && fUseSeparateProxyTor) {
            QString address_tor = settings.value("addrSeparateProxyTor", "").toString();

            gArgs.ModifyRWConfigFile("onion", address_tor.toStdString());
            gArgs.ForceSetArg("-onion", address_tor.toStdString());
            settings.remove("fUseSeparateProxyTor");
        }
    }
}
