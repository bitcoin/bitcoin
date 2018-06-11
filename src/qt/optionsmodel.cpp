// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif

#include <qt/optionsmodel.h>

#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>

#include <interfaces/node.h>
#include <validation.h> // For DEFAULT_SCRIPTCHECK_THREADS
#include <net.h>
#include <netbase.h>
#include <txdb.h> // for -dbcache defaults

#ifdef ENABLE_WALLET
#include <coinjoin/coinjoin-client-options.h>
#endif

#include <QNetworkProxy>
#include <QSettings>
#include <QStringList>

const char *DEFAULT_GUI_PROXY_HOST = "127.0.0.1";

static const QString GetDefaultProxyAddress();

OptionsModel::OptionsModel(interfaces::Node& node, QObject *parent, bool resetSettings) :
    QAbstractListModel(parent), m_node(node)
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
        settings.setValue("nDisplayUnit", BitcoinUnits::DASH);
    nDisplayUnit = settings.value("nDisplayUnit").toInt();

    if (!settings.contains("strThirdPartyTxUrls"))
        settings.setValue("strThirdPartyTxUrls", "");
    strThirdPartyTxUrls = settings.value("strThirdPartyTxUrls", "").toString();

    // Appearance
    if (!settings.contains("theme"))
        settings.setValue("theme", GUIUtil::getDefaultTheme());

    if (!settings.contains("fontFamily"))
        settings.setValue("fontFamily", GUIUtil::fontFamilyToString(GUIUtil::getFontFamilyDefault()));

    if (!settings.contains("fontScale"))
        settings.setValue("fontScale", GUIUtil::getFontScaleDefault());
    if (!m_node.softSetArg("-font-scale", settings.value("fontScale").toString().toStdString()))
        addOverriddenOption("-font-scale");

    if (!settings.contains("fontWeightNormal"))
        settings.setValue("fontWeightNormal", GUIUtil::weightToArg(GUIUtil::getFontWeightNormalDefault()));
    if (!m_node.softSetArg("-font-weight-normal", settings.value("fontWeightNormal").toString().toStdString()))
        addOverriddenOption("-font-weight-normal");

    if (!settings.contains("fontWeightBold"))
        settings.setValue("fontWeightBold", GUIUtil::weightToArg(GUIUtil::getFontWeightBoldDefault()));
    if (!m_node.softSetArg("-font-weight-bold", settings.value("fontWeightBold").toString().toStdString()))
        addOverriddenOption("-font-weight-bold");

#ifdef ENABLE_WALLET
    if (!settings.contains("fCoinControlFeatures"))
        settings.setValue("fCoinControlFeatures", false);
    fCoinControlFeatures = settings.value("fCoinControlFeatures", false).toBool();

    if (!settings.contains("digits"))
        settings.setValue("digits", "2");

    // CoinJoin
    if (!settings.contains("fCoinJoinEnabled")) {
        settings.setValue("fCoinJoinEnabled", true);
    }
    if (!gArgs.SoftSetBoolArg("-enablecoinjoin", settings.value("fCoinJoinEnabled").toBool())) {
        addOverriddenOption("-enablecoinjoin");
    }

    if (!settings.contains("fShowAdvancedCJUI"))
        settings.setValue("fShowAdvancedCJUI", false);
    fShowAdvancedCJUI = settings.value("fShowAdvancedCJUI", false).toBool();

    if (!settings.contains("fShowCoinJoinPopups"))
        settings.setValue("fShowCoinJoinPopups", true);

    if (!settings.contains("fLowKeysWarning"))
        settings.setValue("fLowKeysWarning", true);
