// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_OPTIONSMODEL_H
#define BITCOIN_QT_OPTIONSMODEL_H

#include <qt/bitcoinunits.h>
#include <qt/guiconstants.h>

#include <cstdint>

#include <QAbstractListModel>
#include <QFont>

#include <assert.h>
#include <variant>

struct bilingual_str;
namespace interfaces {
class Node;
}

extern const char *DEFAULT_GUI_PROXY_HOST;
static constexpr uint16_t DEFAULT_GUI_PROXY_PORT = 9050;

/** Default threshold for dust attack protection (in duffs) */
static constexpr qint64 DEFAULT_DUST_PROTECTION_THRESHOLD = 10000;

/**
 * Convert configured prune target MiB to displayed GB. Round up to avoid underestimating max disk usage.
 */
static inline int PruneMiBtoGB(int64_t mib) { return (mib * 1024 * 1024 + GB_BYTES - 1) / GB_BYTES; }

/**
 * Convert displayed prune target GB to configured MiB. Round down so roundtrip GB -> MiB -> GB conversion is stable.
 */
static inline int64_t PruneGBtoMiB(int gb) { return gb * GB_BYTES / 1024 / 1024; }

/** Interface from Qt to configuration data structure for Bitcoin client.
   To Qt, the options are presented as a list with the different options
   laid out vertically.
   This can be changed to a tree once the settings become sufficiently
   complex.
 */
class OptionsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit OptionsModel(interfaces::Node& node, QObject *parent = nullptr);

    enum OptionID {
        StartAtStartup,         // bool
        ShowTrayIcon,           // bool
        MinimizeToTray,         // bool
        MapPortUPnP,            // bool
        MapPortNatpmp,          // bool
        MinimizeOnClose,        // bool
        ProxyUse,               // bool
        ProxyIP,                // QString
        ProxyPort,              // int
        ProxyUseTor,            // bool
        ProxyIPTor,             // QString
        ProxyPortTor,           // int
        DisplayUnit,            // BitcoinUnit
        ThirdPartyTxUrls,       // QString
        Digits,                 // QString
        Theme,                  // QString
        FontFamily,             // int
        FontScale,              // int
        FontWeightNormal,       // int
        FontWeightBold,         // int
        Language,               // QString
        FontForMoney,           // FontChoice
        CoinControlFeatures,    // bool
        SubFeeFromAmount,       // bool
        KeepChangeAddress,      // bool
        ThreadsScriptVerif,     // int
        Prune,                  // bool
        PruneSize,              // int
        DatabaseCache,          // int
        ExternalSignerPath,     // QString
        SpendZeroConfChange,    // bool
        ShowMasternodesTab,     // bool
        ShowGovernanceClock,    // bool
        ShowGovernanceTab,      // bool
        CoinJoinEnabled,        // bool
        ShowAdvancedCJUI,       // bool
        ShowCoinJoinPopups,     // bool
        LowKeysWarning,         // bool
        CoinJoinSessions,       // int
        CoinJoinRounds,         // int
        CoinJoinAmount,         // int
        CoinJoinDenomsGoal,     // int
        CoinJoinDenomsHardCap,  // int
        CoinJoinMultiSession,   // bool
        Listen,                 // bool
        Server,                 // bool
        EnablePSBTControls,     // bool
        MaskValues,             // bool
        DustProtection,         // bool
        DustProtectionThreshold, // CAmount (in duffs)
        OptionIDRowCount,
    };

    enum class FontChoiceAbstract {
        ApplicationFont,
        BestSystemFont,
        EmbeddedFont,
    };
    typedef std::variant<FontChoiceAbstract, QFont> FontChoice;
    static inline const FontChoice UseBestSystemFont{FontChoiceAbstract::BestSystemFont};
    static QFont getFontForChoice(const FontChoice& fc);

    bool Init(bilingual_str& error);
    void Reset();

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    QVariant getOption(OptionID option, const std::string& suffix="") const;
    bool setOption(OptionID option, const QVariant& value, const std::string& suffix="");
    /** Updates current unit in memory, settings and emits displayUnitChanged(new_unit) signal */
    void setDisplayUnit(const QVariant& new_unit);

    /* Explicit getters */
    bool getShowTrayIcon() const { return m_show_tray_icon; }
    bool getMinimizeToTray() const { return fMinimizeToTray; }
    bool getMinimizeOnClose() const { return fMinimizeOnClose; }
    BitcoinUnit getDisplayUnit() const { return m_display_bitcoin_unit; }
    QString getThirdPartyTxUrls() const { return strThirdPartyTxUrls; }
    QFont getFontForMoney() const;
    bool getCoinControlFeatures() const { return fCoinControlFeatures; }
    bool getSubFeeFromAmount() const { return m_sub_fee_from_amount; }
    bool getEnablePSBTControls() const { return m_enable_psbt_controls; }
    bool getKeepChangeAddress() const { return fKeepChangeAddress; }
    bool getShowMasternodesTab() const { return m_enable_masternodes; }
    bool getShowGovernanceClock() const { return m_show_governance_clock; }
    bool getShowGovernanceTab() const { return m_enable_governance; }
    bool getShowAdvancedCJUI() { return fShowAdvancedCJUI; }
    bool getDustProtection() const { return fDustProtection; }
    qint64 getDustProtectionThreshold() const { return nDustProtectionThreshold; }
    const QString& getOverriddenByCommandLine() { return strOverriddenByCommandLine; }
    bool isOptionOverridden(const QString& option) const { return strOverriddenByCommandLine.contains(option); }

    /* Explicit setters */
    void SetPruneTargetGB(int prune_target_gb);

    /* Restart flag helper */
    void setRestartRequired(bool fRequired);
    bool isRestartRequired() const;
    bool resetSettingsOnShutdown{false};

    interfaces::Node& node() const { return m_node; }

