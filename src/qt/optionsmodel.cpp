// Copyright (c) 2011-2020 The Bitcoin Core developers
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
#include <mapport.h>
#include <net.h>
#include <netbase.h>
#include <txdb.h>       // for -dbcache defaults
#include <util/string.h>
#include <validation.h> // For DEFAULT_SCRIPTCHECK_THREADS

#include <QDebug>
#include <QLatin1Char>
#include <QSettings>
#include <QStringList>

#include <univalue.h>

const char *DEFAULT_GUI_PROXY_HOST = "127.0.0.1";

static const QString GetDefaultProxyAddress();

//! Convert bitcoin settings value to integer.
static int ToInt(const util::SettingsValue& value, int fallback = 0)
{
    if (value.isNull()) return fallback;
    if (value.isBool()) return value.get_bool();
    if (value.isNum()) return value.get_int();
    return std::atoi(value.get_str().c_str());
}

//! Convert bitcoin settings value to QString.
static QString ToQString(const util::SettingsValue& value, const QString& fallback = {})
{
    if (value.isNull()) return fallback;
    if (value.isFalse()) return "";
    return QString::fromStdString(value.get_str());
}

//! Convert bitcoin setting to QVariant.
static QVariant ToQVariant(const util::SettingsValue& value, const QVariant& fallback = {})
{
    if (value.isNull()) return fallback;
    if (value.isBool()) return value.get_bool();
    if (value.isNum()) return value.get_int();
    return QString::fromStdString(value.get_str());
}

//! Convert QSettings QVariant value to bitcoin setting.
static util::SettingsValue ToSetting(const QVariant& variant, QVariant::Type type, const util::SettingsValue& fallback = {})
{
    if (!variant.isValid()) return fallback;
    if (type == QVariant::Bool) return variant.toBool();
    if (type == QVariant::Int) return variant.toInt();
    std::string str = variant.toString().toStdString();
    if (str.empty()) return false;
    return str;
}

//! Get pruning enabled value to show in GUI from bitcoin -prune setting.
static bool PruneEnabled(const util::SettingsValue& prune_setting)
{
    // -prune=1 setting is manual pruning mode, so disabled for purposes of the gui
    return ToInt(prune_setting) > 1;
}

//! Get pruning size value to show in GUI from bitcoin -prune setting. If
//! pruning is not enabled, just show default recommended pruning size (2GB).
static int PruneSizeGB(const util::SettingsValue& prune_setting)
{
    int value = ToInt(prune_setting);
    return value > 1 ? PruneMiBtoGB(value) : DEFAULT_PRUNE_TARGET_GB;
}

//! Convert enabled/size values to bitcoin -prune setting.
static util::SettingsValue PruneSetting(bool prune_enabled, int prune_size_gb)
{
    assert(prune_size_gb >= 1); // PruneSizeGB and ParsePruneSizeGB never return less
    return prune_enabled ? PruneGBtoMiB(prune_size_gb) : 0;
}

//! Interpret pruning size value provided by user in GUI or loaded from a legacy
//! QSettings source (windows registry key or qt .conf file). Smallest value
//! that the GUI can display is 1 GB, so round up if anything less is parsed.
static int ParsePruneSizeGB(const QVariant& prune_size) { return std::max(1, prune_size.toInt()); }

struct ProxySetting {
    bool is_set;
    QString ip;
    QString port;
};
static ProxySetting ParseProxyString(const QString& proxy);
static QString ProxyString(bool is_set, QString ip, QString port);

OptionsModel::OptionsModel(interfaces::Node& node, QObject *parent, bool resetSettings) :
    QAbstractListModel(parent), m_node(&node)
{
    Init(resetSettings);
}

void OptionsModel::addOverriddenOption(const std::string &option)
{
    strOverriddenByCommandLine += QString::fromStdString(option) + "=" + QString::fromStdString(gArgs.GetArg(option, "")) + " ";
}