#endif // ENABLE_WALLET

    // These are shared with the core or have a command-line parameter
    // and we want command-line parameters to overwrite the GUI settings.
    //
    // If setting doesn't exist create it with defaults.
    //
    // If m_node.softSetArg() or m_node.softSetBoolArg() return false we were overridden
    // by command-line and show this in the UI.

    // Main
    if (!settings.contains("bPrune"))
        settings.setValue("bPrune", false);
    if (!settings.contains("nPruneSize"))
        settings.setValue("nPruneSize", 2);
    // Convert prune size to MB:
    const uint64_t nPruneSizeMB = settings.value("nPruneSize").toInt() * 1000;
    if (!m_node.softSetArg("-prune", settings.value("bPrune").toBool() ? std::to_string(nPruneSizeMB) : "0")) {
        addOverriddenOption("-prune");
    }

    // If GUI is setting prune, then we also must set disablegovernance and  txindex
    if (settings.value("bPrune").toBool()) {
        if (gArgs.SoftSetBoolArg("-disablegovernance", true)) {
            LogPrintf("%s: parameter interaction: -prune=true -> setting -disablegovernance=true\n", __func__);
            addOverriddenOption("-disablegovernance");
        }
        if (gArgs.SoftSetBoolArg("-txindex", false)) {
            LogPrintf("%s: parameter interaction: -prune=true -> setting -txindex=false\n", __func__);
            addOverriddenOption("-txindex");
        }
    }

    if (!settings.contains("nDatabaseCache"))
        settings.setValue("nDatabaseCache", (qint64)nDefaultDbCache);
    if (!m_node.softSetArg("-dbcache", settings.value("nDatabaseCache").toString().toStdString()))
        addOverriddenOption("-dbcache");

    if (!settings.contains("nThreadsScriptVerif"))
        settings.setValue("nThreadsScriptVerif", DEFAULT_SCRIPTCHECK_THREADS);
    if (!m_node.softSetArg("-par", settings.value("nThreadsScriptVerif").toString().toStdString()))
        addOverriddenOption("-par");

    // Wallet
#ifdef ENABLE_WALLET
    if (!settings.contains("bSpendZeroConfChange"))
        settings.setValue("bSpendZeroConfChange", true);
    if (!m_node.softSetBoolArg("-spendzeroconfchange", settings.value("bSpendZeroConfChange").toBool()))
        addOverriddenOption("-spendzeroconfchange");

    // CoinJoin
    if (!settings.contains("nCoinJoinRounds"))
        settings.setValue("nCoinJoinRounds", DEFAULT_COINJOIN_ROUNDS);
    if (!m_node.softSetArg("-coinjoinrounds", settings.value("nCoinJoinRounds").toString().toStdString()))
        addOverriddenOption("-coinjoinrounds");
    m_node.coinJoinOptions().setRounds(settings.value("nCoinJoinRounds").toInt());

    if (!settings.contains("nCoinJoinAmount")) {
        // for migration from old settings
        if (!settings.contains("nAnonymizeDashAmount"))
            settings.setValue("nCoinJoinAmount", DEFAULT_COINJOIN_AMOUNT);
        else
            settings.setValue("nCoinJoinAmount", settings.value("nAnonymizeDashAmount").toInt());
    }
    if (!m_node.softSetArg("-coinjoinamount", settings.value("nCoinJoinAmount").toString().toStdString()))
        addOverriddenOption("-coinjoinamount");
    m_node.coinJoinOptions().setAmount(settings.value("nCoinJoinAmount").toInt());

    if (!settings.contains("fCoinJoinMultiSession"))
        settings.setValue("fCoinJoinMultiSession", DEFAULT_COINJOIN_MULTISESSION);
    if (!m_node.softSetBoolArg("-coinjoinmultisession", settings.value("fCoinJoinMultiSession").toBool()))
        addOverriddenOption("-coinjoinmultisession");
    m_node.coinJoinOptions().setMultiSessionEnabled(settings.value("fCoinJoinMultiSession").toBool());
#endif

    // Network
    if (!settings.contains("fUseUPnP"))
        settings.setValue("fUseUPnP", DEFAULT_UPNP);
    if (!m_node.softSetBoolArg("-upnp", settings.value("fUseUPnP").toBool()))
        addOverriddenOption("-upnp");

    if (!settings.contains("fListen"))
        settings.setValue("fListen", DEFAULT_LISTEN);
    if (!m_node.softSetBoolArg("-listen", settings.value("fListen").toBool()))
        addOverriddenOption("-listen");

    if (!settings.contains("fUseProxy"))
        settings.setValue("fUseProxy", false);
    if (!settings.contains("addrProxy"))
        settings.setValue("addrProxy", GetDefaultProxyAddress());
    // Only try to set -proxy, if user has enabled fUseProxy
    if (settings.value("fUseProxy").toBool() && !m_node.softSetArg("-proxy", settings.value("addrProxy").toString().toStdString()))
        addOverriddenOption("-proxy");
    else if(!settings.value("fUseProxy").toBool() && !gArgs.GetArg("-proxy", "").empty())
        addOverriddenOption("-proxy");

    if (!settings.contains("fUseSeparateProxyTor"))
        settings.setValue("fUseSeparateProxyTor", false);
    if (!settings.contains("addrSeparateProxyTor"))
        settings.setValue("addrSeparateProxyTor", GetDefaultProxyAddress());
    // Only try to set -onion, if user has enabled fUseSeparateProxyTor
    if (settings.value("fUseSeparateProxyTor").toBool() && !m_node.softSetArg("-onion", settings.value("addrSeparateProxyTor").toString().toStdString()))
        addOverriddenOption("-onion");
    else if(!settings.value("fUseSeparateProxyTor").toBool() && !gArgs.GetArg("-onion", "").empty())
        addOverriddenOption("-onion");

    // Display
    if (!settings.contains("language"))
        settings.setValue("language", "");
    if (!m_node.softSetArg("-lang", settings.value("language").toString().toStdString()))
        addOverriddenOption("-lang");

    language = settings.value("language").toString();
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
    qWarning() << "Backing up GUI settings to" << GUIUtil::boostPathToQString(filename);
    QSettings dst(GUIUtil::boostPathToQString(filename), QSettings::IniFormat);
    dst.clear();
    CopySettings(dst, src);
}

