// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/optionsmodel.h>

#include <qt/bitcoinunits.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/guiutil_font.h>

#include <interfaces/node.h>
#include <mapport.h>
#include <net.h>
#include <netbase.h>
#include <txdb.h>       // for -dbcache defaults
#include <validation.h> // For DEFAULT_SCRIPTCHECK_THREADS
#include <util/string.h>

#ifdef ENABLE_WALLET
#include <coinjoin/options.h>
#include <wallet/wallet.h> // For DEFAULT_SPEND_ZEROCONF_CHANGE
#endif

#include <QDebug>
#include <QLatin1Char>
#include <QSettings>
#include <QStringList>
#include <QVariant>

#include <univalue.h>

const char *DEFAULT_GUI_PROXY_HOST = "127.0.0.1";

static QString GetDefaultProxyAddress();

/** Map GUI option ID to node setting name. */
static const char* SettingName(OptionsModel::OptionID option)
{
    switch (option) {
    case OptionsModel::DatabaseCache: return "dbcache";
    case OptionsModel::ThreadsScriptVerif: return "par";
    case OptionsModel::SpendZeroConfChange: return "spendzeroconfchange";
    case OptionsModel::ExternalSignerPath: return "signer";
    case OptionsModel::MapPortUPnP: return "upnp";
    case OptionsModel::MapPortNatpmp: return "natpmp";
    case OptionsModel::Listen: return "listen";
    case OptionsModel::Server: return "server";
    case OptionsModel::PruneSize: return "prune";
    case OptionsModel::Prune: return "prune";
    case OptionsModel::ProxyIP: return "proxy";
    case OptionsModel::ProxyPort: return "proxy";
    case OptionsModel::ProxyUse: return "proxy";
    case OptionsModel::ProxyIPTor: return "onion";
    case OptionsModel::ProxyPortTor: return "onion";
    case OptionsModel::ProxyUseTor: return "onion";
    case OptionsModel::Language: return "lang";
    //! Dash
    case OptionsModel::CoinJoinAmount: return "coinjoinamount";
    case OptionsModel::CoinJoinDenomsGoal: return "coinjoindenomsgoal";
    case OptionsModel::CoinJoinDenomsHardCap: return "coinjoindenomshardcap";
    case OptionsModel::CoinJoinEnabled: return "enablecoinjoin";
    case OptionsModel::CoinJoinMultiSession: return "coinjoinmultisession";
    case OptionsModel::CoinJoinRounds: return "coinjoinrounds";
    case OptionsModel::CoinJoinSessions: return "coinjoinsessions";
    case OptionsModel::FontFamily: return "font-family";
    case OptionsModel::FontScale: return "font-scale";
    case OptionsModel::FontWeightBold: return "font-weight-bold";
    case OptionsModel::FontWeightNormal: return "font-weight-normal";
    default: throw std::logic_error(strprintf("GUI option %i has no corresponding node setting.", option));
    }
}

static bool RequiresNumWorkaround(OptionsModel::OptionID option)
{
    switch (option) {
    case OptionsModel::CoinJoinAmount:
    case OptionsModel::CoinJoinDenomsGoal:
    case OptionsModel::CoinJoinDenomsHardCap:
    case OptionsModel::CoinJoinRounds:
    case OptionsModel::CoinJoinSessions:
    case OptionsModel::DatabaseCache:
    case OptionsModel::FontScale:
    case OptionsModel::FontWeightBold:
    case OptionsModel::FontWeightNormal:
    case OptionsModel::Prune:
    case OptionsModel::PruneSize:
    case OptionsModel::ThreadsScriptVerif:
        return true;
    default:
        return false;
    }
}

/** Call node.updateRwSetting() with Bitcoin 22.x workaround. */
static void UpdateRwSetting(interfaces::Node& node, OptionsModel::OptionID option, const std::string& suffix, const util::SettingsValue& value)
{
    if (value.isNum() && RequiresNumWorkaround(option)) {
        // Write certain old settings as strings, even though they are numbers,
        // because Bitcoin 22.x releases try to read these specific settings as
        // strings in addOverriddenOption() calls at startup, triggering
        // uncaught exceptions in UniValue::get_str(). These errors were fixed
        // in later releases by https://github.com/bitcoin/bitcoin/pull/24498.
        // If new numeric settings are added, they can be written as numbers
        // instead of strings, because bitcoin 22.x will not try to read these.
        node.updateRwSetting(SettingName(option) + suffix, value.getValStr());
    } else {
        node.updateRwSetting(SettingName(option) + suffix, value);
    }
}