// Writes all missing QSettings with their default values
void OptionsModel::Init(bool resetSettings)
{
    if (resetSettings)
        Reset();

    // Initialize display settings from stored settings.
    m_prune_size_gb = PruneSizeGB(node().getPersistentSetting("prune"));
    ProxySetting proxy = ParseProxyString(ToQString(node().getPersistentSetting("proxy")));
    m_proxy_ip = proxy.ip;
    m_proxy_port = proxy.port;
    ProxySetting onion = ParseProxyString(ToQString(node().getPersistentSetting("onion")));
    m_onion_ip = onion.ip;
    m_onion_port = onion.port;
    language = ToQString(node().getPersistentSetting("language"));

    checkAndMigrate();

    QSettings settings;

    // Ensure restart flag is unset on client startup
    setRestartRequired(false);

    // These are Qt-only settings:

    // Window
    if (!settings.contains("fHideTrayIcon")) {
        settings.setValue("fHideTrayIcon", false);
    }
    m_show_tray_icon = !settings.value("fHideTrayIcon").toBool();
    Q_EMIT showTrayIconChanged(m_show_tray_icon);

    if (!settings.contains("fMinimizeToTray"))
        settings.setValue("fMinimizeToTray", false);
    fMinimizeToTray = settings.value("fMinimizeToTray").toBool() && m_show_tray_icon;

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

    // These are shared with the core or have a command-line parameter
    // and we want command-line parameters to overwrite the GUI settings.
    if (node().isSettingIgnored("prune")) addOverriddenOption("-prune");
    if (node().isSettingIgnored("dbcache")) addOverriddenOption("-dbcache");
    if (node().isSettingIgnored("par")) addOverriddenOption("-par");
    if (node().isSettingIgnored("spendzeroconfchange")) addOverriddenOption("-spendzeroconfchange");
    if (node().isSettingIgnored("signer")) addOverriddenOption("-signer");
    if (node().isSettingIgnored("upnp")) addOverriddenOption("-upnp");
    if (node().isSettingIgnored("natpmp")) addOverriddenOption("-natpmp");
    if (node().isSettingIgnored("listen")) addOverriddenOption("-listen");
    if (node().isSettingIgnored("proxy")) addOverriddenOption("-proxy");
    if (node().isSettingIgnored("onion")) addOverriddenOption("-onion");
    if (node().isSettingIgnored("language")) addOverriddenOption("-language");

    // If setting doesn't exist create it with defaults.
    if (!settings.contains("strDataDir"))
        settings.setValue("strDataDir", GUIUtil::getDefaultDataDirectory());

    if (!settings.contains("UseEmbeddedMonospacedFont")) {
        settings.setValue("UseEmbeddedMonospacedFont", "true");
    }
    m_use_embedded_monospaced_font = settings.value("UseEmbeddedMonospacedFont").toBool();
    Q_EMIT useEmbeddedMonospacedFontChanged(m_use_embedded_monospaced_font);
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
    BackupSettings(gArgs.GetDataDirNet() / "guisettings.ini.bak", settings);

    // Save the strDataDir setting
    QString dataDir = GUIUtil::getDefaultDataDirectory();
    dataDir = settings.value("strDataDir", dataDir).toString();

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

static ProxySetting ParseProxyString(const QString& proxy)
{
    static const ProxySetting default_val = {false, DEFAULT_GUI_PROXY_HOST, QString("%1").arg(DEFAULT_GUI_PROXY_PORT)};
    // Handle the case that the setting is not set at all
    if (proxy.isEmpty()) {
        return default_val;
    }
    // contains IP at index 0 and port at index 1
    QStringList ip_port = GUIUtil::SplitSkipEmptyParts(proxy, ":");
    if (ip_port.size() == 2) {
        return {true, ip_port.at(0), ip_port.at(1)};
    } else { // Invalid: return default
        return default_val;
    }
}

static QString ProxyString(bool is_set, QString ip, QString port)
{
    return is_set ? ip + ":" + port : QString{""};
}

static const QString GetDefaultProxyAddress()
{
    return QString("%1:%2").arg(DEFAULT_GUI_PROXY_HOST).arg(DEFAULT_GUI_PROXY_PORT);
}

void OptionsModel::SetPruneEnabled(bool prune, bool force)
{
    if (force) {
        node().updateSetting("prune", PruneSetting(prune, m_prune_size_gb));
    } else {
        setOption(Prune, prune);
    }
}

void OptionsModel::SetPruneTargetGB(int prune_target_gb, bool force)
{
    const bool prune = prune_target_gb > 0;
    if (prune) {
        QSettings settings;
        settings.setValue("nPruneSize", prune_target_gb);
    }
    SetPruneEnabled(prune, force);
}

// read QSettings values and return them
QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::EditRole)
    {
        return getOption(OptionID(index.row()));
    }
    return QVariant();
}

// write QSettings values
bool OptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    bool successful = true; /* set to false on parse error */
    if(role == Qt::EditRole)
    {
        successful = setOption(OptionID(index.row()), value);
    }

    Q_EMIT dataChanged(index, index);

    return successful;
}

