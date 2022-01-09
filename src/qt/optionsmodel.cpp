// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2024 The Dash Core developers
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
#include <validation.h> // For DEFAULT_SCRIPTCHECK_THREADS
#include <util/string.h>

#ifdef ENABLE_WALLET
#include <coinjoin/options.h>
#endif

#include <QDebug>
#include <QLatin1Char>
#include <QSettings>
#include <QStringList>
#include <QVariant>

const char *DEFAULT_GUI_PROXY_HOST = "127.0.0.1";

static QString GetDefaultProxyAddress();

OptionsModel::OptionsModel(QObject *parent, bool resetSettings) :
    QAbstractListModel(parent)
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
    if (!settings.contains("DisplayDashUnit")) {
        settings.setValue("DisplayDashUnit", QVariant::fromValue(BitcoinUnit::DASH));
    }
    QVariant unit = settings.value("DisplayDashUnit");
    if (unit.canConvert<BitcoinUnit>()) {
        m_display_bitcoin_unit = unit.value<BitcoinUnit>();
    } else {
        m_display_bitcoin_unit = BitcoinUnit::DASH;
        settings.setValue("DisplayDashUnit", QVariant::fromValue(m_display_bitcoin_unit));
    }

    if (!settings.contains("strThirdPartyTxUrls"))
        settings.setValue("strThirdPartyTxUrls", "");
    strThirdPartyTxUrls = settings.value("strThirdPartyTxUrls", "").toString();

    // Appearance
    if (!settings.contains("theme"))
        settings.setValue("theme", GUIUtil::getDefaultTheme());

    if (!settings.contains("fontFamily"))
        settings.setValue("fontFamily", GUIUtil::fontFamilyToString(GUIUtil::getFontFamilyDefault()));
    if (gArgs.SoftSetArg("-font-family", settings.value("fontFamily").toString().toStdString())) {
        if (GUIUtil::fontsLoaded()) {
            GUIUtil::setFontFamily(GUIUtil::fontFamilyFromString(settings.value("fontFamily").toString()));
        }
    } else {
        addOverriddenOption("-font-family");
    }

    if (!settings.contains("fontScale"))
        settings.setValue("fontScale", GUIUtil::getFontScaleDefault());
    if (gArgs.SoftSetArg("-font-scale", settings.value("fontScale").toString().toStdString())) {
        if (GUIUtil::fontsLoaded()) {
            GUIUtil::setFontScale(settings.value("fontScale").toInt());
        }
    } else {
        addOverriddenOption("-font-scale");
    }

    if (!settings.contains("fontWeightNormal"))
        settings.setValue("fontWeightNormal", GUIUtil::weightToArg(GUIUtil::getFontWeightNormalDefault()));
    if (gArgs.SoftSetArg("-font-weight-normal", settings.value("fontWeightNormal").toString().toStdString())) {
        if (GUIUtil::fontsLoaded()) {
            QFont::Weight weight;
            GUIUtil::weightFromArg(settings.value("fontWeightNormal").toInt(), weight);
            if (!GUIUtil::isSupportedWeight(weight)) {
                // If the currently selected weight is not supported fallback to the lightest weight for normal font.
                weight = GUIUtil::getSupportedWeights().front();
                settings.setValue("fontWeightNormal", GUIUtil::weightToArg(weight));
            }
            GUIUtil::setFontWeightNormal(weight);
        }
    } else {
        addOverriddenOption("-font-weight-normal");
    }

    if (!settings.contains("fontWeightBold"))
        settings.setValue("fontWeightBold", GUIUtil::weightToArg(GUIUtil::getFontWeightBoldDefault()));
    if (gArgs.SoftSetArg("-font-weight-bold", settings.value("fontWeightBold").toString().toStdString())) {
        if (GUIUtil::fontsLoaded()) {
            QFont::Weight weight;
            GUIUtil::weightFromArg(settings.value("fontWeightBold").toInt(), weight);
            if (!GUIUtil::isSupportedWeight(weight)) {
                // If the currently selected weight is not supported fallback to the second lightest weight for bold font
                // or the lightest if there is only one.
                auto vecSupported = GUIUtil::getSupportedWeights();
                weight = vecSupported[vecSupported.size() > 1 ? 1 : 0];
                settings.setValue("fontWeightBold", GUIUtil::weightToArg(weight));
            }
            GUIUtil::setFontWeightBold(weight);
        }
    } else {
        addOverriddenOption("-font-weight-bold");
    }