//! Convert enabled/size values to bitcoin -prune setting.
static util::SettingsValue PruneSetting(bool prune_enabled, int prune_size_gb)
{
    assert(!prune_enabled || prune_size_gb >= 1); // PruneSizeGB and ParsePruneSizeGB never return less
    return prune_enabled ? PruneGBtoMiB(prune_size_gb) : 0;
}

//! Get pruning enabled value to show in GUI from bitcoin -prune setting.
static bool PruneEnabled(const util::SettingsValue& prune_setting)
{
    // -prune=1 setting is manual pruning mode, so disabled for purposes of the gui
    return SettingToInt(prune_setting, 0) > 1;
}

//! Get pruning size value to show in GUI from bitcoin -prune setting. If
//! pruning is not enabled, just show default recommended pruning size (2GB).
static int PruneSizeGB(const util::SettingsValue& prune_setting)
{
    int value = SettingToInt(prune_setting, 0);
    return value > 1 ? PruneMiBtoGB(value) : DEFAULT_PRUNE_TARGET_GB;
}

//! Parse pruning size value provided by user in GUI or loaded from QSettings
//! (windows registry key or qt .conf file). Smallest value that the GUI can
//! display is 1 GB, so round up if anything less is parsed.
static int ParsePruneSizeGB(const QVariant& prune_size)
{
    return std::max(1, prune_size.toInt());
}

static int GetFallbackWeightIndex(bool is_bold)
{
    if (is_bold) {
        // If the currently selected weight is not supported fallback to the second lightest weight for bold font
        // or the lightest if there is only one.
        return GUIUtil::g_font_registry.GetSupportedWeights().size() > 1 ? 1 : 0;
    } else {
        // If the currently selected weight is not supported fallback to the lightest weight for normal font.
        return 0;
    }
}

static int WeightArgToIdx(bool is_bold, int weight_arg)
{
    if (QFont::Weight weight; GUIUtil::weightFromArg(weight_arg, weight)) {
        if (int index = GUIUtil::g_font_registry.WeightToIdx(weight); index != -1) {
            return index;
        }
    }
    return GetFallbackWeightIndex(is_bold);
}

struct ProxySetting {
    bool is_set;
    QString ip;
    QString port;
};
static ProxySetting ParseProxyString(const std::string& proxy);
static std::string ProxyString(bool is_set, QString ip, QString port);

static const QLatin1String fontchoice_str_app_font{"application_font"};
static const QLatin1String fontchoice_str_embedded{"embedded"};
static const QLatin1String fontchoice_str_best_system{"best_system"};
static const QString fontchoice_str_custom_prefix{QStringLiteral("custom, ")};

QString OptionsModel::FontChoiceToString(const OptionsModel::FontChoice& f)
{
    if (std::holds_alternative<FontChoiceAbstract>(f)) {
        switch (std::get<FontChoiceAbstract>(f)) {
        case FontChoiceAbstract::ApplicationFont:
            return fontchoice_str_app_font;
        case FontChoiceAbstract::BestSystemFont:
            return fontchoice_str_best_system;
        case FontChoiceAbstract::EmbeddedFont:
            return fontchoice_str_embedded;
        }
        assert(false);
    }
    return fontchoice_str_custom_prefix + std::get<QFont>(f).toString();
}

OptionsModel::FontChoice OptionsModel::FontChoiceFromString(const QString& s)
{
    if (s == fontchoice_str_app_font) {
        return FontChoiceAbstract::ApplicationFont;
    } else if (s == fontchoice_str_best_system) {
        return FontChoiceAbstract::BestSystemFont;
    } else if (s == fontchoice_str_embedded) {
        return FontChoiceAbstract::EmbeddedFont;
    } else if (s.startsWith(fontchoice_str_custom_prefix)) {
        QFont f;
        f.fromString(s.mid(fontchoice_str_custom_prefix.size()));
        return f;
    } else {
        return FontChoiceAbstract::ApplicationFont;  // default
    }
}

OptionsModel::OptionsModel(interfaces::Node& node, QObject *parent) :
    QAbstractListModel(parent), m_node{node}
{
}

void OptionsModel::addOverriddenOption(const std::string &option)
{
    strOverriddenByCommandLine += QString::fromStdString(option) + "=" + QString::fromStdString(gArgs.GetArg(option, "")) + " ";
}