QVariant OptionsModel::getOption(OptionID option) const
{
    QSettings settings;
    switch (option) {
    case StartAtStartup:
        return GUIUtil::GetStartOnSystemStartup();
    case ShowTrayIcon:
        return m_show_tray_icon;
    case MinimizeToTray:
        return fMinimizeToTray;
    case MapPortUPnP:
#ifdef USE_UPNP
        return settings.value("fUseUPnP");
#else
        return false;
#endif
    case MapPortNatpmp:
#ifdef USE_NATPMP
        return settings.value("fUseNatpmp");
#else
        return false;
#endif // USE_NATPMP
    case MinimizeOnClose:
        return fMinimizeOnClose;

    // default proxy
    case ProxyUse:
        return !ToQString(node().getPersistentSetting("proxy")).isEmpty();
    case ProxyIP:
        return m_proxy_ip;
    case ProxyPort:
        return m_proxy_port;

    // separate Tor proxy
    case ProxyUseTor:
        return !ToQString(node().getPersistentSetting("onion")).isEmpty();
    case ProxyIPTor:
        return m_onion_ip;
    case ProxyPortTor:
        return m_onion_port;

#ifdef ENABLE_WALLET
    case SpendZeroConfChange:
        return ToQVariant(node().getPersistentSetting("spendzeroconfchange"), true);
    case ExternalSignerPath:
        return ToQVariant(node().getPersistentSetting("signer"), "");
#endif
    case DisplayUnit:
        return nDisplayUnit;
    case ThirdPartyTxUrls:
        return strThirdPartyTxUrls;
    case Language:
        return ToQVariant(node().getPersistentSetting("language"), "");
    case UseEmbeddedMonospacedFont:
        return m_use_embedded_monospaced_font;
    case CoinControlFeatures:
        return fCoinControlFeatures;
    case Prune:
        return PruneEnabled(node().getPersistentSetting("prune"));
    case PruneSize:
        return m_prune_size_gb;
    case DatabaseCache:
        return ToQVariant(node().getPersistentSetting("dbcache"), (qint64)nDefaultDbCache);
    case ThreadsScriptVerif:
        return ToQVariant(node().getPersistentSetting("par"), DEFAULT_SCRIPTCHECK_THREADS);
    case Listen:
        return ToQVariant(node().getPersistentSetting("listen"), DEFAULT_LISTEN);
    default:
        return QVariant();
    }
}

