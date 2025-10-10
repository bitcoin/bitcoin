// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <qt/optionsmodel.h>

#include <qt/bitcoinunits.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/tonalutils.h>

#include <chainparams.h>
#include <common/args.h>
#include <consensus/consensus.h>
#include <index/blockfilterindex.h>
#include <interfaces/node.h>
#include <kernel/mempool_options.h> // for DEFAULT_MAX_MEMPOOL_SIZE_MB, DEFAULT_MEMPOOL_EXPIRY_HOURS
#include <mapport.h>
#include <policy/settings.h>
#include <net.h>
#include <net_processing.h>
#include <netbase.h>
#include <node/caches.h>
#include <node/chainstatemanager_args.h>
#include <node/context.h>
#include <node/mempool_args.h> // for ParseDustDynamicOpt
#include <outputtype.h>
#include <policy/settings.h>
#include <util/moneystr.h> // for FormatMoney
#include <util/string.h>
#include <validation.h>    // For DEFAULT_SCRIPTCHECK_THREADS
#include <wallet/wallet.h> // For DEFAULT_SPEND_ZEROCONF_CHANGE

#ifdef ENABLE_WALLET
#include <interfaces/wallet.h>
#endif

#include <chrono>
#include <string>
#include <unordered_set>
#include <utility>

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
    case OptionsModel::walletrbf: return "walletrbf";
    case OptionsModel::addresstype: return "addresstype";
    case OptionsModel::PruneSizeMiB: return "prune";
    case OptionsModel::PruneTristate: return "prune";
    case OptionsModel::ProxyIP: return "proxy";
    case OptionsModel::ProxyPort: return "proxy";
    case OptionsModel::ProxyUse: return "proxy";
    case OptionsModel::ProxyIPTor: return "onion";
    case OptionsModel::ProxyPortTor: return "onion";
    case OptionsModel::ProxyUseTor: return "onion";
    case OptionsModel::Language: return "lang";
    case OptionsModel::maxuploadtarget: return "maxuploadtarget";
    case OptionsModel::peerbloomfilters: return "peerbloomfilters";
    case OptionsModel::peerblockfilters: return "peerblockfilters";
    case OptionsModel::mempoolreplacement: return "mempoolreplacement";
    case OptionsModel::mempooltruc: return "mempooltruc";
    case OptionsModel::maxorphantx: return "maxorphantx";
    case OptionsModel::maxmempool: return "maxmempool";
    case OptionsModel::incrementalrelayfee: return "incrementalrelayfee";
    case OptionsModel::mempoolexpiry: return "mempoolexpiry";
    case OptionsModel::rejectunknownscripts: return "rejectunknownscripts";
    case OptionsModel::rejectunknownwitness: return "rejectunknownwitness";
    case OptionsModel::rejectparasites: return "rejectparasites";
    case OptionsModel::rejecttokens: return "rejecttokens";
    case OptionsModel::rejectspkreuse: return "rejectspkreuse";
    case OptionsModel::minrelaytxfee: return "minrelaytxfee";
    case OptionsModel::minrelaycoinblocks: return "minrelaycoinblocks";
    case OptionsModel::minrelaymaturity: return "minrelaymaturity";
    case OptionsModel::bytespersigop: return "bytespersigop";
    case OptionsModel::bytespersigopstrict: return "bytespersigopstrict";
    case OptionsModel::limitancestorcount: return "limitancestorcount";
    case OptionsModel::limitancestorsize: return "limitancestorsize";
    case OptionsModel::limitdescendantcount: return "limitdescendantcount";
    case OptionsModel::limitdescendantsize: return "limitdescendantsize";
    case OptionsModel::rejectbarepubkey: return "rejectbarepubkey";
    case OptionsModel::rejectbaremultisig: return "rejectbaremultisig";
    case OptionsModel::permitephemeral: return "permitephemeral";
    case OptionsModel::rejectbareanchor: return "rejectbareanchor";
    case OptionsModel::rejectbaredatacarrier: return "rejectbaredatacarrier";
    case OptionsModel::maxscriptsize: return "maxscriptsize";
    case OptionsModel::maxtxlegacysigops: return "maxtxlegacysigops";
    case OptionsModel::datacarriercost: return "datacarriercost";
    case OptionsModel::datacarriersize: return "datacarriersize";
    case OptionsModel::rejectnonstddatacarrier: return "rejectnonstddatacarrier";
    case OptionsModel::dustrelayfee: return "dustrelayfee";
    case OptionsModel::dustdynamic: return "dustdynamic";
    case OptionsModel::blockmintxfee: return "blockmintxfee";
    case OptionsModel::blockmaxsize: return "blockmaxsize";
    case OptionsModel::blockprioritysize: return "blockprioritysize";
    case OptionsModel::blockmaxweight: return "blockmaxweight";
    case OptionsModel::blockreconstructionextratxn: return "blockreconstructionextratxn";
    case OptionsModel::blockreconstructionextratxnsize: return "blockreconstructionextratxnsize";
    default: throw std::logic_error(strprintf("GUI option %i has no corresponding node setting.", option));
    }
}