#ifdef ENABLE_WALLET
    if (!settings.contains("fCoinControlFeatures"))
        settings.setValue("fCoinControlFeatures", false);
    fCoinControlFeatures = settings.value("fCoinControlFeatures", false).toBool();

    if (!settings.contains("enable_psbt_controls")) {
        settings.setValue("enable_psbt_controls", false);
    }
    m_enable_psbt_controls = settings.value("enable_psbt_controls", false).toBool();

    if (!settings.contains("fKeepChangeAddress"))
        settings.setValue("fKeepChangeAddress", false);
    fKeepChangeAddress = settings.value("fKeepChangeAddress", false).toBool();

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
    // If gArgs.SoftSetArg() or m_node.softSetBoolArg() return false we were overridden
    // by command-line and show this in the UI.

    // Main
    if (!settings.contains("bPrune"))
        settings.setValue("bPrune", false);
    if (!settings.contains("nPruneSize"))
        settings.setValue("nPruneSize", DEFAULT_PRUNE_TARGET_GB);
    SetPruneEnabled(settings.value("bPrune").toBool());

    if (!settings.contains("nDatabaseCache"))
        settings.setValue("nDatabaseCache", (qint64)nDefaultDbCache);
    if (!gArgs.SoftSetArg("-dbcache", settings.value("nDatabaseCache").toString().toStdString()))
        addOverriddenOption("-dbcache");

    if (!settings.contains("nThreadsScriptVerif"))
        settings.setValue("nThreadsScriptVerif", DEFAULT_SCRIPTCHECK_THREADS);
    if (!gArgs.SoftSetArg("-par", settings.value("nThreadsScriptVerif").toString().toStdString()))
        addOverriddenOption("-par");

    if (!settings.contains("strDataDir"))
        settings.setValue("strDataDir", GUIUtil::getDefaultDataDirectory());

    // Wallet
#ifdef ENABLE_WALLET
    if (!settings.contains("bSpendZeroConfChange"))
        settings.setValue("bSpendZeroConfChange", true);
    if (!gArgs.SoftSetBoolArg("-spendzeroconfchange", settings.value("bSpendZeroConfChange").toBool()))
        addOverriddenOption("-spendzeroconfchange");

    if (!settings.contains("SubFeeFromAmount")) {
        settings.setValue("SubFeeFromAmount", false);
    }
    m_sub_fee_from_amount = settings.value("SubFeeFromAmount", false).toBool();

    // CoinJoin
    if (!settings.contains("nCoinJoinSessions"))
        settings.setValue("nCoinJoinSessions", DEFAULT_COINJOIN_SESSIONS);
    if (!gArgs.SoftSetArg("-coinjoinsessions", settings.value("nCoinJoinSessions").toString().toStdString()))
        addOverriddenOption("-coinjoinsessions");

    if (!settings.contains("nCoinJoinRounds"))
        settings.setValue("nCoinJoinRounds", DEFAULT_COINJOIN_ROUNDS);
    if (!gArgs.SoftSetArg("-coinjoinrounds", settings.value("nCoinJoinRounds").toString().toStdString()))
        addOverriddenOption("-coinjoinrounds");

    if (!settings.contains("nCoinJoinAmount"))
        settings.setValue("nCoinJoinAmount", DEFAULT_COINJOIN_AMOUNT);
    if (!gArgs.SoftSetArg("-coinjoinamount", settings.value("nCoinJoinAmount").toString().toStdString()))
        addOverriddenOption("-coinjoinamount");

    if (!settings.contains("fCoinJoinMultiSession"))
        settings.setValue("fCoinJoinMultiSession", DEFAULT_COINJOIN_MULTISESSION);
    if (!gArgs.SoftSetBoolArg("-coinjoinmultisession", settings.value("fCoinJoinMultiSession").toBool()))
        addOverriddenOption("-coinjoinmultisession");

    if (!settings.contains("nCoinJoinDenomsGoal"))
        settings.setValue("nCoinJoinDenomsGoal", DEFAULT_COINJOIN_DENOMS_GOAL);
    if (!gArgs.SoftSetArg("-coinjoindenomsgoal", settings.value("nCoinJoinDenomsGoal").toString().toStdString()))
        addOverriddenOption("-coinjoindenomsgoal");

    if (!settings.contains("nCoinJoinDenomsHardCap"))
        settings.setValue("nCoinJoinDenomsHardCap", DEFAULT_COINJOIN_DENOMS_HARDCAP);
    if (!gArgs.SoftSetArg("-coinjoindenomshardcap", settings.value("nCoinJoinDenomsHardCap").toString().toStdString()))
        addOverriddenOption("-coinjoindenomshardcap");