// Writes all missing QSettings with their default values
bool OptionsModel::Init(bilingual_str& error)
{
    // Initialize display settings from stored settings.
    language = QString::fromStdString(SettingToString(node().getPersistentSetting("lang"), ""));

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

    // Font Family
    if (node().isSettingIgnored("font-family")) {
        addOverriddenOption("-font-family");
    }
    if (GUIUtil::fontsLoaded()) {
        if (auto font_name = QString::fromStdString(SettingToString(node().getPersistentSetting("font-family"), GUIUtil::FontRegistry::DEFAULT_FONT.toUtf8().toStdString()));
            GUIUtil::g_font_registry.RegisterFont(font_name, /*selectable=*/true) && GUIUtil::g_font_registry.SetFont(font_name)) {
            GUIUtil::setApplicationFont();
        }
    }

    // Font Scale
    if (node().isSettingIgnored("font-scale")) {
        addOverriddenOption("-font-scale");
    }
    if (GUIUtil::fontsLoaded()) {
        GUIUtil::g_font_registry.SetFontScale(SettingToInt(node().getPersistentSetting("font-scale"), GUIUtil::FontRegistry::DEFAULT_FONT_SCALE));
    }

    // Font Weight (Normal)
    if (node().isSettingIgnored("font-weight-normal")) {
        addOverriddenOption("-font-weight-normal");
    }

    const bool override_family{isOptionOverridden("-font-family")};
    if (GUIUtil::fontsLoaded()) {
        // If font was overridden by CLI but weight wasn't, use the font's default weight
        QFont::Weight weight{GUIUtil::g_font_registry.GetWeightNormalDefault()};
        if (!override_family || isOptionOverridden("-font-weight-normal")) {
            const auto raw_weight{SettingToInt(node().getPersistentSetting("font-weight-normal"), GUIUtil::weightToArg(GUIUtil::FontRegistry::TARGET_WEIGHT_NORMAL))};
            if (!GUIUtil::weightFromArg(raw_weight, weight) || !GUIUtil::g_font_registry.IsValidWeight(weight)) {
                weight = GUIUtil::g_font_registry.IdxToWeight(GetFallbackWeightIndex(/*is_bold=*/false));
                node().forceSetting("font-weight-normal", GUIUtil::weightToArg(weight));
            }
        }
        GUIUtil::g_font_registry.SetWeightNormal(weight);
    }

    // Font Weight (Bold)
    if (node().isSettingIgnored("font-weight-bold")) {
        addOverriddenOption("-font-weight-bold");
    }
    if (GUIUtil::fontsLoaded()) {
        // If font was overridden by CLI but weight wasn't, use the font's default weight
        QFont::Weight weight{GUIUtil::g_font_registry.GetWeightBoldDefault()};
        if (!override_family || isOptionOverridden("-font-weight-bold")) {
            const auto raw_weight{SettingToInt(node().getPersistentSetting("font-weight-bold"), GUIUtil::weightToArg(GUIUtil::FontRegistry::TARGET_WEIGHT_BOLD))};
            if (!GUIUtil::weightFromArg(raw_weight, weight) || !GUIUtil::g_font_registry.IsValidWeight(weight)) {
                weight = GUIUtil::g_font_registry.IdxToWeight(GetFallbackWeightIndex(/*is_bold=*/true));
                node().forceSetting("font-weight-bold", GUIUtil::weightToArg(weight));
            }
        }
        GUIUtil::g_font_registry.SetWeightBold(weight);
    }

    // Apply font changes
    GUIUtil::setApplicationFont();

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

    if (!settings.contains("fShowMasternodesTab"))
        settings.setValue("fShowMasternodesTab", false);
    m_enable_masternodes = settings.value("fShowMasternodesTab", false).toBool();

    if (!settings.contains("show_governance_clock"))
        settings.setValue("show_governance_clock", false);
    m_show_governance_clock = settings.value("show_governance_clock", false).toBool();

    if (!settings.contains("fShowGovernanceTab"))
        settings.setValue("fShowGovernanceTab", false);
    m_enable_governance = settings.value("fShowGovernanceTab", false).toBool();

    if (!settings.contains("digits"))
        settings.setValue("digits", "2");

    // CoinJoin
    if (!settings.contains("fShowAdvancedCJUI"))
        settings.setValue("fShowAdvancedCJUI", false);
    fShowAdvancedCJUI = settings.value("fShowAdvancedCJUI", false).toBool();

    if (!settings.contains("fShowCoinJoinPopups"))
        settings.setValue("fShowCoinJoinPopups", true);

    if (!settings.contains("fLowKeysWarning"))
        settings.setValue("fLowKeysWarning", true);

    // Dust protection
    if (!settings.contains("fDustProtection"))
        settings.setValue("fDustProtection", false);
    fDustProtection = settings.value("fDustProtection", false).toBool();

    if (!settings.contains("nDustProtectionThreshold"))
        settings.setValue("nDustProtectionThreshold", (qlonglong)DEFAULT_DUST_PROTECTION_THRESHOLD);
    nDustProtectionThreshold = settings.value("nDustProtectionThreshold", (qlonglong)DEFAULT_DUST_PROTECTION_THRESHOLD).toLongLong();
#endif // ENABLE_WALLET

    // These are shared with the core or have a command-line parameter
    // and we want command-line parameters to overwrite the GUI settings.
    for (OptionID option : {DatabaseCache, ThreadsScriptVerif, SpendZeroConfChange, ExternalSignerPath, MapPortUPnP,
                            MapPortNatpmp, Listen, Server, Prune, ProxyUse, ProxyUseTor, Language, CoinJoinAmount,
                            CoinJoinDenomsGoal, CoinJoinDenomsHardCap, CoinJoinEnabled, CoinJoinMultiSession,
                            CoinJoinRounds, CoinJoinSessions}) {
        std::string setting = SettingName(option);
        if (node().isSettingIgnored(setting)) addOverriddenOption("-" + setting);
        try {
            getOption(option);
        } catch (const std::exception& e) {
            // This handles exceptions thrown by univalue that can happen if
            // settings in settings.json don't have the expected types.
            error.original = strprintf("Could not read setting \"%s\", %s.", setting, e.what());
            error.translated = tr("Could not read setting \"%1\", %2.").arg(QString::fromStdString(setting), e.what()).toStdString();
            return false;
        }
    }

    // If setting doesn't exist create it with defaults.

    // Main
    if (!settings.contains("strDataDir"))
        settings.setValue("strDataDir", GUIUtil::getDefaultDataDirectory());

    // Wallet
#ifdef ENABLE_WALLET
    if (!settings.contains("SubFeeFromAmount")) {
        settings.setValue("SubFeeFromAmount", false);
    }
    m_sub_fee_from_amount = settings.value("SubFeeFromAmount", false).toBool();
#endif

    // Display
    if (settings.contains("FontForMoney")) {
        m_font_money = FontChoiceFromString(settings.value("FontForMoney").toString());
    } else if (settings.contains("UseEmbeddedMonospacedFont")) {
        if (settings.value("UseEmbeddedMonospacedFont").toBool()) {
            m_font_money = FontChoiceAbstract::EmbeddedFont;
        } else {
            m_font_money = FontChoiceAbstract::BestSystemFont;
        }
    }
    Q_EMIT fontForMoneyChanged(getFontForMoney());

    m_mask_values = settings.value("mask_values", false).toBool();

    return true;
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
    // Backup settings.json
    gArgs.WriteSettingsFile(/*errors=*/nullptr, /*backup=*/true);

    // Clear GUI-managed settings from settings.json as node().resetSettings()
    // clears ALL settings including non-GUI settings
    for (int i{0}; i < OptionIDRowCount; i++) {
        const char* setting_name = nullptr;
        try {
            setting_name = SettingName(static_cast<OptionID>(i));
        } catch (const std::logic_error&) {
            // Option doesn't have a corresponding node setting, skip it
            continue;
        }
        node().updateRwSetting(setting_name, {});
        node().updateRwSetting(strprintf("%s-prev", setting_name), {});
    }

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

static ProxySetting ParseProxyString(const std::string& proxy)
{
    return ParseProxyString(QString::fromStdString(proxy));
}

static std::string ProxyString(bool is_set, QString ip, QString port)
{
    return is_set ? QString(ip + ":" + port).toStdString() : "";
}

static QString GetDefaultProxyAddress()
{
    return QString("%1:%2").arg(DEFAULT_GUI_PROXY_HOST).arg(DEFAULT_GUI_PROXY_PORT);
}

void OptionsModel::SetPruneTargetGB(int prune_target_gb)
{
    const util::SettingsValue cur_value = node().getPersistentSetting("prune");
    const util::SettingsValue new_value = PruneSetting(prune_target_gb > 0, prune_target_gb);

    // Force setting to take effect. It is still safe to change the value at
    // this point because this function is only called after the intro screen is
    // shown, before the node starts.
    node().forceSetting("prune", new_value);

    // When pruning is enabled, force disable governance and txindex
    if (prune_target_gb > 0) {
        node().forceSetting("disablegovernance", "1");
        node().forceSetting("txindex", "0");
    }

    // Update settings.json if value configured in intro screen is different
    // from saved value. Avoid writing settings.json if bitcoin.conf value
    // doesn't need to be overridden.
    if (PruneEnabled(cur_value) != PruneEnabled(new_value) ||
        PruneSizeGB(cur_value) != PruneSizeGB(new_value)) {
        // Call UpdateRwSetting() instead of setOption() to avoid setting
        // RestartRequired flag
        UpdateRwSetting(node(), Prune, "", new_value);
    }

    // Keep previous pruning size, if pruning was disabled.
    if (PruneEnabled(cur_value)) {
        UpdateRwSetting(node(), Prune, "-prev", PruneEnabled(new_value) ? util::SettingsValue{} : cur_value);
    }
}

QFont OptionsModel::getFontForChoice(const FontChoice& fc)
{
    QFont f;
    if (std::holds_alternative<FontChoiceAbstract>(fc)) {
        switch (std::get<FontChoiceAbstract>(fc)) {
        case FontChoiceAbstract::ApplicationFont:
            f.setFamily(GUIUtil::g_font_registry.GetFont());
            break;
        case FontChoiceAbstract::EmbeddedFont:
            f = GUIUtil::fixedPitchFont(true);
            break;
        case FontChoiceAbstract::BestSystemFont:
            f = GUIUtil::fixedPitchFont(false);
            break;
        }
    } else {
        f = std::get<QFont>(fc);
    }
    // Note: Only the font family is actually used by callers.
    // Weight and size are overridden in setMonospacedFont().
    return f;
}

QFont OptionsModel::getFontForMoney() const
{
    return getFontForChoice(m_font_money);
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

QVariant OptionsModel::getOption(OptionID option, const std::string& suffix) const
{
    auto setting = [&]{ return node().getPersistentSetting(SettingName(option) + suffix); };

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
        return SettingToBool(setting(), DEFAULT_UPNP);
#else
        return false;
#endif // USE_UPNP
    case MapPortNatpmp:
#ifdef USE_NATPMP
        return SettingToBool(setting(), DEFAULT_NATPMP);
#else
        return false;
#endif // USE_NATPMP
    case MinimizeOnClose:
        return fMinimizeOnClose;

    // default proxy
    case ProxyUse:
    case ProxyUseTor:
        return ParseProxyString(SettingToString(setting(), "")).is_set;
    case ProxyIP:
    case ProxyIPTor: {
        ProxySetting proxy = ParseProxyString(SettingToString(setting(), ""));
        if (proxy.is_set) {
            return proxy.ip;
        } else if (suffix.empty()) {
            return getOption(option, "-prev");
        } else {
            return ParseProxyString(GetDefaultProxyAddress().toStdString()).ip;
        }
    }
    case ProxyPort:
    case ProxyPortTor: {
        ProxySetting proxy = ParseProxyString(SettingToString(setting(), ""));
        if (proxy.is_set) {
            return proxy.port;
        } else if (suffix.empty()) {
            return getOption(option, "-prev");
        } else {
            return ParseProxyString(GetDefaultProxyAddress().toStdString()).port;
        }
    }

#ifdef ENABLE_WALLET
    case SpendZeroConfChange:
        return SettingToBool(setting(), wallet::DEFAULT_SPEND_ZEROCONF_CHANGE);
    case ExternalSignerPath:
        return QString::fromStdString(SettingToString(setting(), ""));
    case SubFeeFromAmount:
        return m_sub_fee_from_amount;
    case ShowMasternodesTab:
        return m_enable_masternodes;
    case ShowGovernanceClock:
        return m_show_governance_clock;
    case ShowGovernanceTab:
        return m_enable_governance;
    case CoinJoinEnabled:
        return SettingToBool(setting(), /*fDefault=*/true);
    case ShowAdvancedCJUI:
        return fShowAdvancedCJUI;
    case ShowCoinJoinPopups:
        return settings.value("fShowCoinJoinPopups");
    case LowKeysWarning:
        return settings.value("fLowKeysWarning");
    case CoinJoinSessions:
        return qlonglong(SettingToInt(setting(), DEFAULT_COINJOIN_SESSIONS));
    case CoinJoinRounds:
        return qlonglong(SettingToInt(setting(), DEFAULT_COINJOIN_ROUNDS));
    case CoinJoinAmount:
        return qlonglong(SettingToInt(setting(), DEFAULT_COINJOIN_AMOUNT));
    case CoinJoinDenomsGoal:
        return qlonglong(SettingToInt(setting(), DEFAULT_COINJOIN_DENOMS_GOAL));
    case CoinJoinDenomsHardCap:
        return qlonglong(SettingToInt(setting(), DEFAULT_COINJOIN_DENOMS_HARDCAP));
    case CoinJoinMultiSession:
        return SettingToBool(setting(), DEFAULT_COINJOIN_MULTISESSION);
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
        return QString::fromStdString(SettingToString(setting(), GUIUtil::FontRegistry::DEFAULT_FONT.toUtf8().toStdString()));
    case FontScale:
        return qlonglong(SettingToInt(setting(), GUIUtil::FontRegistry::DEFAULT_FONT_SCALE));
    case FontWeightNormal:
        return WeightArgToIdx(/*is_bold=*/false, SettingToInt(setting(), GUIUtil::g_font_registry.GetWeightNormalDefault()));
    case FontWeightBold:
        return WeightArgToIdx(/*is_bold=*/true, SettingToInt(setting(), GUIUtil::g_font_registry.GetWeightBoldDefault()));
    case Language:
        return QString::fromStdString(SettingToString(setting(), ""));
    case FontForMoney:
        return QVariant::fromValue(m_font_money);
#ifdef ENABLE_WALLET
    case CoinControlFeatures:
        return fCoinControlFeatures;
    case EnablePSBTControls:
        return settings.value("enable_psbt_controls");
    case KeepChangeAddress:
        return fKeepChangeAddress;
    case DustProtection:
        return fDustProtection;
    case DustProtectionThreshold:
        return qlonglong(nDustProtectionThreshold);
#endif // ENABLE_WALLET
    case Prune:
        return PruneEnabled(setting());
    case PruneSize:
        return PruneEnabled(setting()) ? PruneSizeGB(setting()) :
               suffix.empty()          ? getOption(option, "-prev") :
                                         DEFAULT_PRUNE_TARGET_GB;
    case DatabaseCache:
        return qlonglong(SettingToInt(setting(), nDefaultDbCache));
    case ThreadsScriptVerif:
        return qlonglong(SettingToInt(setting(), DEFAULT_SCRIPTCHECK_THREADS));
    case Listen:
        return SettingToBool(setting(), DEFAULT_LISTEN);
    case Server:
        return SettingToBool(setting(), false);
    case MaskValues:
        return m_mask_values;
    default:
        return QVariant();
    }
}

bool OptionsModel::setOption(OptionID option, const QVariant& value, const std::string& suffix)
{
    auto changed = [&] { return value.isValid() && value != getOption(option, suffix); };
    auto update = [&](const util::SettingsValue& value) { return UpdateRwSetting(node(), option, suffix, value); };

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
            update(value.toBool());
            node().mapPort(value.toBool(), getOption(MapPortNatpmp).toBool());
        }
        break;
    case MapPortNatpmp: // core option - can be changed on-the-fly
        if (changed()) {
            update(value.toBool());
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
            if (suffix.empty() && !value.toBool()) setOption(option, true, "-prev");
            update(ProxyString(value.toBool(), getOption(ProxyIP).toString(), getOption(ProxyPort).toString()));
            if (suffix.empty() && value.toBool()) UpdateRwSetting(node(), option, "-prev", {});
            if (suffix.empty()) setRestartRequired(true);
        }
        break;
    case ProxyIP:
        if (changed()) {
            if (suffix.empty() && !getOption(ProxyUse).toBool()) {
                setOption(option, value, "-prev");
            } else {
                update(ProxyString(true, value.toString(), getOption(ProxyPort).toString()));
            }
            if (suffix.empty() && getOption(ProxyUse).toBool()) setRestartRequired(true);
        }
        break;
    case ProxyPort:
        if (changed()) {
            if (suffix.empty() && !getOption(ProxyUse).toBool()) {
                setOption(option, value, "-prev");
            } else {
                update(ProxyString(true, getOption(ProxyIP).toString(), value.toString()));
            }
            if (suffix.empty() && getOption(ProxyUse).toBool()) setRestartRequired(true);
        }
        break;

    // separate Tor proxy
    case ProxyUseTor:
        if (changed()) {
            if (suffix.empty() && !value.toBool()) setOption(option, true, "-prev");
            update(ProxyString(value.toBool(), getOption(ProxyIPTor).toString(), getOption(ProxyPortTor).toString()));
            if (suffix.empty() && value.toBool()) UpdateRwSetting(node(), option, "-prev", {});
            if (suffix.empty()) setRestartRequired(true);
        }
        break;
    case ProxyIPTor:
        if (changed()) {
            if (suffix.empty() && !getOption(ProxyUseTor).toBool()) {
                setOption(option, value, "-prev");
            } else {
                update(ProxyString(true, value.toString(), getOption(ProxyPortTor).toString()));
            }
            if (suffix.empty() && getOption(ProxyUseTor).toBool()) setRestartRequired(true);
        }
        break;
    case ProxyPortTor:
        if (changed()) {
            if (suffix.empty() && !getOption(ProxyUseTor).toBool()) {
                setOption(option, value, "-prev");
            } else {
                update(ProxyString(true, getOption(ProxyIPTor).toString(), value.toString()));
            }
            if (suffix.empty() && getOption(ProxyUseTor).toBool()) setRestartRequired(true);
        }
        break;

#ifdef ENABLE_WALLET
    case SpendZeroConfChange:
        if (changed()) {
            update(value.toBool());
            setRestartRequired(true);
        }
        break;
    case ExternalSignerPath:
        if (changed()) {
            update(value.toString().toStdString());
            setRestartRequired(true);
        }
        break;
    case ShowMasternodesTab:
        if (changed()) {
            m_enable_masternodes = value.toBool();
            settings.setValue("fShowMasternodesTab", m_enable_masternodes);
            Q_EMIT showMasternodesChanged();
        }
        break;
    case SubFeeFromAmount:
        m_sub_fee_from_amount = value.toBool();
        settings.setValue("SubFeeFromAmount", m_sub_fee_from_amount);
        break;
    case ShowGovernanceClock:
        if (changed()) {
            m_show_governance_clock = value.toBool();
            settings.setValue("show_governance_clock", m_show_governance_clock);
            Q_EMIT showGovernanceClockChanged();
        }
        break;
    case ShowGovernanceTab:
        if (changed()) {
            m_enable_governance = value.toBool();
            settings.setValue("fShowGovernanceTab", m_enable_governance);
            Q_EMIT showGovernanceChanged();
        }
        break;
    case CoinJoinEnabled:
        if (changed()) {
            node().coinJoinOptions().setEnabled(value.toBool());
            update(value.toBool());
            Q_EMIT showCoinJoinChanged();
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
        if (changed()) {
            node().coinJoinOptions().setSessions(value.toInt());
            update(value.toInt());
            Q_EMIT coinJoinRoundsChanged();
        }
        break;
    case CoinJoinRounds:
        if (changed()) {
            node().coinJoinOptions().setRounds(value.toInt());
            update(value.toInt());
            Q_EMIT coinJoinRoundsChanged();
        }
        break;
    case CoinJoinAmount:
        if (changed()) {
            node().coinJoinOptions().setAmount(value.toInt());
            update(value.toInt());
            Q_EMIT coinJoinAmountChanged();
        }
        break;
    case CoinJoinDenomsGoal:
        if (changed()) {
            node().coinJoinOptions().setDenomsGoal(value.toInt());
            update(value.toInt());
        }
        break;
    case CoinJoinDenomsHardCap:
        if (changed()) {
            node().coinJoinOptions().setDenomsHardCap(value.toInt());
            update(value.toInt());
        }
        break;
    case CoinJoinMultiSession:
        if (changed()) {
            node().coinJoinOptions().setMultiSessionEnabled(value.toBool());
            update(value.toBool());
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
        if (changed()) {
            update(value.toString().toStdString());
        }
        break;
    case FontScale:
        if (changed()) {
            update(value.toInt());
        }
        break;
    case FontWeightNormal: {
        if (changed()) {
            update(GUIUtil::weightToArg(GUIUtil::g_font_registry.IdxToWeight(value.toInt())));
        }
        break;
    }
    case FontWeightBold: {
        if (changed()) {
            update(GUIUtil::weightToArg(GUIUtil::g_font_registry.IdxToWeight(value.toInt())));
        }
        break;
    }
    case Language:
        if (changed()) {
            update(value.toString().toStdString());
            setRestartRequired(true);
        }
        break;
    case FontForMoney:
    {
        const auto& new_font = value.value<FontChoice>();
        if (m_font_money != new_font) {
            settings.setValue("FontForMoney", FontChoiceToString(new_font));
            m_font_money = new_font;
        }
        Q_EMIT fontForMoneyChanged(getFontForMoney());
        break;
    }
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
    case DustProtection:
        fDustProtection = value.toBool();
        settings.setValue("fDustProtection", fDustProtection);
        Q_EMIT dustProtectionChanged();
        break;
    case DustProtectionThreshold:
        nDustProtectionThreshold = value.toLongLong();
        settings.setValue("nDustProtectionThreshold", qlonglong(nDustProtectionThreshold));
        Q_EMIT dustProtectionChanged();
        break;
#endif // ENABLE_WALLET
    case Prune:
        if (changed()) {
            if (suffix.empty() && !value.toBool()) setOption(option, true, "-prev");
            update(PruneSetting(value.toBool(), getOption(PruneSize).toInt()));
            if (suffix.empty() && value.toBool()) UpdateRwSetting(node(), option, "-prev", {});
            if (suffix.empty()) setRestartRequired(true);
        }
        break;
    case PruneSize:
        if (changed()) {
            if (suffix.empty() && !getOption(Prune).toBool()) {
                setOption(option, value, "-prev");
            } else {
                update(PruneSetting(true, ParsePruneSizeGB(value)));
            }
            if (suffix.empty() && getOption(Prune).toBool()) setRestartRequired(true);
        }
        break;
    case DatabaseCache:
        if (changed()) {
            update(static_cast<int64_t>(value.toLongLong()));
            setRestartRequired(true);
        }
        break;
    case ThreadsScriptVerif:
        if (changed()) {
            update(static_cast<int64_t>(value.toLongLong()));
            setRestartRequired(true);
        }
        break;
    case Listen:
    case Server:
        if (changed()) {
            update(value.toBool());
            setRestartRequired(true);
        }
        break;
    case MaskValues:
        m_mask_values = value.toBool();
        settings.setValue("mask_values", m_mask_values);
        break;
    default:
        break;
    }

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

        if (settingsVersion < 230100) {
            settings.remove("fPrivateSendEnabled");
            settings.remove("fPrivateSendMultiSession");
            settings.remove("fShowAdvancedPSUI");
            settings.remove("fShowPrivateSendPopups");
            settings.remove("nPrivateSendAmount");
            settings.remove("nPrivateSendRounds");
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

    // Migrate and delete legacy GUI settings that have now moved to <datadir>/settings.json.
    auto migrate_setting = [&](OptionID option, const QString& qt_name) {
        if (!settings.contains(qt_name)) return;
        QVariant value = settings.value(qt_name);
        if (node().getPersistentSetting(SettingName(option)).isNull()) {
            if (option == ProxyIP) {
                ProxySetting parsed = ParseProxyString(value.toString());
                setOption(ProxyIP, parsed.ip);
                setOption(ProxyPort, parsed.port);
            } else if (option == ProxyIPTor) {
                ProxySetting parsed = ParseProxyString(value.toString());
                setOption(ProxyIPTor, parsed.ip);
                setOption(ProxyPortTor, parsed.port);
            } else if (option == FontWeightNormal || option == FontWeightBold) {
                setOption(option, WeightArgToIdx(/*is_bold=*/option == FontWeightBold, value.toInt()));
            } else {
                setOption(option, value);
            }
        }
        settings.remove(qt_name);
    };

    migrate_setting(DatabaseCache, "nDatabaseCache");
    migrate_setting(ThreadsScriptVerif, "nThreadsScriptVerif");
#ifdef ENABLE_WALLET
    migrate_setting(SpendZeroConfChange, "bSpendZeroConfChange");
    migrate_setting(ExternalSignerPath, "external_signer_path");
#endif
    migrate_setting(MapPortUPnP, "fUseUPnP");
    migrate_setting(MapPortNatpmp, "fUseNatpmp");
    migrate_setting(Listen, "fListen");
    migrate_setting(Server, "server");
    migrate_setting(PruneSize, "nPruneSize");
    migrate_setting(Prune, "bPrune");
    migrate_setting(ProxyIP, "addrProxy");
    migrate_setting(ProxyUse, "fUseProxy");
    migrate_setting(ProxyIPTor, "addrSeparateProxyTor");
    migrate_setting(ProxyUseTor, "fUseSeparateProxyTor");
    migrate_setting(Language, "language");

    //! Dash
    if (GUIUtil::fontsLoaded()) {
        migrate_setting(FontFamily, "fontFamily");
        migrate_setting(FontScale, "fontScale");
        migrate_setting(FontWeightBold, "fontWeightBold");
        migrate_setting(FontWeightNormal, "fontWeightNormal");
    }
#ifdef ENABLE_WALLET
    migrate_setting(CoinJoinAmount, "nCoinJoinAmount");
    migrate_setting(CoinJoinDenomsGoal, "nCoinJoinDenomsGoal");
    migrate_setting(CoinJoinDenomsHardCap, "nCoinJoinDenomsHardCap");
    migrate_setting(CoinJoinEnabled, "fCoinJoinEnabled");
    migrate_setting(CoinJoinMultiSession, "fCoinJoinMultiSession");
    migrate_setting(CoinJoinRounds, "nCoinJoinRounds");
    migrate_setting(CoinJoinSessions, "nCoinJoinSessions");
#endif

    // In case migrating QSettings caused any settings value to change, rerun
    // parameter interaction code to update other settings. This is particularly
    // important for the -listen setting, which should cause -listenonion, -upnp,
    // and other settings to default to false if it was set to false.
    // (https://github.com/bitcoin-core/gui/issues/567).
    node().initParameterInteraction();
}