bool OptionsModel::setOption(OptionID option, const QVariant& value)
{
    auto changed = [&] { return value.isValid() && value != getOption(option); };

    bool successful = true; /* set to false on parse error */
    QSettings settings;

    switch (option) {
    case StartAtStartup:
        successful = GUIUtil::SetStartOnSystemStartup(value.toBool());
        break;
    case ShowTrayIcon:
        m_show_tray_icon = value.toBool();
        settings.setValue("fHideTrayIcon", !m_show_tray_icon);
        Q_EMIT showTrayIconChanged(m_show_tray_icon);
        break;
    case MinimizeToTray:
        fMinimizeToTray = value.toBool();
        settings.setValue("fMinimizeToTray", fMinimizeToTray);
        break;
    case MapPortUPnP: // core option - can be changed on-the-fly
        if (changed()) {
            node().updateSetting("upnp", ToSetting(value, QVariant::Bool));
            node().mapPort(value.toBool(), getOption(MapPortNatpmp).toBool());
        }
        break;
    case MapPortNatpmp: // core option - can be changed on-the-fly
        if (changed()) {
            node().updateSetting("fUseNatpmp", ToSetting(value, QVariant::Bool));
            node().mapPort(getOption(MapPortUPnP).toBool(), value.toBool());
        }
        break;
    case MinimizeOnClose:
        fMinimizeOnClose = value.toBool();
        settings.setValue("fMinimizeOnClose", fMinimizeOnClose);
        break;

    // default proxy
    case ProxyUse:
        if (changed()) {
            node().updateSetting("proxy", ProxyString(value.toBool(), m_proxy_ip, m_proxy_port).toStdString());
            setRestartRequired(true);
        }
        break;
    case ProxyIP:
        if (changed()) {
            m_proxy_ip = value.toString();
            if (getOption(ProxyUse).toBool()) {
                node().updateSetting("proxy", ProxyString(true, m_proxy_ip, m_proxy_port).toStdString());
                setRestartRequired(true);
            }
        }
        break;
    case ProxyPort:
        if (changed()) {
            m_proxy_port = value.toString();
            if (getOption(ProxyUse).toBool()) {
                node().updateSetting("proxy", ProxyString(true, m_proxy_ip, m_proxy_port).toStdString());
                setRestartRequired(true);
            }
        }
        break;

    // separate Tor proxy
    case ProxyUseTor:
        if (changed()) {
            node().updateSetting("onion", ProxyString(value.toBool(), m_onion_ip, m_onion_port).toStdString());
            setRestartRequired(true);
        }
        break;
    case ProxyIPTor:
        if (changed()) {
            m_onion_ip = value.toString();
            if (getOption(ProxyUseTor).toBool()) {
                node().updateSetting("onion", ProxyString(true, m_onion_ip, m_onion_port).toStdString());
                setRestartRequired(true);
            }
        }
        break;
    case ProxyPortTor:
        if (changed()) {
            m_onion_port = value.toString();
            if (getOption(ProxyUseTor).toBool()) {
                node().updateSetting("onion", ProxyString(true, m_onion_ip, m_onion_port).toStdString());
                setRestartRequired(true);
            }
        }
        break;

#ifdef ENABLE_WALLET
    case SpendZeroConfChange:
        if (changed()) {
            node().updateSetting("spendzeroconfchange", ToSetting(value, QVariant::Bool));
            setRestartRequired(true);
        }
        break;
    case ExternalSignerPath:
        if (changed()) {
            node().updateSetting("signer", ToSetting(value, QVariant::String));
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
        if (changed()) {
            node().updateSetting("lang", ToSetting(value, QVariant::String));
            setRestartRequired(true);
        }
        break;
    case UseEmbeddedMonospacedFont:
        m_use_embedded_monospaced_font = value.toBool();
        settings.setValue("UseEmbeddedMonospacedFont", m_use_embedded_monospaced_font);
        Q_EMIT useEmbeddedMonospacedFontChanged(m_use_embedded_monospaced_font);
        break;
    case CoinControlFeatures:
        fCoinControlFeatures = value.toBool();
        settings.setValue("fCoinControlFeatures", fCoinControlFeatures);
        Q_EMIT coinControlFeaturesChanged(fCoinControlFeatures);
        break;
    case Prune:
        if (changed()) {
            node().updateSetting("prune", PruneSetting(value.toBool(), m_prune_size_gb));
            setRestartRequired(true);
        }
        break;
    case PruneSize:
        if (changed()) {
            m_prune_size_gb = ParsePruneSizeGB(value);
            if (getOption(Prune).toBool()) {
                node().updateSetting("prune", PruneSetting(true, m_prune_size_gb));
                setRestartRequired(true);
            }
        }
        break;
    case DatabaseCache:
        if (changed()) {
            node().updateSetting("dbcache", ToSetting(value, QVariant::Int));
            setRestartRequired(true);
        }
        break;
    case ThreadsScriptVerif:
        if (changed()) {
            node().updateSetting("par", ToSetting(value, QVariant::Int));
            setRestartRequired(true);
        }
        break;
    case Listen:
        if (changed()) {
            node().updateSetting("listen", ToSetting(value, QVariant::Bool));
            setRestartRequired(true);
        }
        break;
    default:
        break;
    }
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

    // Migrate and delete legacy GUI settings that have now moved to <datadir>/settings.json.
    auto migrate_setting = [&](OptionID option, const QString& qt_name, const std::string& name) {
        if (!settings.contains(qt_name)) return;
        QVariant value = settings.value(qt_name);
        if (node().getPersistentSetting(name).isNull()) {
            if (option == ProxyIP) {
                ProxySetting parsed = ParseProxyString(value.toString());
                setOption(ProxyIP, parsed.ip);
                setOption(ProxyPort, parsed.port);
            } else if (option == ProxyIPTor) {
                ProxySetting parsed = ParseProxyString(value.toString());
                setOption(ProxyIPTor, parsed.ip);
                setOption(ProxyPortTor, parsed.port);
            } else {
                setOption(option, value);
            }
        }
        settings.remove(qt_name);
    };

    migrate_setting(DatabaseCache, "nDatabaseCache", "dbcache");
    migrate_setting(ThreadsScriptVerif, "nThreadsScriptVerif", "par");
#ifdef ENABLE_WALLET
    migrate_setting(SpendZeroConfChange, "bSpendZeroConfChange", "spendzeroconfchange");
    migrate_setting(ExternalSignerPath, "external_signer_path", "signer");
#endif
    migrate_setting(MapPortUPnP, "fUseUPnP", "upnp");
    migrate_setting(MapPortNatpmp, "fUseNatpmp", "natpmp");
    migrate_setting(Listen, "fListen", "listen");
    migrate_setting(PruneSize, "nPruneSize", "prune");
    migrate_setting(Prune, "bPrune", "prune");
    migrate_setting(ProxyIP, "addrProxy", "proxy");
    migrate_setting(ProxyUse, "fUseProxy", "proxy");
    migrate_setting(ProxyIPTor, "addrSeparateProxyTor", "onion");
    migrate_setting(ProxyUseTor, "fUseSeparateProxyTor", "onion");
}