#endif

    // Network
    if (!settings.contains("fUseUPnP"))
        settings.setValue("fUseUPnP", DEFAULT_UPNP);
    if (!gArgs.SoftSetBoolArg("-upnp", settings.value("fUseUPnP").toBool()))
        addOverriddenOption("-upnp");

    if (!settings.contains("fUseNatpmp")) {
        settings.setValue("fUseNatpmp", DEFAULT_NATPMP);
    }
    if (!gArgs.SoftSetBoolArg("-natpmp", settings.value("fUseNatpmp").toBool())) {
        addOverriddenOption("-natpmp");
    }

    if (!settings.contains("fListen"))
        settings.setValue("fListen", DEFAULT_LISTEN);
    const bool listen{settings.value("fListen").toBool()};
    if (!gArgs.SoftSetBoolArg("-listen", listen)) {
        addOverriddenOption("-listen");
    } else if (!listen) {
        // We successfully set -listen=0, thus mimic the logic from InitParameterInteraction():
        // "parameter interaction: -listen=0 -> setting -listenonion=0".
        //
        // Both -listen and -listenonion default to true.
        //
        // The call order is:
        //
        // InitParameterInteraction()
        //     would set -listenonion=0 if it sees -listen=0, but for bitcoin-qt with
        //     fListen=false -listen is 1 at this point
        //
        // OptionsModel::Init()
        //     (this method) can flip -listen from 1 to 0 if fListen=false
        //
        // AppInitParameterInteraction()
        //     raises an error if -listen=0 and -listenonion=1
        gArgs.SoftSetBoolArg("-listenonion", false);
    }

    if (!settings.contains("server")) {
        settings.setValue("server", false);
    }
    if (!gArgs.SoftSetBoolArg("-server", settings.value("server").toBool())) {
        addOverriddenOption("-server");
    }

    if (!settings.contains("fUseProxy"))
        settings.setValue("fUseProxy", false);
    if (!settings.contains("addrProxy"))
        settings.setValue("addrProxy", GetDefaultProxyAddress());
    // Only try to set -proxy, if user has enabled fUseProxy
    if ((settings.value("fUseProxy").toBool() && !gArgs.SoftSetArg("-proxy", settings.value("addrProxy").toString().toStdString())))
        addOverriddenOption("-proxy");
    else if(!settings.value("fUseProxy").toBool() && !gArgs.GetArg("-proxy", "").empty())
        addOverriddenOption("-proxy");

    if (!settings.contains("fUseSeparateProxyTor"))
        settings.setValue("fUseSeparateProxyTor", false);
    if (!settings.contains("addrSeparateProxyTor"))
        settings.setValue("addrSeparateProxyTor", GetDefaultProxyAddress());
    // Only try to set -onion, if user has enabled fUseSeparateProxyTor
    if ((settings.value("fUseSeparateProxyTor").toBool() && !gArgs.SoftSetArg("-onion", settings.value("addrSeparateProxyTor").toString().toStdString())))
        addOverriddenOption("-onion");
    else if(!settings.value("fUseSeparateProxyTor").toBool() && !gArgs.GetArg("-onion", "").empty())
        addOverriddenOption("-onion");

    // Display
    if (!settings.contains("language"))
        settings.setValue("language", "");
    if (!gArgs.SoftSetArg("-lang", settings.value("language").toString().toStdString()))
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
    qInfo() << "Backing up GUI settings to" << GUIUtil::PathToQString(filename);
    QSettings dst(GUIUtil::PathToQString(filename), QSettings::IniFormat);
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
    QStringList ip_port = GUIUtil::SplitSkipEmptyParts(settings.value(name).toString(), ":");
    if (ip_port.size() == 2) {
        return {true, ip_port.at(0), ip_port.at(1)};
    } else { // Invalid: return default
        return default_val;
    }
}