/** Call node.updateRwSetting() with Bitcoin 22.x workaround. */
static void UpdateRwSetting(interfaces::Node& node, OptionsModel::OptionID option, const std::string& suffix, const common::SettingsValue& value)
{
    if (value.isNum() &&
        (option == OptionsModel::DatabaseCache ||
         option == OptionsModel::ThreadsScriptVerif ||
         option == OptionsModel::PruneTristate ||
         option == OptionsModel::PruneSizeMiB)) {
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
static common::SettingsValue PruneSettingFromMiB(Qt::CheckState prune_enabled, int prune_size_mib)
{
    assert(prune_enabled != Qt::Checked || prune_size_mib >= 1); // PruneSizeMiB and ParsePruneSizeMiB never return less
    switch (prune_enabled) {
    case Qt::Unchecked:
        return 0;
    case Qt::PartiallyChecked:
        return 1;
    default:
        return prune_size_mib;
    }
}

//! Get pruning enabled value to show in GUI from bitcoin -prune setting.
static bool PruneEnabled(const common::SettingsValue& prune_setting)
{
    // -prune=1 setting is manual pruning mode, so disabled for purposes of the gui
    return SettingToInt(prune_setting, 0) > 1;
}

//! Get pruning enabled value to show in GUI from bitcoin -prune setting.
static Qt::CheckState PruneSettingAsTristate(const common::SettingsValue& prune_setting)
{
    switch (SettingToInt(prune_setting, 0)) {
    case 0:
        return Qt::Unchecked;
    case 1:
        return Qt::PartiallyChecked;
    default:
        return Qt::Checked;
    }
}

//! Get pruning size value to show in GUI from bitcoin -prune setting. If
//! pruning is not enabled, just show default recommended pruning size (2GB).
static int PruneSizeAsMiB(const common::SettingsValue& prune_setting)
{
    int value = SettingToInt(prune_setting, 0);
    return value > 1 ? value : DEFAULT_PRUNE_TARGET_MiB;
}

struct ProxySetting {
    bool is_set;
    QString ip;
    QString port;
};
static ProxySetting ParseProxyString(const std::string& proxy);
static std::string ProxyString(bool is_set, QString ip, QString port);

static const QLatin1String fontchoice_str_embedded{"embedded"};
static const QLatin1String fontchoice_str_best_system{"best_system"};
static const QString fontchoice_str_custom_prefix{QStringLiteral("custom, ")};

static const std::map<OutputType, std::pair<const char*, const char*>> UntranslatedOutputTypeDescriptions{
    {OutputType::LEGACY, {
        QT_TRANSLATE_NOOP("Output type name", "Base58 (Legacy)"),
        QT_TRANSLATE_NOOP("Output type description", "Widest compatibility and best for health of the Bitcoin network, but may result in higher fees later. Recommended."),
    }},
    {OutputType::P2SH_SEGWIT, {
        QT_TRANSLATE_NOOP("Output type name", "Base58 (P2SH Segwit)"),
        QT_TRANSLATE_NOOP("Output type description", "Compatible with most older wallets, and may result in lower fees than Legacy."),
    }},
    {OutputType::BECH32, {
        QT_TRANSLATE_NOOP("Output type name", "Native Segwit (Bech32)"),
        QT_TRANSLATE_NOOP("Output type description", "Lower fees than Base58, but some old wallets don't support it."),
    }},
    {OutputType::BECH32M, {
        QT_TRANSLATE_NOOP("Output type name", "Taproot (Bech32m)"),
        QT_TRANSLATE_NOOP("Output type description", "Lowest fees, but wallet support is still limited."),
    }},
};

std::pair<QString, QString> GetOutputTypeDescription(const OutputType type)
{
    auto& untr = UntranslatedOutputTypeDescriptions.at(type);
    QString text = QCoreApplication::translate("Output type name", untr.first);
    QString tooltip = QCoreApplication::translate("Output type description", untr.second);
    return std::make_pair(text, tooltip);
}

QString OptionsModel::FontChoiceToString(const OptionsModel::FontChoice& f)
{
    if (std::holds_alternative<FontChoiceAbstract>(f)) {
        if (f == UseBestSystemFont) {
            return fontchoice_str_best_system;
        } else {
            return fontchoice_str_embedded;
        }
    }
    return fontchoice_str_custom_prefix + std::get<QFont>(f).toString();
}

OptionsModel::FontChoice OptionsModel::FontChoiceFromString(const QString& s)
{
    if (s == fontchoice_str_best_system) {
        return FontChoiceAbstract::BestSystemFont;
    } else if (s == fontchoice_str_embedded) {
        return FontChoiceAbstract::EmbeddedFont;
    } else if (s.startsWith(fontchoice_str_custom_prefix)) {
        QFont f;
        f.fromString(s.mid(fontchoice_str_custom_prefix.size()));
        return f;
    } else {
        return FontChoiceAbstract::EmbeddedFont;  // default
    }
}

static QString CanonicalMempoolReplacement(const OptionsModel& model)
{
    switch (model.node().mempool().m_opts.rbf_policy) {
    case RBFPolicy::Never:  return "never";
    case RBFPolicy::OptIn:  return "fee,optin";
    case RBFPolicy::Always: return "fee,-optin";
    }
    assert(0);
}

static QString CanonicalMempoolTRUC(const OptionsModel& model)
{
    switch (model.node().mempool().m_opts.truc_policy) {
    case TRUCPolicy::Reject:  return "reject";
    case TRUCPolicy::Accept:  return "accept";
    case TRUCPolicy::Enforce: return "enforce";
    }
    assert(0);
}

static QString CanonicalPermitEphemeral(const OptionsModel& model)
{
    const auto& opts = model.node().mempool().m_opts;
    if (!(opts.permitephemeral_anchor || opts.permitephemeral_send)) {
        return "reject";
    }
    return
        QString(opts.permitephemeral_anchor ? "" : "-") + "anchor," +
        (opts.permitephemeral_send ? "" : "-") + "send," +
        (opts.permitephemeral_dust ? "" : "-") + "dust";
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
    if (!settings.contains("DisplayBitcoinUnit")) {
        auto init_unit = BitcoinUnit::BTC;
        if (settings.contains("nDisplayUnit")) {
            // Migrate to new setting
            init_unit = BitcoinUnits::FromSetting(settings.value("nDisplayUnit").toString(), init_unit);
        }
        settings.setValue("DisplayBitcoinUnit", QVariant::fromValue(init_unit));
    }

    constexpr auto unit_set_to_variant = [](BitcoinUnit& out, const QVariant& unit_variant){
        if (unit_variant.isNull()) return false;
        if (!unit_variant.canConvert<BitcoinUnit>()) return false;
        const auto unit = unit_variant.value<BitcoinUnit>();
        if (!BitcoinUnits::availableUnits().contains(unit)) return false;
        out = unit;
        return true;
    };
    if (!unit_set_to_variant(m_display_bitcoin_unit, settings.value("DisplayBitcoinUnitKnots"))) {
        if (!unit_set_to_variant(m_display_bitcoin_unit, settings.value("DisplayBitcoinUnit"))) {
            m_display_bitcoin_unit = BitcoinUnit::BTC;
        }
    }

    if (!settings.contains("bDisplayAddresses"))
        settings.setValue("bDisplayAddresses", false);
    bDisplayAddresses = settings.value("bDisplayAddresses", false).toBool();

    if (!settings.contains("strThirdPartyTxUrls"))
        settings.setValue("strThirdPartyTxUrls", "");
    strThirdPartyTxUrls = settings.value("strThirdPartyTxUrls", "").toString();

    if (!settings.contains("fCoinControlFeatures"))
        settings.setValue("fCoinControlFeatures", false);
    fCoinControlFeatures = settings.value("fCoinControlFeatures", false).toBool();

    if (!settings.contains("enable_psbt_controls")) {
        settings.setValue("enable_psbt_controls", false);
    }
    m_enable_psbt_controls = settings.value("enable_psbt_controls", false).toBool();

    // These are shared with the core or have a command-line parameter
    // and we want command-line parameters to overwrite the GUI settings.
    std::unordered_set<std::string> checked_settings;
    for (OptionID option : {DatabaseCache, ThreadsScriptVerif, SpendZeroConfChange, ExternalSignerPath, MapPortUPnP,
                            MapPortNatpmp, Listen, Server, PruneTristate, ProxyUse, ProxyUseTor, Language}) {
        // isSettingIgnored will have a false positive here during first-run prune changes
        if (option == PruneTristate && m_prune_forced_by_gui) continue;

        std::string setting = SettingName(option);
        checked_settings.insert(setting);
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

    if (m_prune_forced_by_gui) checked_settings.insert("prune");
    for (OptionID option = OptionID(0); option < OptionIDRowCount; option = OptionID(option + 1)) {
        std::string setting;
        try {
            setting = SettingName(option);
        } catch (const std::logic_error&) {
            continue;  // Ignore GUI-only settings
        }
        if (!checked_settings.insert(setting).second) continue;
        if (node().isSettingIgnored(setting)) addOverriddenOption("-" + setting);
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

    // Network
    if (!settings.contains("nNetworkPort"))
        settings.setValue("nNetworkPort", (quint16)Params().GetDefaultPort());
    if (!gArgs.SoftSetArg("-port", settings.value("nNetworkPort").toString().toStdString()))
        addOverriddenOption("-port");

    // rwconf settings that require a restart
    // Caution: This is before general initialisation occurs!
    f_peerbloomfilters = gArgs.GetBoolArg("-peerbloomfilters", DEFAULT_PEERBLOOMFILTERS);
    f_rejectspkreuse = !(gArgs.GetArg("-spkreuse", DEFAULT_SPKREUSE) == "allow" || gArgs.GetBoolArg("-spkreuse", false));

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
    m_font_money_supports_tonal = TonalUtils::font_supports_tonal(getFontForMoney(BitcoinUnit::BTC));
    Q_EMIT fontForMoneyChanged(getFontForMoney(BitcoinUnit::BTC));

    if (settings.contains("FontForQRCodes")) {
        m_font_qrcodes = FontChoiceFromString(settings.value("FontForQRCodes").toString());
    }
    Q_EMIT fontForQRCodesChanged(getFontChoiceForQRCodes());

    if (!settings.contains("PeersTabAlternatingRowColors")) {
        settings.setValue("PeersTabAlternatingRowColors", "false");
    }
    m_peers_tab_alternating_row_colors = settings.value("PeersTabAlternatingRowColors").toBool();
    Q_EMIT peersTabAlternatingRowColorsChanged(m_peers_tab_alternating_row_colors);

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
    // Backup and reset settings.json
    node().resetSettings();

    QSettings settings;

    // Backup old settings to chain-specific datadir for troubleshooting
    BackupSettings(gArgs.GetDataDirNet() / "guisettings.ini.bak", settings);

    // Save the strDataDir setting
    QString dataDir = GUIUtil::getDefaultDataDirectory();
    dataDir = settings.value("strDataDir", dataDir).toString();

    // Remove rw config file
    gArgs.EraseRWConfigFile();

    // Remove all entries from our QSettings object
    settings.clear();

    // Set strDataDir
    settings.setValue("strDataDir", dataDir);

    // Set prune option iff it was configured in rwconf
    if (gArgs.RWConfigHasPruneOption()) {
        SetPruneTargetMiB(gArgs.GetIntArg("-prune", 0));
    }

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
    uint16_t port{0};
    std::string hostname;
    if (SplitHostPort(proxy.toStdString(), port, hostname) && port != 0) {
        // Valid and port within the valid range
        // Check if the hostname contains a colon, indicating an IPv6 address
        if (hostname.find(':') != std::string::npos) {
            hostname = "[" + hostname + "]"; // Wrap IPv6 address in brackets
        }
        return {true, QString::fromStdString(hostname), QString::number(port)};
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

void OptionsModel::SetPruneTargetMiB(int prune_target_mib)
{
    const common::SettingsValue cur_value = node().getPersistentSetting("prune");
    const common::SettingsValue new_value = prune_target_mib;

    // Force setting to take effect. It is still safe to change the value at
    // this point because this function is only called after the intro screen is
    // shown, before the node starts.
    node().forceSetting("prune", new_value);
    m_prune_forced_by_gui = true;

    // Update settings.json if value configured in intro screen is different
    // from saved value. Avoid writing settings.json if bitcoin.conf value
    // doesn't need to be overridden.
    if (cur_value.write() != new_value.write()) {
        // Call UpdateRwSetting() instead of setOption() to avoid setting
        // RestartRequired flag
        UpdateRwSetting(node(), PruneTristate, "", new_value);
        gArgs.ModifyRWConfigFile("prune", new_value.getValStr());
    }

    // Keep previous pruning size, if pruning was disabled.
    if (PruneEnabled(cur_value)) {
        UpdateRwSetting(node(), PruneTristate, "-prev", PruneEnabled(new_value) ? common::SettingsValue{} : cur_value);
    }
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

// NOLINTNEXTLINE(misc-no-recursion)
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
    case NetworkPort:
        return settings.value("nNetworkPort");
    case MapPortUPnP:
#ifdef USE_UPNP
        return SettingToBool(setting(), DEFAULT_UPNP);
#else
        return false;
#endif // USE_UPNP
    case MapPortNatpmp:
        return SettingToBool(setting(), DEFAULT_NATPMP);
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
    case addresstype:
    {
        const OutputType default_address_type = ParseOutputType(gArgs.GetArg("-addresstype", "")).value_or(wallet::DEFAULT_ADDRESS_TYPE);
        return QString::fromStdString(FormatOutputType(default_address_type));
    }
#endif
    case DisplayUnit:
        return QVariant::fromValue(m_display_bitcoin_unit);
    case DisplayAddresses:
        return bDisplayAddresses;
    case ThirdPartyTxUrls:
        return strThirdPartyTxUrls;
    case Language:
        return QString::fromStdString(SettingToString(setting(), ""));
    case FontForMoney:
        return QVariant::fromValue(m_font_money);
    case FontForQRCodes:
        return QVariant::fromValue(m_font_qrcodes);
    case PeersTabAlternatingRowColors:
        return m_peers_tab_alternating_row_colors;
#ifdef ENABLE_WALLET
    case walletrbf:
        return gArgs.GetBoolArg("-walletrbf", wallet::DEFAULT_WALLET_RBF);
#endif
    case CoinControlFeatures:
        return fCoinControlFeatures;
    case EnablePSBTControls:
        return settings.value("enable_psbt_controls");
    case PruneTristate:
        return PruneSettingAsTristate(setting());
    case PruneSizeMiB:
        return PruneEnabled(setting()) ? PruneSizeAsMiB(setting()) :
               suffix.empty()          ? getOption(option, "-prev") :
                                         DEFAULT_PRUNE_TARGET_MiB;
    case DatabaseCache:
        return qlonglong(SettingToInt(setting(), DEFAULT_DB_CACHE >> 20));
    case ThreadsScriptVerif:
        return qlonglong(SettingToInt(setting(), DEFAULT_SCRIPTCHECK_THREADS));
    case Listen:
        return SettingToBool(setting(), DEFAULT_LISTEN);
    case Server:
        return SettingToBool(setting(), false);
    case MaskValues:
        return m_mask_values;
    case maxuploadtarget:
        return qlonglong(node().context()->connman->GetMaxOutboundTarget() / 1024 / 1024);
    case peerbloomfilters:
        return f_peerbloomfilters;
    case peerblockfilters:
        return gArgs.GetBoolArg("-peerblockfilters", DEFAULT_PEERBLOCKFILTERS);
    case mempoolreplacement:
        return CanonicalMempoolReplacement(*this);
    case mempooltruc:
        return CanonicalMempoolTRUC(*this);
    case maxorphantx:
        return qlonglong(gArgs.GetIntArg("-maxorphantx", DEFAULT_MAX_ORPHAN_TRANSACTIONS));
    case maxmempool:
        return qlonglong(node().mempool().m_opts.max_size_bytes / 1'000'000);
    case incrementalrelayfee:
        return qlonglong(node().mempool().m_opts.incremental_relay_feerate.GetFeePerK());
    case mempoolexpiry:
        return qlonglong(std::chrono::duration_cast<std::chrono::hours>(node().mempool().m_opts.expiry).count());
    case rejectunknownscripts:
        return node().mempool().m_opts.require_standard;
    case rejectunknownwitness:
        return !node().mempool().m_opts.acceptunknownwitness;
    case rejectparasites:
        return node().mempool().m_opts.reject_parasites;
    case rejecttokens:
        return node().mempool().m_opts.reject_tokens;
    case rejectspkreuse:
        return f_rejectspkreuse;
    case minrelaytxfee:
        return qlonglong(node().mempool().m_opts.min_relay_feerate.GetFeePerK());
    case minrelaycoinblocks:
        return qlonglong(node().mempool().m_opts.minrelaycoinblocks);
    case minrelaymaturity:
        return node().mempool().m_opts.minrelaymaturity;
    case bytespersigop:
        return nBytesPerSigOp;
    case bytespersigopstrict:
        return nBytesPerSigOpStrict;
    case limitancestorcount:
        return qlonglong(node().mempool().m_opts.limits.ancestor_count);
    case limitancestorsize:
        return qlonglong(node().mempool().m_opts.limits.ancestor_size_vbytes / 1'000);
    case limitdescendantcount:
        return qlonglong(node().mempool().m_opts.limits.descendant_count);
    case limitdescendantsize:
        return qlonglong(node().mempool().m_opts.limits.descendant_size_vbytes / 1'000);
    case rejectbarepubkey:
        return !node().mempool().m_opts.permit_bare_pubkey;
    case rejectbaremultisig:
        return !node().mempool().m_opts.permit_bare_multisig;
    case permitephemeral:
        return CanonicalPermitEphemeral(*this);
    case rejectbareanchor:
        return !node().mempool().m_opts.permitbareanchor;
    case rejectbaredatacarrier:
        return !node().mempool().m_opts.permitbaredatacarrier;
    case maxscriptsize:
        return ::g_script_size_policy_limit;
    case maxtxlegacysigops:
        return node().mempool().m_opts.maxtxlegacysigops;
    case datacarriercost:
        return double(::g_weight_per_data_byte) / WITNESS_SCALE_FACTOR;
    case datacarriersize:
        return qlonglong(node().mempool().m_opts.max_datacarrier_bytes.value_or(0));
    case rejectnonstddatacarrier:
        return !node().mempool().m_opts.accept_non_std_datacarrier;
    case dustrelayfee:
        return qlonglong(node().mempool().m_opts.dust_relay_feerate_floor.GetFeePerK());
    case dustdynamic:
        return QString::fromStdString(SettingToString(setting(), DEFAULT_DUST_DYNAMIC));
    case blockmintxfee:
        if (gArgs.IsArgSet("-blockmintxfee")) {
            return qlonglong(ParseMoney(gArgs.GetArg("-blockmintxfee", "")).value_or(0));
        } else {
            return qlonglong(DEFAULT_BLOCK_MIN_TX_FEE);
        }
    case blockmaxsize:
        return qlonglong(gArgs.GetIntArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE) / 1000);
    case blockprioritysize:
        return qlonglong(gArgs.GetIntArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE) / 1000);
    case blockmaxweight:
        return qlonglong(gArgs.GetIntArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT) / 1000);
    case blockreconstructionextratxn:
        return qlonglong(gArgs.GetIntArg("-blockreconstructionextratxn", DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN));
    case blockreconstructionextratxnsize:
        return qlonglong(gArgs.GetIntArg("-blockreconstructionextratxnsize", DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN_SIZE / 1'000'000));
    default:
        return QVariant();
    }
}

QFont OptionsModel::getFontForChoice(const FontChoice& fc)
{
    QFont f;
    if (std::holds_alternative<FontChoiceAbstract>(fc)) {
        f = GUIUtil::fixedPitchFont(fc != UseBestSystemFont);
        if (fc == UseBestSystemFont) f.setWeight(QFont::Bold);
    } else {
        f = std::get<QFont>(fc);
    }
    return f;
}

QFont OptionsModel::getFontForMoney(const BitcoinUnit unit) const
{
    if (BitcoinUnits::numsys(unit) == BitcoinUnits::Unit::TBC && !m_font_money_supports_tonal) {
        return getFontForChoice(FontChoiceAbstract::EmbeddedFont);
    }
    return getFontForChoice(m_font_money);
}

// NOLINTNEXTLINE(misc-no-recursion)
bool OptionsModel::setOption(OptionID option, const QVariant& value, const std::string& suffix)
{
    auto changed = [&] { return value.isValid() && value != getOption(option, suffix); };
    auto update = [&](const common::SettingsValue& value) { return UpdateRwSetting(node(), option, suffix, value); };

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
    case NetworkPort:
        if (settings.value("nNetworkPort") != value) {
            // If the port input box is empty, set to default port
            if (value.toString().isEmpty()) {
                settings.setValue("nNetworkPort", (quint16)Params().GetDefaultPort());
            }
            else {
                settings.setValue("nNetworkPort", (quint16)value.toInt());
            }
            setRestartRequired(true);
        }
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
    case SubFeeFromAmount:
        m_sub_fee_from_amount = value.toBool();
        settings.setValue("SubFeeFromAmount", m_sub_fee_from_amount);
        break;
    case addresstype:
    {
        const std::string newvalue_str = value.toString().toStdString();
        const OutputType oldvalue = ParseOutputType(gArgs.GetArg("-addresstype", "")).value_or(wallet::DEFAULT_ADDRESS_TYPE);
        const OutputType newvalue = ParseOutputType(newvalue_str).value_or(oldvalue);
        if (newvalue != oldvalue) {
            gArgs.ModifyRWConfigFile("addresstype", newvalue_str);
            gArgs.ForceSetArg("-addresstype", newvalue_str);
            for (auto& wallet_interface : m_node.walletLoader().getWallets()) {
                wallet::CWallet *wallet;
                if (wallet_interface && (wallet = wallet_interface->wallet())) {
                    wallet->m_default_address_type = newvalue;
                } else {
                    setRestartRequired(true);
                    continue;
                }
            }
            Q_EMIT addresstypeChanged(newvalue);
        }
        break;
    }
#endif
    case DisplayUnit:
        setDisplayUnit(value);
        break;
    case DisplayAddresses:
        bDisplayAddresses = value.toBool();
        settings.setValue("bDisplayAddresses", bDisplayAddresses);
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
            update(value.toString().toStdString());
            setRestartRequired(true);
        }
        break;
    case FontForMoney:
    {
        const auto& new_font = value.value<FontChoice>();
        if (m_font_money == new_font) break;
        settings.setValue("FontForMoney", FontChoiceToString(new_font));
        m_font_money = new_font;
        m_font_money_supports_tonal = TonalUtils::font_supports_tonal(getFontForMoney(BitcoinUnit::BTC));
        Q_EMIT fontForMoneyChanged(getFontForMoney(BitcoinUnit::BTC));
        break;
    }
    case FontForQRCodes:
    {
        const auto& new_font = value.value<FontChoice>();
        if (m_font_qrcodes == new_font) break;
        settings.setValue("FontForQRCodes", FontChoiceToString(new_font));
        m_font_qrcodes = new_font;
        Q_EMIT fontForQRCodesChanged(new_font);
        break;
    }
    case PeersTabAlternatingRowColors:
        m_peers_tab_alternating_row_colors = value.toBool();
        settings.setValue("PeersTabAlternatingRowColors", m_peers_tab_alternating_row_colors);
        Q_EMIT peersTabAlternatingRowColorsChanged(m_peers_tab_alternating_row_colors);
        break;
#ifdef ENABLE_WALLET
    case walletrbf:
        if (changed()) {
            const bool fNewValue = value.toBool();
            const std::string newvalue_str = strprintf("%d", fNewValue);
            gArgs.ModifyRWConfigFile("walletrbf", newvalue_str);
            gArgs.ForceSetArg("-walletrbf", newvalue_str);
            for (auto& wallet_interface : m_node.walletLoader().getWallets()) {
                wallet::CWallet *wallet;
                if (wallet_interface && (wallet = wallet_interface->wallet())) {
                    wallet->m_signal_rbf = fNewValue;
                } else {
                    setRestartRequired(true);
                }
            }
        }
        break;
#endif
    case CoinControlFeatures:
        fCoinControlFeatures = value.toBool();
        settings.setValue("fCoinControlFeatures", fCoinControlFeatures);
        Q_EMIT coinControlFeaturesChanged(fCoinControlFeatures);
        break;
    case EnablePSBTControls:
        m_enable_psbt_controls = value.toBool();
        settings.setValue("enable_psbt_controls", m_enable_psbt_controls);
        break;
    case PruneTristate:
        if (changed()) {
            const bool is_autoprune = (value.value<Qt::CheckState>() == Qt::Checked);
            const auto prune_setting = PruneSettingFromMiB(value.value<Qt::CheckState>(), getOption(PruneSizeMiB).toInt());
            if (suffix.empty() && !is_autoprune) setOption(option, Qt::Checked, "-prev");
            update(prune_setting);
            if (suffix.empty()) gArgs.ModifyRWConfigFile("prune", prune_setting.getValStr());
            if (suffix.empty() && is_autoprune) UpdateRwSetting(node(), option, "-prev", {});
            if (suffix.empty()) setRestartRequired(true);
        }
        break;
    case PruneSizeMiB:
        if (changed()) {
            const bool is_autoprune = (Qt::Checked == getOption(PruneTristate).value<Qt::CheckState>());
            if (suffix.empty() && !is_autoprune) {
                setOption(option, value, "-prev");
            } else {
                const auto prune_setting = PruneSettingFromMiB(Qt::Checked, value.toInt());
                update(prune_setting);
                if (suffix.empty()) gArgs.ModifyRWConfigFile("prune", prune_setting.getValStr());
            }
            if (suffix.empty() && is_autoprune) setRestartRequired(true);
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
    case maxuploadtarget:
    {
        if (changed()) {
            gArgs.ModifyRWConfigFile("maxuploadtarget", value.toString().toStdString());
            node().context()->connman->SetMaxOutboundTarget(value.toLongLong() * 1024 * 1024);
        }
        break;
    }
    case peerbloomfilters:
        if (changed()) {
            gArgs.ModifyRWConfigFile("peerbloomfilters", strprintf("%d", value.toBool()));
            f_peerbloomfilters = value.toBool();
            setRestartRequired(true);
        }
        break;
    case peerblockfilters:
    {
        bool nv = value.toBool();
        if (gArgs.GetBoolArg("-peerblockfilters", DEFAULT_PEERBLOCKFILTERS) != nv) {
            gArgs.ModifyRWConfigFile("peerblockfilters", strprintf("%d", nv));
            gArgs.ModifyRWConfigFile("peercfilters", strprintf("%d", nv), /*also_settings_json=*/ false);  // for downgrade compatibility with Knots 0.19
            gArgs.ForceSetArg("peerblockfilters", nv);
            if (nv && !GetBlockFilterIndex(BlockFilterType::BASIC)) {
                // TODO: When other options are possible, we need to append a list!
                // TODO: Some way to unset/delete this...
                gArgs.ModifyRWConfigFile("blockfilterindex", "basic");
                gArgs.ForceSetArg("blockfilterindex", "basic");
            }
            setRestartRequired(true);
        }
        break;
    }
    case mempoolreplacement:
    {
        if (changed()) {
            QString nv = value.toString();
            if (nv == "never") {
                node().mempool().m_opts.rbf_policy = RBFPolicy::Never;
                node().updateRwSetting("mempoolfullrbf", "0");
            } else if (nv == "fee,optin") {
                node().mempool().m_opts.rbf_policy = RBFPolicy::OptIn;
                node().updateRwSetting("mempoolfullrbf", "0");
            } else {  // "fee,-optin"
                node().mempool().m_opts.rbf_policy = RBFPolicy::Always;
                node().updateRwSetting("mempoolfullrbf", "1");
            }
            gArgs.ModifyRWConfigFile("mempoolreplacement", nv.toStdString());
        }
        break;
    }
    case mempooltruc:
    {
        if (changed()) {
            QString nv = value.toString();
            if (nv == "reject") {
                node().mempool().m_opts.truc_policy = TRUCPolicy::Reject;
            } else if (nv == "accept") {
                node().mempool().m_opts.truc_policy = TRUCPolicy::Accept;
            } else if (nv == "enforce") {
                node().mempool().m_opts.truc_policy = TRUCPolicy::Enforce;
            }
            node().updateRwSetting("mempooltruc", nv.toStdString());
        }
        break;
    }
    case maxorphantx:
    {
        if (changed()) {
            unsigned int nMaxOrphanTx = gArgs.GetIntArg("-maxorphantx", DEFAULT_MAX_ORPHAN_TRANSACTIONS);
            unsigned int nNv = value.toLongLong();
            std::string strNv = value.toString().toStdString();
            gArgs.ForceSetArg("-maxorphantx", strNv);
            gArgs.ModifyRWConfigFile("maxorphantx", strNv);
            if (nNv < nMaxOrphanTx) {
                assert(node().context() && node().context()->peerman);
                node().context()->peerman->LimitOrphanTxSize(nNv);
            }
        }
        break;
    }
    case maxmempool:
    {
        if (changed()) {
            long long nOldValue = node().mempool().m_opts.max_size_bytes;
            long long nNv = value.toLongLong();
            std::string strNv = value.toString().toStdString();
            node().mempool().m_opts.max_size_bytes = nNv * 1'000'000;
            gArgs.ForceSetArg("-maxmempool", strNv);
            gArgs.ModifyRWConfigFile("maxmempool", strNv);
            if (nNv < nOldValue) {
                LOCK(cs_main);
                auto node_ctx = node().context();
                assert(node_ctx && node_ctx->mempool && node_ctx->chainman);
                auto& active_chainstate = node_ctx->chainman->ActiveChainstate();
                LOCK(node_ctx->mempool->cs);
                LimitMempoolSize(*node_ctx->mempool, active_chainstate.CoinsTip());
            }
        }
        break;
    }
    case incrementalrelayfee:
        if (changed()) {
            CAmount nNv = value.toLongLong();
            gArgs.ModifyRWConfigFile("incrementalrelayfee", FormatMoney(nNv));
            node().mempool().m_opts.incremental_relay_feerate = CFeeRate(nNv);
        }
        break;
    case mempoolexpiry:
    {
        if (changed()) {
            const auto old_value = node().mempool().m_opts.expiry;
            const std::chrono::hours new_value{value.toLongLong()};
            std::string strNv = value.toString().toStdString();
            node().mempool().m_opts.expiry = new_value;
            gArgs.ForceSetArg("-mempoolexpiry", strNv);
            gArgs.ModifyRWConfigFile("mempoolexpiry", strNv);
            if (new_value < old_value) {
                LOCK(cs_main);
                auto node_ctx = node().context();
                assert(node_ctx && node_ctx->mempool && node_ctx->chainman);
                auto& active_chainstate = node_ctx->chainman->ActiveChainstate();
                LOCK(node_ctx->mempool->cs);
                LimitMempoolSize(*node_ctx->mempool, active_chainstate.CoinsTip());
            }
        }
        break;
    }
    case rejectunknownscripts:
    {
        if (changed()) {
            const bool fNewValue = value.toBool();
            node().mempool().m_opts.require_standard = fNewValue;
            // This option is inverted in the config:
            gArgs.ModifyRWConfigFile("acceptnonstdtxn", strprintf("%d", ! fNewValue));
        }
        break;
    }
    case rejectunknownwitness:
        if (changed()) {
            // This option is inverted
            const bool new_value = ! value.toBool();
            node().updateRwSetting("acceptunknownwitness" + suffix, new_value);
            node().mempool().m_opts.acceptunknownwitness = new_value;
        }
        break;
    case rejectparasites:
    {
        if (changed()) {
            const bool nv = value.toBool();
            node().mempool().m_opts.reject_parasites = nv;
            node().updateRwSetting("rejectparasites", nv);
        }
        break;
    }
    case rejecttokens:
    {
        if (changed()) {
            const bool nv = value.toBool();
            node().mempool().m_opts.reject_tokens = nv;
            node().updateRwSetting("rejecttokens", nv);
        }
        break;
    }
    case rejectspkreuse:
        if (changed()) {
            const bool fNewValue = value.toBool();
            gArgs.ModifyRWConfigFile("spkreuse", fNewValue ? "conflict" : "allow");
            f_rejectspkreuse = fNewValue;
            setRestartRequired(true);
        }
        break;
    case minrelaytxfee:
        if (changed()) {
            CAmount nNv = value.toLongLong();
            gArgs.ModifyRWConfigFile("minrelaytxfee", FormatMoney(nNv));
            node().mempool().m_opts.min_relay_feerate = CFeeRate(nNv);
        }
        break;
    case minrelaycoinblocks:
        if (changed()) {
            uint64_t nNv = value.toLongLong();
            update(nNv);
            node().mempool().m_opts.minrelaycoinblocks = nNv;
        }
        break;
    case minrelaymaturity:
        if (changed()) {
            int nNv = value.toInt();
            update(nNv);
            node().mempool().m_opts.minrelaymaturity = nNv;
        }
        break;
    case bytespersigop:
        if (changed()) {
            gArgs.ModifyRWConfigFile("bytespersigop", value.toString().toStdString());
            nBytesPerSigOp = value.toLongLong();
        }
        break;
    case bytespersigopstrict:
        if (changed()) {
            gArgs.ModifyRWConfigFile("bytespersigopstrict", value.toString().toStdString());
            nBytesPerSigOpStrict = value.toLongLong();
        }
        break;
    case limitancestorcount:
        if (changed()) {
            long long nNv = value.toLongLong();
            std::string strNv = value.toString().toStdString();
            node().mempool().m_opts.limits.ancestor_count = nNv;
            gArgs.ForceSetArg("-limitancestorcount", strNv);
            gArgs.ModifyRWConfigFile("limitancestorcount", strNv);
        }
        break;
    case limitancestorsize:
        if (changed()) {
            long long nNv = value.toLongLong();
            std::string strNv = value.toString().toStdString();
            node().mempool().m_opts.limits.ancestor_size_vbytes = nNv * 1'000;
            gArgs.ForceSetArg("-limitancestorsize", strNv);
            gArgs.ModifyRWConfigFile("limitancestorsize", strNv);
        }
        break;
    case limitdescendantcount:
        if (changed()) {
            long long nNv = value.toLongLong();
            std::string strNv = value.toString().toStdString();
            node().mempool().m_opts.limits.descendant_count = nNv;
            gArgs.ForceSetArg("-limitdescendantcount", strNv);
            gArgs.ModifyRWConfigFile("limitdescendantcount", strNv);
        }
        break;
    case limitdescendantsize:
        if (changed()) {
            long long nNv = value.toLongLong();
            std::string strNv = value.toString().toStdString();
            node().mempool().m_opts.limits.descendant_size_vbytes = nNv * 1'000;
            gArgs.ForceSetArg("-limitdescendantsize", strNv);
            gArgs.ModifyRWConfigFile("limitdescendantsize", strNv);
        }
        break;
    case rejectbarepubkey:
        if (changed()) {
            // The config and internal option is inverted
            const bool nv = ! value.toBool();
            node().mempool().m_opts.permit_bare_pubkey = nv;
            node().updateRwSetting("permitbarepubkey", nv);
        }
        break;
    case rejectbaremultisig:
        if (changed()) {
            // The config and internal option is inverted
            const bool fNewValue = ! value.toBool();
            node().mempool().m_opts.permit_bare_multisig = fNewValue;
            gArgs.ModifyRWConfigFile("permitbaremultisig", strprintf("%d", fNewValue));
        }
        break;
    case permitephemeral:
    {
        if (changed()) {
            std::string nv = value.toString().toStdString();
            ApplyPermitEphemeralOption(nv, node().mempool().m_opts);
            update(nv);
        }
        break;
    }
    case rejectbareanchor:
        if (changed()) {
            // The config and internal option is inverted
            const bool nv = ! value.toBool();
            node().mempool().m_opts.permitbareanchor = nv;
            node().updateRwSetting("permitbareanchor", nv);
        }
        break;
    case rejectbaredatacarrier:
        if (changed()) {
            // The config and internal option is inverted
            const bool nv = ! value.toBool();
            node().mempool().m_opts.permitbaredatacarrier = nv;
            node().updateRwSetting("permitbaredatacarrier", nv);
        }
        break;
    case maxscriptsize:
        if (changed()) {
            const auto nv = value.toLongLong();
            update(nv);
            ::g_script_size_policy_limit = nv;
        }
        break;
    case maxtxlegacysigops:
        if (changed()) {
            const auto nv = value.toLongLong();
            update(nv);
            node().mempool().m_opts.maxtxlegacysigops = nv;
        }
        break;
    case datacarriercost:
        if (changed()) {
            const double nNewSize = value.toDouble();
            update(nNewSize);
            ::g_weight_per_data_byte = nNewSize * WITNESS_SCALE_FACTOR;
        }
        break;
    case datacarriersize:
        if (changed()) {
            const int nNewSize = value.toInt();
            const bool fNewEn = (nNewSize > 0);
            if (fNewEn) {
                if (!node().mempool().m_opts.max_datacarrier_bytes.has_value()) {
                    gArgs.ModifyRWConfigFile("datacarrier", strprintf("%d", fNewEn));
                }
                gArgs.ModifyRWConfigFile("datacarriersize", value.toString().toStdString());
                node().mempool().m_opts.max_datacarrier_bytes = nNewSize;
            } else {
                gArgs.ModifyRWConfigFile("datacarrier", "0");
                node().mempool().m_opts.max_datacarrier_bytes = std::nullopt;
            }
        }
        break;
    case rejectnonstddatacarrier:
        if (changed()) {
            // This option is inverted
            const bool new_value = ! value.toBool();
            node().updateRwSetting("acceptnonstddatacarrier" + suffix, new_value);
            node().mempool().m_opts.accept_non_std_datacarrier = new_value;
        }
        break;
    case dustrelayfee:
        if (changed()) {
            CAmount nNv = value.toLongLong();
            gArgs.ModifyRWConfigFile("dustrelayfee", FormatMoney(nNv));
            CFeeRate feerate{nNv};
            node().mempool().m_opts.dust_relay_feerate_floor = feerate;
            if (node().mempool().m_opts.dust_relay_feerate < feerate || !node().mempool().m_opts.dust_relay_target) {
                node().mempool().m_opts.dust_relay_feerate = feerate;
            } else {
                node().mempool().UpdateDynamicDustFeerate();
            }
        }
        break;
    case dustdynamic:
        if (changed()) {
            const std::string newvalue_str = value.toString().toStdString();
            const util::Result<std::pair<int32_t, int>> parsed = ParseDustDynamicOpt(newvalue_str, 1008 /* FIXME: get from estimator */);
            assert(parsed);  // FIXME: what to do if it fails to parse?
            // FIXME: save -prev-<type> for each type
            update(newvalue_str);
            node().mempool().m_opts.dust_relay_target = parsed->first;
            node().mempool().m_opts.dust_relay_multiplier = parsed->second;
        }
        break;
    case blockmintxfee:
        if (changed()) {
            std::string strNv = FormatMoney(value.toLongLong());
            gArgs.ForceSetArg("-blockmintxfee", strNv);
            gArgs.ModifyRWConfigFile("blockmintxfee", strNv);
        }
        break;
    case blockmaxsize:
    case blockprioritysize:
    case blockmaxweight:
        if (changed()) {
            const int nNewValue_kB = value.toInt();
            std::string strNv = strprintf("%d000", nNewValue_kB);
            std::string strKey;
            switch (option) {
                case blockmaxsize:
                    strKey = "blockmaxsize";
                    break;
                case blockprioritysize:
                    strKey = "blockprioritysize";
                    break;
                case blockmaxweight:
                    strKey = "blockmaxweight";
                    break;
                default: assert(0);
            }
            gArgs.ForceSetArg("-" + strKey, strNv);
            gArgs.ModifyRWConfigFile(strKey, strNv);
        }
        break;
    case blockreconstructionextratxn:
        if (changed()) {
            std::string strNv = value.toString().toStdString();
            gArgs.ForceSetArg("-blockreconstructionextratxn", strNv);
            gArgs.ModifyRWConfigFile("blockreconstructionextratxn", strNv);
            setRestartRequired(true);
        }
        break;
    case blockreconstructionextratxnsize:
        if (changed()) {
            update(value.toLongLong());
            gArgs.ForceSetArg("-blockreconstructionextratxnsize", value.toString().toStdString());
            setRestartRequired(true);
        }
        break;
    case corepolicy:
        gArgs.ModifyRWConfigFile("corepolicy", value.toString().toStdString());
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
    if (BitcoinUnits::numsys(m_display_bitcoin_unit) == BitcoinUnit::BTC) {
        settings.setValue("DisplayBitcoinUnit", QVariant::fromValue(m_display_bitcoin_unit));
        settings.remove("DisplayBitcoinUnitKnots");
    } else {
        settings.setValue("DisplayBitcoinUnitKnots", QVariant::fromValue(m_display_bitcoin_unit));
    }
    {
        // For older versions:
        auto setting_val = BitcoinUnits::ToSetting(m_display_bitcoin_unit);
        if (const QString* setting_str = std::get_if<QString>(&setting_val)) {
            settings.setValue("nDisplayUnit", *setting_str);
        } else {
            settings.setValue("nDisplayUnit", std::get<qint8>(setting_val));
        }
    }
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

bool OptionsModel::hasSigner()
{
    return gArgs.GetArg("-signer", "") != "";
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
            settings.setValue("nDatabaseCache", (qint64)(DEFAULT_DB_CACHE >> 20));

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
            } else if (option == PruneSizeMiB) {
                // Stored as GB
                const int64_t prune_size_gb = value.toInt();
                const int prune_size_mib = std::max(prune_size_gb * GB_BYTES / MiB_BYTES, MIN_DISK_SPACE_FOR_BLOCK_FILES / MiB_BYTES);
                setOption(option, prune_size_mib);
            } else if (option == PruneTristate) {
                // Stored as bool
                setOption(option, value.toBool() ? Qt::Checked : Qt::Unchecked);
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
    migrate_setting(PruneSizeMiB, "nPruneSize");
    migrate_setting(PruneTristate, "bPrune");
    migrate_setting(ProxyIP, "addrProxy");
    migrate_setting(ProxyUse, "fUseProxy");
    migrate_setting(ProxyIPTor, "addrSeparateProxyTor");
    migrate_setting(ProxyUseTor, "fUseSeparateProxyTor");
    migrate_setting(Language, "language");

    // In case migrating QSettings caused any settings value to change, rerun
    // parameter interaction code to update other settings. This is particularly
    // important for the -listen setting, which should cause -listenonion, -upnp,
    // and other settings to default to false if it was set to false.
    // (https://github.com/bitcoin-core/gui/issues/567).
    node().initParameterInteraction();
}