void OptionsModel::Reset()
{
    QSettings settings;

    // Backup old settings to chain-specific datadir for troubleshooting
    BackupSettings(GetDataDir(true) / "guisettings.ini.bak", settings);

    // Remove all entries from our QSettings object
    settings.clear();

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

// read QSettings values and return them
QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
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
            return settings.value("fUseUPnP");
#else
            return false;
#endif
        case MinimizeOnClose:
            return fMinimizeOnClose;

        // default proxy
        case ProxyUse:
            return settings.value("fUseProxy", false);
        case ProxyIP:
            return GetProxySetting(settings, "addrProxy").ip;
        case ProxyPort:
            return GetProxySetting(settings, "addrProxy").port;

        // separate Tor proxy
        case ProxyUseTor:
            return settings.value("fUseSeparateProxyTor", false);
        case ProxyIPTor:
            return GetProxySetting(settings, "addrSeparateProxyTor").ip;
        case ProxyPortTor:
            return GetProxySetting(settings, "addrSeparateProxyTor").port;

#ifdef ENABLE_WALLET
        case SpendZeroConfChange:
            return settings.value("bSpendZeroConfChange");
        case ShowMasternodesTab:
            return settings.value("fShowMasternodesTab");
        case CoinJoinEnabled:
            return settings.value("fCoinJoinEnabled");
        case ShowAdvancedCJUI:
            return fShowAdvancedCJUI;
        case ShowCoinJoinPopups:
            return settings.value("fShowCoinJoinPopups");
        case LowKeysWarning:
            return settings.value("fLowKeysWarning");
        case CoinJoinRounds:
            return settings.value("nCoinJoinRounds");
        case CoinJoinAmount:
            return settings.value("nCoinJoinAmount");
        case CoinJoinMultiSession:
            return settings.value("fCoinJoinMultiSession");
#endif
        case DisplayUnit:
            return nDisplayUnit;
        case ThirdPartyTxUrls:
            return strThirdPartyTxUrls;
#ifdef ENABLE_WALLET
        case Digits:
            return settings.value("digits");
#endif // ENABLE_WALLET
        case Theme:
            return settings.value("theme");
        case FontFamily:
            return settings.value("fontFamily");
        case FontScale:
            return settings.value("fontScale");
        case FontWeightNormal: {
            QFont::Weight weight;
            GUIUtil::weightFromArg(settings.value("fontWeightNormal").toInt(), weight);
            return GUIUtil::supportedWeightToIndex(weight);
        }
        case FontWeightBold: {
            QFont::Weight weight;
            GUIUtil::weightFromArg(settings.value("fontWeightBold").toInt(), weight);
            return GUIUtil::supportedWeightToIndex(weight);
        }
        case Language:
            return settings.value("language");
#ifdef ENABLE_WALLET
        case CoinControlFeatures:
            return fCoinControlFeatures;
#endif // ENABLE_WALLET
        case Prune:
            return settings.value("bPrune");
        case PruneSize:
            return settings.value("nPruneSize");
        case DatabaseCache:
            return settings.value("nDatabaseCache");
        case ThreadsScriptVerif:
            return settings.value("nThreadsScriptVerif");
        case Listen:
            return settings.value("fListen");
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
        case MapPortUPnP: // core option - can be changed on-the-fly
            settings.setValue("fUseUPnP", value.toBool());
            m_node.mapPort(value.toBool());
            break;
        case MinimizeOnClose:
            fMinimizeOnClose = value.toBool();
            settings.setValue("fMinimizeOnClose", fMinimizeOnClose);
            break;

        // default proxy
        case ProxyUse:
            if (settings.value("fUseProxy") != value) {
                settings.setValue("fUseProxy", value.toBool());
                setRestartRequired(true);
            }
            break;
        case ProxyIP: {
            auto ip_port = GetProxySetting(settings, "addrProxy");
            if (!ip_port.is_set || ip_port.ip != value.toString()) {
                ip_port.ip = value.toString();
                SetProxySetting(settings, "addrProxy", ip_port);
                setRestartRequired(true);
            }
        }
        break;
        case ProxyPort: {
            auto ip_port = GetProxySetting(settings, "addrProxy");
            if (!ip_port.is_set || ip_port.port != value.toString()) {
                ip_port.port = value.toString();
                SetProxySetting(settings, "addrProxy", ip_port);
                setRestartRequired(true);
            }
        }
        break;

        // separate Tor proxy
        case ProxyUseTor:
            if (settings.value("fUseSeparateProxyTor") != value) {
                settings.setValue("fUseSeparateProxyTor", value.toBool());
                setRestartRequired(true);
            }
            break;
        case ProxyIPTor: {
            auto ip_port = GetProxySetting(settings, "addrSeparateProxyTor");
            if (!ip_port.is_set || ip_port.ip != value.toString()) {
                ip_port.ip = value.toString();
                SetProxySetting(settings, "addrSeparateProxyTor", ip_port);
                setRestartRequired(true);
            }
        }
        break;
        case ProxyPortTor: {
            auto ip_port = GetProxySetting(settings, "addrSeparateProxyTor");
            if (!ip_port.is_set || ip_port.port != value.toString()) {
                ip_port.port = value.toString();
                SetProxySetting(settings, "addrSeparateProxyTor", ip_port);
                setRestartRequired(true);
            }
        }
        break;

#ifdef ENABLE_WALLET
        case SpendZeroConfChange:
            if (settings.value("bSpendZeroConfChange") != value) {
                settings.setValue("bSpendZeroConfChange", value);
                setRestartRequired(true);
            }
            break;
        case ShowMasternodesTab:
            if (settings.value("fShowMasternodesTab") != value) {
                settings.setValue("fShowMasternodesTab", value);
                setRestartRequired(true);
            }
            break;
        case CoinJoinEnabled:
            if (settings.value("fCoinJoinEnabled") != value) {
                settings.setValue("fCoinJoinEnabled", value.toBool());
                Q_EMIT coinJoinEnabledChanged();
            }
            break;
        case ShowAdvancedCJUI:
            if (settings.value("fShowAdvancedCJUI") != value) {
                fShowAdvancedCJUI = value.toBool();
                settings.setValue("fShowAdvancedCJUI", fShowAdvancedCJUI);
                Q_EMIT AdvancedCJUIChanged(fShowAdvancedCJUI);
            }
            break;
        case ShowCoinJoinPopups:
            settings.setValue("fShowCoinJoinPopups", value);
            break;
        case LowKeysWarning:
            settings.setValue("fLowKeysWarning", value);
            break;
        case CoinJoinRounds:
            if (settings.value("nCoinJoinRounds") != value)
            {
                m_node.coinJoinOptions().setRounds(value.toInt());
                settings.setValue("nCoinJoinRounds", m_node.coinJoinOptions().getRounds());
                Q_EMIT coinJoinRoundsChanged();
            }
            break;
        case CoinJoinAmount:
            if (settings.value("nCoinJoinAmount") != value)
            {
                m_node.coinJoinOptions().setAmount(value.toInt());
                settings.setValue("nCoinJoinAmount", m_node.coinJoinOptions().getAmount());
                Q_EMIT coinJoinAmountChanged();
            }
            break;
        case CoinJoinMultiSession:
            if (settings.value("fCoinJoinMultiSession") != value)
            {
                m_node.coinJoinOptions().setMultiSessionEnabled(value.toBool());
                settings.setValue("fCoinJoinMultiSession", m_node.coinJoinOptions().isMultiSessionEnabled());
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
#ifdef ENABLE_WALLET
        case Digits:
            if (settings.value("digits") != value) {
                settings.setValue("digits", value);
                setRestartRequired(true);
            }
            break;
#endif // ENABLE_WALLET
        case Theme:
            // Set in AppearanceWidget::updateTheme slot now
            // to allow instant theme changes.
            break;
        case FontFamily:
            if (settings.value("fontFamily") != value) {
                settings.setValue("fontFamily", value);
            }
            break;
        case FontScale:
            if (settings.value("fontScale") != value) {
                settings.setValue("fontScale", value);
            }
            break;
        case FontWeightNormal: {
            int nWeight = GUIUtil::weightToArg(GUIUtil::supportedWeightFromIndex(value.toInt()));
            if (settings.value("fontWeightNormal") != nWeight) {
                settings.setValue("fontWeightNormal", nWeight);
            }
            break;
        }
        case FontWeightBold: {
            int nWeight = GUIUtil::weightToArg(GUIUtil::supportedWeightFromIndex(value.toInt()));
            if (settings.value("fontWeightBold") != nWeight) {
                settings.setValue("fontWeightBold", nWeight);
            }
            break;
        }
        case Language:
            if (settings.value("language") != value) {
                settings.setValue("language", value);
                setRestartRequired(true);
            }
            break;
#ifdef ENABLE_WALLET
        case CoinControlFeatures:
            fCoinControlFeatures = value.toBool();
            settings.setValue("fCoinControlFeatures", fCoinControlFeatures);
            Q_EMIT coinControlFeaturesChanged(fCoinControlFeatures);
            break;
        case Prune:
            if (settings.value("bPrune") != value) {
                settings.setValue("bPrune", value);
                setRestartRequired(true);
            }
            break;
        case PruneSize:
            if (settings.value("nPruneSize") != value) {
                settings.setValue("nPruneSize", value);
                setRestartRequired(true);
            }
            break;
#endif // ENABLE_WALLET
        case DatabaseCache:
            if (settings.value("nDatabaseCache") != value) {
                settings.setValue("nDatabaseCache", value);
                setRestartRequired(true);
            }
            break;
        case ThreadsScriptVerif:
            if (settings.value("nThreadsScriptVerif") != value) {
                settings.setValue("nThreadsScriptVerif", value);
                setRestartRequired(true);
            }
            break;
        case Listen:
            if (settings.value("fListen") != value) {
                settings.setValue("fListen", value);
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

bool OptionsModel::getProxySettings(QNetworkProxy& proxy) const
{
    // Directly query current base proxy, because
    // GUI settings can be overridden with -proxy.
    proxyType curProxy;
    if (m_node.getProxy(NET_IPV4, curProxy)) {
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(QString::fromStdString(curProxy.proxy.ToStringIP()));
        proxy.setPort(curProxy.proxy.GetPort());

        return true;
    }
    else
        proxy.setType(QNetworkProxy::NoProxy);

    return false;
}

void OptionsModel::emitCoinJoinEnabledChanged()
{
    Q_EMIT coinJoinEnabledChanged();
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

    // Make sure there is a valid theme set in the options.
    QString strActiveTheme = settings.value("theme", GUIUtil::getDefaultTheme()).toString();
    if (!GUIUtil::isValidTheme(strActiveTheme)) {
        settings.setValue("theme", GUIUtil::getDefaultTheme());
    }

    // begin PrivateSend -> CoinJoin migration
    if (settings.contains("nPrivateSendRounds") && !settings.contains("nCoinJoinRounds")) {
        settings.setValue("nCoinJoinRounds", settings.value("nPrivateSendRounds").toInt());
        settings.remove("nPrivateSendRounds");
    }
    if (settings.contains("nPrivateSendAmount") && !settings.contains("nCoinJoinAmount")) {
        settings.setValue("nCoinJoinAmount", settings.value("nPrivateSendAmount").toInt());
        settings.remove("nPrivateSendAmount");
    }
    if (settings.contains("fPrivateSendEnabled") && !settings.contains("fCoinJoinEnabled")) {
        settings.setValue("fCoinJoinEnabled", settings.value("fPrivateSendEnabled").toBool());
        settings.remove("fPrivateSendEnabled");
    }
    if (settings.contains("fPrivateSendMultiSession") && !settings.contains("fCoinJoinMultiSession")) {
        settings.setValue("fCoinJoinMultiSession", settings.value("fPrivateSendMultiSession").toBool());
        settings.remove("fPrivateSendMultiSession");
    }
    if (settings.contains("fShowAdvancedPSUI") && !settings.contains("fShowAdvancedCJUI")) {
        settings.setValue("fShowAdvancedCJUI", settings.value("fShowAdvancedPSUI").toBool());
        settings.remove("fShowAdvancedPSUI");
    }
    if (settings.contains("fShowPrivateSendPopups") && !settings.contains("fShowCoinJoinPopups")) {
        settings.setValue("fShowCoinJoinPopups", settings.value("fShowPrivateSendPopups").toBool());
        settings.remove("fShowPrivateSendPopups");
    }
    // end PrivateSend -> CoinJoin migration
}