static void SetProxySetting(QSettings &settings, const QString &name, const ProxySetting &ip_port)
{
    settings.setValue(name, QString{ip_port.ip + QLatin1Char(':') + ip_port.port});
}

static QString GetDefaultProxyAddress()
{
    return QString("%1:%2").arg(DEFAULT_GUI_PROXY_HOST).arg(DEFAULT_GUI_PROXY_PORT);
}

void OptionsModel::SetPruneEnabled(bool prune, bool force)
{
    QSettings settings;
    settings.setValue("bPrune", prune);
    const int64_t prune_target_mib = PruneGBtoMiB(settings.value("nPruneSize").toInt());
    std::string prune_val = prune ? ToString(prune_target_mib) : "0";
    if (force) {
        gArgs.ForceSetArg("-prune", prune_val);
        if (prune) {
            gArgs.ForceSetArg("-disablegovernance", "1");
            gArgs.ForceSetArg("-txindex", "0");
        }
        return;
    }
    if (!gArgs.SoftSetArg("-prune", prune_val)) {
        addOverriddenOption("-prune");
    }
    if (gArgs.GetIntArg("-prune", 0) > 0) {
        gArgs.SoftSetBoolArg("-disablegovernance", true);
        gArgs.SoftSetBoolArg("-txindex", false);
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
        QSettings settings;
        switch(index.row())
        {
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
#endif // USE_UPNP
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
        case SubFeeFromAmount:
            return m_sub_fee_from_amount;
        case ShowMasternodesTab:
            return settings.value("fShowMasternodesTab");
        case ShowGovernanceTab:
            return settings.value("fShowGovernanceTab");
        case CoinJoinEnabled:
            return settings.value("fCoinJoinEnabled");
        case ShowAdvancedCJUI:
            return fShowAdvancedCJUI;
        case ShowCoinJoinPopups:
            return settings.value("fShowCoinJoinPopups");
        case LowKeysWarning:
            return settings.value("fLowKeysWarning");
        case CoinJoinSessions:
            return settings.value("nCoinJoinSessions");
        case CoinJoinRounds:
            return settings.value("nCoinJoinRounds");
        case CoinJoinAmount:
            return settings.value("nCoinJoinAmount");
        case CoinJoinDenomsGoal:
            return settings.value("nCoinJoinDenomsGoal");
        case CoinJoinDenomsHardCap:
            return settings.value("nCoinJoinDenomsHardCap");
        case CoinJoinMultiSession:
            return settings.value("fCoinJoinMultiSession");
#endif
        case DisplayUnit:
            return QVariant::fromValue(m_display_bitcoin_unit);
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
            int nIndex = GUIUtil::supportedWeightToIndex(weight);
            assert(nIndex != -1);
            return nIndex;
        }
        case FontWeightBold: {
            QFont::Weight weight;
            GUIUtil::weightFromArg(settings.value("fontWeightBold").toInt(), weight);
            int nIndex = GUIUtil::supportedWeightToIndex(weight);
            assert(nIndex != -1);
            return nIndex;
        }
        case Language:
            return settings.value("language");
#ifdef ENABLE_WALLET
        case CoinControlFeatures:
            return fCoinControlFeatures;
        case EnablePSBTControls:
            return settings.value("enable_psbt_controls");
        case KeepChangeAddress:
            return fKeepChangeAddress;
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
        case Server:
            return settings.value("server");
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
            settings.setValue("fUseUPnP", value.toBool());
            break;
        case MapPortNatpmp: // core option - can be changed on-the-fly
            settings.setValue("fUseNatpmp", value.toBool());
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
        case SubFeeFromAmount:
            m_sub_fee_from_amount = value.toBool();
            settings.setValue("SubFeeFromAmount", m_sub_fee_from_amount);
            break;
        case ShowGovernanceTab:
            if (settings.value("fShowGovernanceTab") != value) {
                settings.setValue("fShowGovernanceTab", value);
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
        case CoinJoinSessions:
            if (settings.value("nCoinJoinSessions") != value) {
                node().coinJoinOptions().setSessions(value.toInt());
                settings.setValue("nCoinJoinSessions", node().coinJoinOptions().getSessions());
                Q_EMIT coinJoinRoundsChanged();
            }
            break;
        case CoinJoinRounds:
            if (settings.value("nCoinJoinRounds") != value)
            {
                node().coinJoinOptions().setRounds(value.toInt());
                settings.setValue("nCoinJoinRounds", node().coinJoinOptions().getRounds());
                Q_EMIT coinJoinRoundsChanged();
            }
            break;
        case CoinJoinAmount:
            if (settings.value("nCoinJoinAmount") != value)
            {
                node().coinJoinOptions().setAmount(value.toInt());
                settings.setValue("nCoinJoinAmount", node().coinJoinOptions().getAmount());
                Q_EMIT coinJoinAmountChanged();
            }
            break;
        case CoinJoinDenomsGoal:
            if (settings.value("nCoinJoinDenomsGoal") != value) {
                node().coinJoinOptions().setDenomsGoal(value.toInt());
                settings.setValue("nCoinJoinDenomsGoal", node().coinJoinOptions().getDenomsGoal());
            }
            break;
        case CoinJoinDenomsHardCap:
            if (settings.value("nCoinJoinDenomsHardCap") != value) {
                node().coinJoinOptions().setDenomsHardCap(value.toInt());
                settings.setValue("nCoinJoinDenomsHardCap", node().coinJoinOptions().getDenomsHardCap());
            }
            break;
        case CoinJoinMultiSession:
            if (settings.value("fCoinJoinMultiSession") != value)
            {
                node().coinJoinOptions().setMultiSessionEnabled(value.toBool());
                settings.setValue("fCoinJoinMultiSession", node().coinJoinOptions().isMultiSessionEnabled());
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
        case EnablePSBTControls:
            m_enable_psbt_controls = value.toBool();
            settings.setValue("enable_psbt_controls", m_enable_psbt_controls);
            break;
        case KeepChangeAddress:
            fKeepChangeAddress = value.toBool();
            settings.setValue("fKeepChangeAddress", fKeepChangeAddress);
            Q_EMIT keepChangeAddressChanged(fKeepChangeAddress);
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
        case Server:
            if (settings.value("server") != value) {
                settings.setValue("server", value);
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

void OptionsModel::setDisplayUnit(const QVariant& new_unit)
{
    if (new_unit.isNull() || new_unit.value<BitcoinUnit>() == m_display_bitcoin_unit) return;
    m_display_bitcoin_unit = new_unit.value<BitcoinUnit>();
    QSettings settings;
    settings.setValue("DisplayDashUnit", QVariant::fromValue(m_display_bitcoin_unit));
    Q_EMIT displayUnitChanged(m_display_bitcoin_unit);
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

        if (settingsVersion < 170000) {
            settings.remove("nWindowPos");
            settings.remove("nWindowSize");
            settings.remove("fMigrationDone121");
            settings.remove("bUseInstantX");
            settings.remove("bUseInstantSend");
            settings.remove("bUseDarkSend");
            settings.remove("bUsePrivateSend");
        }

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
