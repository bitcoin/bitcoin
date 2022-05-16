// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_OPTIONSMODEL_H
#define BITCOIN_QT_OPTIONSMODEL_H

#include <cstdint>
#include <qt/bitcoinunits.h>
#include <qt/guiconstants.h>

#include <QAbstractListModel>
#include <QFont>
#include <QString>

#include <assert.h>
#include <map>
#include <utility>
#include <variant>

struct bilingual_str;
enum class OutputType;

namespace interfaces {
class Node;
}

extern const char *DEFAULT_GUI_PROXY_HOST;
static constexpr uint16_t DEFAULT_GUI_PROXY_PORT = 9050;

std::pair<QString, QString> GetOutputTypeDescription(const OutputType type);

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
        NetworkPort,            // int
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
        Language,               // QString
        FontForMoney,           // FontChoice
        FontForQRCodes,         // FontChoice
        PeersTabAlternatingRowColors, // bool
        CoinControlFeatures,    // bool
        SubFeeFromAmount,       // bool
        ThreadsScriptVerif,     // int
        PruneTristate,          // Qt::CheckState
        PruneSizeMiB,           // int
        DatabaseCache,          // int
        ExternalSignerPath,     // QString
        SpendZeroConfChange,    // bool
        addresstype,            // QString
        Listen,                 // bool
        Server,                 // bool
        EnablePSBTControls,     // bool
        MaskValues,             // bool
        maxuploadtarget,
        peerbloomfilters,       // bool
        peerblockfilters,       // bool
        OptionIDRowCount,
    };

    enum class FontChoiceAbstract {
        EmbeddedFont,
        BestSystemFont,
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
    FontChoice getFontChoiceForQRCodes() const { return m_font_qrcodes; }
    bool getPeersTabAlternatingRowColors() const { return m_peers_tab_alternating_row_colors; }
    bool getCoinControlFeatures() const { return fCoinControlFeatures; }
    bool getSubFeeFromAmount() const { return m_sub_fee_from_amount; }
    bool getEnablePSBTControls() const { return m_enable_psbt_controls; }
    const QString& getOverriddenByCommandLine() { return strOverriddenByCommandLine; }

    /** Whether -signer was set or not */
    bool hasSigner();

    /* Explicit setters */
    void SetPruneTargetMiB(int prune_target_mib);

    /* Restart flag helper */
    void setRestartRequired(bool fRequired);
    bool isRestartRequired() const;

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
    FontChoice m_font_money{FontChoiceAbstract::EmbeddedFont};
    FontChoice m_font_qrcodes{FontChoiceAbstract::EmbeddedFont};
    bool m_peers_tab_alternating_row_colors;
    bool fCoinControlFeatures;
    bool m_sub_fee_from_amount;
    bool m_enable_psbt_controls;
    bool m_mask_values;

    /* settings that were overridden by command-line */
    QString strOverriddenByCommandLine;
    bool m_prune_forced_by_gui{false};

    static QString FontChoiceToString(const OptionsModel::FontChoice&);
    static FontChoice FontChoiceFromString(const QString&);

    /* rwconf settings that require a restart */
    bool f_peerbloomfilters;

    // Add option to list of GUI options overridden through command line/config file
    void addOverriddenOption(const std::string &option);

    // Check settings version and upgrade default values if required
    void checkAndMigrate();

Q_SIGNALS:
    void displayUnitChanged(BitcoinUnit unit);
    void coinControlFeaturesChanged(bool);
    void addresstypeChanged(OutputType);
    void showTrayIconChanged(bool);
    void fontForMoneyChanged(const QFont&);
    void fontForQRCodesChanged(const FontChoice&);
    void peersTabAlternatingRowColorsChanged(bool);
};

Q_DECLARE_METATYPE(OptionsModel::FontChoice)

#endif // BITCOIN_QT_OPTIONSMODEL_H