private:
    interfaces::Node& m_node;
    /* Qt-only settings */
    bool m_show_tray_icon;
    bool fMinimizeToTray;
    bool fMinimizeOnClose;
    QString language;
    BitcoinUnit m_display_bitcoin_unit;
    QString strThirdPartyTxUrls;
    FontChoice m_font_money{FontChoiceAbstract::ApplicationFont};
    bool fCoinControlFeatures;
    bool m_sub_fee_from_amount;
    bool m_enable_psbt_controls;
    bool m_mask_values;
    bool fKeepChangeAddress;
    bool m_enable_masternodes;
    bool m_enable_governance;
    bool m_show_governance_clock;
    bool fShowAdvancedCJUI;
    bool fDustProtection{false};
    qint64 nDustProtectionThreshold{DEFAULT_DUST_PROTECTION_THRESHOLD};

    /* settings that were overridden by command-line */
    QString strOverriddenByCommandLine;

    static QString FontChoiceToString(const OptionsModel::FontChoice&);
    static FontChoice FontChoiceFromString(const QString&);

    // Add option to list of GUI options overridden through command line/config file
    void addOverriddenOption(const std::string &option);

    // Check settings version and upgrade default values if required
    void checkAndMigrate();

Q_SIGNALS:
    void displayUnitChanged(BitcoinUnit unit);
    void coinJoinRoundsChanged();
    void coinJoinAmountChanged();
    void AdvancedCJUIChanged(bool);
    void coinControlFeaturesChanged(bool);
    void keepChangeAddressChanged(bool);
    void showCoinJoinChanged();
    void showGovernanceClockChanged();
    void showGovernanceChanged();
    void showMasternodesChanged();
    void showTrayIconChanged(bool);
    void fontForMoneyChanged(const QFont&);
    void dustProtectionChanged();
};

Q_DECLARE_METATYPE(OptionsModel::FontChoice)

#endif // BITCOIN_QT_OPTIONSMODEL_H
