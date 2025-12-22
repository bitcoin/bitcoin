// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/guiutil_font.h>

#include <tinyformat.h>
#include <util/system.h>

#include <qt/guiutil.h>

#include <QApplication>
#include <QDebug>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QPointer>
#include <QWidget>

#include <cmath>
#include <map>
#include <memory>
#include <utility>

namespace {
//! Map between font weights, Montserrat's convention and italic availability
const std::map<QFont::Weight, std::pair<std::string, /*can_italic=*/bool>> mapMontserrat{{
    {QFont::Black, {"Black", true}},
    {QFont::Bold, {"Bold", true}},
    {QFont::DemiBold, {"SemiBold", true}},
    {QFont::ExtraBold, {"ExtraBold", true}},
    {QFont::ExtraLight, {"ExtraLight", true}},
    {QFont::Light, {"Light", true}},
    {QFont::Medium, {"Medium", true}},
    {QFont::Normal, {"Regular", false}},
    {QFont::Thin, {"Thin", true}},
}};

//! Map between font weights and settings representation
const auto mapWeightArgs = []() {
    std::map<int, QFont::Weight> kv{
        {0, QFont::Thin},
        {1, QFont::ExtraLight},
        {2, QFont::Light},
        {3, QFont::Normal},
        {4, QFont::Medium},
        {5, QFont::DemiBold},
        {6, QFont::Bold},
        {7, QFont::ExtraBold},
        {8, QFont::Black}
    };
    std::map<QFont::Weight, int> vk;
    for (const auto& [key, val] : kv) {
        vk[val] = key;
    }
    return std::pair{std::move(kv), std::move(vk)};
}();

//! Wrapper for tinyformat (strprintf) that converts to QString
template <typename... Args>
QString qstrprintf(const std::string& fmt, const Args&... args)
{
    return QString::fromStdString(tfm::format(fmt, args...));
}
} // anonymous namespace

namespace GUIUtil {
/** loadFonts stores the SystemDefault font in osDefaultFont to be able to reference it later again */
static std::unique_ptr<QFont> osDefaultFont;
/** Font related default values. */
static const FontFamily defaultFontFamily = FontFamily::SystemDefault;
static const int defaultFontSize = 12;
static const double fontScaleSteps = 0.01;
#ifdef Q_OS_MACOS
static const QFont::Weight defaultFontWeightNormal = QFont::ExtraLight;
static const QFont::Weight defaultFontWeightBold = QFont::Medium;
static const int defaultFontScale = 0;
#else
static const QFont::Weight defaultFontWeightNormal = QFont::Light;
static const QFont::Weight defaultFontWeightBold = QFont::Medium;
static const int defaultFontScale = 0;
#endif

/** Font related variables. */
// Application font family. May be overwritten by -font-family.
static FontFamily fontFamily = defaultFontFamily;
// Application font scale value. May be overwritten by -font-scale.
static int fontScale = defaultFontScale;
// Contains the weight settings separated for all available fonts
static std::map<FontFamily, std::pair<QFont::Weight, QFont::Weight>> mapDefaultWeights;
static std::map<FontFamily, std::pair<QFont::Weight, QFont::Weight>> mapWeights;
// Contains all widgets and its font attributes (weight, italic, size) with font changes due to GUIUtil::setFont
static std::map<QPointer<QWidget>, std::tuple<FontWeight, bool, int>> mapFontUpdates;
// Contains a list of supported font weights for all members of GUIUtil::FontFamily
static std::map<FontFamily, std::vector<QFont::Weight>> mapSupportedWeights;

FontFamily fontFamilyFromString(const QString& strFamily)
{
    if (strFamily == "SystemDefault") {
        return FontFamily::SystemDefault;
    }
    if (strFamily == "Montserrat") {
        return FontFamily::Montserrat;
    }
    throw std::invalid_argument(strprintf("Invalid font-family: %s", strFamily.toStdString()));
}

QString fontFamilyToString(FontFamily family)
{
    switch (family) {
    case FontFamily::SystemDefault:
        return "SystemDefault";
    case FontFamily::Montserrat:
        return "Montserrat";
    default:
        assert(false);
    }
}

FontFamily getFontFamilyDefault()
{
    return defaultFontFamily;
}

FontFamily getFontFamily()
{
    return fontFamily;
}

void setFontFamily(FontFamily family)
{
    fontFamily = family;
    setApplicationFont();
    updateFonts();
}

bool weightFromArg(int nArg, QFont::Weight& weight)
{
    auto it = mapWeightArgs.first.find(nArg);
    if (it == mapWeightArgs.first.end()) {
        return false;
    }
    weight = it->second;
    return true;
}

int weightToArg(const QFont::Weight weight)
{
    assert(mapWeightArgs.second.count(weight));
    return mapWeightArgs.second.find(weight)->second;
}

QFont::Weight toQFontWeight(FontWeight weight)
{
    return weight == FontWeight::Bold ? getFontWeightBold() : getFontWeightNormal();
}

QFont::Weight getFontWeightNormalDefault()
{
    return defaultFontWeightNormal;
}

QFont::Weight getFontWeightNormal()
{
    if (!mapWeights.count(fontFamily)) {
        return defaultFontWeightNormal;
    }
    return mapWeights[fontFamily].first;
}

void setFontWeightNormal(QFont::Weight weight)
{
    if (!mapWeights.count(fontFamily)) {
        throw std::runtime_error(
            strprintf("%s: Font family not loaded: %s", __func__, fontFamilyToString(fontFamily).toStdString()));
    }
    mapWeights[fontFamily].first = weight;
    updateFonts();
}

QFont::Weight getFontWeightBoldDefault()
{
    return defaultFontWeightBold;
}

QFont::Weight getFontWeightBold()
{
    if (!mapWeights.count(fontFamily)) {
        return defaultFontWeightBold;
    }
    return mapWeights[fontFamily].second;
}

void setFontWeightBold(QFont::Weight weight)
{
    if (!mapWeights.count(fontFamily)) {
        throw std::runtime_error(
            strprintf("%s: Font family not loaded: %s", __func__, fontFamilyToString(fontFamily).toStdString()));
    }
    mapWeights[fontFamily].second = weight;
    updateFonts();
}

int getFontScaleDefault()
{
    return defaultFontScale;
}

int getFontScale()
{
    return fontScale;
}

void setFontScale(int nScale)
{
    fontScale = nScale;
    updateFonts();
}

double getScaledFontSize(int nSize)
{
    return std::round(nSize * (1 + (fontScale * fontScaleSteps)) * 4) / 4.0;
}

bool loadFonts()
{
    // Before any font changes store the applications default font to use it as SystemDefault.
    osDefaultFont = std::make_unique<QFont>(QApplication::font());

    std::vector<int> vecFontIds{};
    auto importFont = [&vecFontIds](const QString& font_name) -> void {
        vecFontIds.push_back(QFontDatabase::addApplicationFont(font_name));
        qDebug() << qstrprintf("%s: %s loaded with id %d", __func__, font_name.toStdString(), vecFontIds.back());
    };

    // Load Montserrat
    const std::string montserrat_str{fontFamilyToString(FontFamily::Montserrat).toStdString()};
    // Import the italic Montserrat variant as it doesn't map to a weight
    importFont(qstrprintf(":fonts/%s-Italic", montserrat_str));
    // Import the rest of Montserrat variants
    for (const auto& [_, val] : mapMontserrat) {
        const auto& [variant, can_italic] = val;
        importFont(qstrprintf(":fonts/%s-%s", montserrat_str, variant));
        if (can_italic) {
            importFont(qstrprintf(":fonts/%s-%sItalic", montserrat_str, variant));
        }
    }

    // Fail if an added id is -1 which means QFontDatabase::addApplicationFont failed.
    if (std::find(vecFontIds.begin(), vecFontIds.end(), -1) != vecFontIds.end()) {
        osDefaultFont = nullptr;
        return false;
    }

#ifndef QT_NO_DEBUG
    QFontDatabase database;

    // Print debug logs for added fonts fetched by the added ids
    for (const auto& i : vecFontIds) {
        for (const QString& f : QFontDatabase::applicationFontFamilies(i)) {
            qDebug() << qstrprintf("%s: - Font id %d is family: %s", __func__, i, f.toStdString());
            for (const QString& style : database.styles(f)) {
                qDebug() << qstrprintf("%s: Style for family %s with id: %d is %s", __func__, f.toStdString(), i,
                                       style.toStdString());
            }
        }
    }

    // Print debug logs for added fonts fetched by the family name
    for (const QString& f : database.families()) {
        if (f.contains(QString::fromStdString(montserrat_str))) {
            for (const QString& style : database.styles(f)) {
                qDebug() << qstrprintf("%s: Family: %s, Style: %s", __func__, f.toStdString(), style.toStdString());
            }
        }
    }
#endif // QT_NO_DEBUG

    setApplicationFont();

    // Initialize supported font weights for all available fonts
    // Generate a vector with supported font weights by comparing the width of a certain test text for all font weights
    auto supportedWeights = [](FontFamily family) -> std::vector<QFont::Weight> {
        auto getTestWidth = [&](QFont::Weight weight) -> int {
            QFont font = getFont(family, weight, false, defaultFontSize);
            return TextWidth(QFontMetrics(font),
                             ("Check the width of this text to see if the weight change has an impact!"));
        };
        std::vector<QFont::Weight> vecWeights{QFont::Thin,   QFont::ExtraLight, QFont::Light,
                                              QFont::Normal, QFont::Medium,     QFont::DemiBold,
                                              QFont::Bold,   QFont::ExtraBold,  QFont::Black};
        std::vector<QFont::Weight> vecSupported;
        QFont::Weight prevWeight = vecWeights.front();
        for (auto weight = vecWeights.begin() + 1; weight != vecWeights.end(); ++weight) {
            if (getTestWidth(prevWeight) != getTestWidth(*weight)) {
                if (vecSupported.empty()) {
                    vecSupported.push_back(prevWeight);
                }
                vecSupported.push_back(*weight);
            }
            prevWeight = *weight;
        }
        if (vecSupported.empty()) {
            vecSupported.push_back(QFont::Normal);
        }
        return vecSupported;
    };

    mapSupportedWeights.insert(std::make_pair(FontFamily::SystemDefault, supportedWeights(FontFamily::SystemDefault)));
    mapSupportedWeights.insert(std::make_pair(FontFamily::Montserrat, supportedWeights(FontFamily::Montserrat)));

    auto getBestMatch = [&](FontFamily fontFamily, QFont::Weight targetWeight) {
        auto& vecSupported = mapSupportedWeights[fontFamily];
        auto it = vecSupported.begin();
        QFont::Weight bestWeight = *it;
        int nBestDiff = abs(*it - targetWeight);
        while (++it != vecSupported.end()) {
            int nDiff = abs(*it - targetWeight);
            if (nDiff < nBestDiff) {
                bestWeight = *it;
                nBestDiff = nDiff;
            }
        }
        return bestWeight;
    };

    auto addBestDefaults = [&](FontFamily family) -> auto {
        QFont::Weight normalWeight = getBestMatch(family, defaultFontWeightNormal);
        QFont::Weight boldWeight = getBestMatch(family, defaultFontWeightBold);
        if (normalWeight == boldWeight) {
            // If the results are the same use the next possible weight for bold font
            auto& vecSupported = mapSupportedWeights[family];
            auto it = std::find(vecSupported.begin(), vecSupported.end(), normalWeight);
            if (++it != vecSupported.end()) {
                boldWeight = *it;
            }
        }
        mapDefaultWeights.emplace(family, std::make_pair(normalWeight, boldWeight));
    };

    addBestDefaults(FontFamily::SystemDefault);
    addBestDefaults(FontFamily::Montserrat);

    // Load supported defaults. May become overwritten later.
    mapWeights = mapDefaultWeights;

    return true;
}

bool fontsLoaded()
{
    return osDefaultFont != nullptr;
}

void setApplicationFont()
{
    if (!fontsLoaded()) {
        return;
    }

    std::unique_ptr<QFont> font;

    if (fontFamily == FontFamily::Montserrat) {
        QString family = fontFamilyToString(FontFamily::Montserrat);
#ifdef Q_OS_MACOS
        if (getFontWeightNormal() != getFontWeightNormalDefault()) {
            font = std::make_unique<QFont>(getFontNormal());
        } else {
            font = std::make_unique<QFont>(family);
            font->setWeight(getFontWeightNormalDefault());
        }
#else
        font = std::make_unique<QFont>(family);
        font->setWeight(getFontWeightNormal());
#endif
    } else {
        font = std::make_unique<QFont>(*osDefaultFont);
    }

    font->setPointSizeF(defaultFontSize);
    qApp->setFont(*font);

    qDebug() << qstrprintf("%s: %s family: %s, style: %s match: %s", __func__, qApp->font().toString().toStdString(),
                           qApp->font().family().toStdString(), qApp->font().styleName().toStdString(),
                           qApp->font().exactMatch() ? "true" : "false");
}

void setFont(const std::vector<QWidget*>& vecWidgets, FontWeight weight, int nPointSize, bool fItalic)
{
    for (auto it : vecWidgets) {
        auto fontAttributes = std::make_tuple(weight, fItalic, nPointSize);
        auto itFontUpdate = mapFontUpdates.emplace(std::make_pair(it, fontAttributes));
        if (!itFontUpdate.second) {
            itFontUpdate.first->second = fontAttributes;
        }
    }
}

void updateFonts()
{
    // Fonts need to be loaded by GUIUtil::loadFonts(), if not just return.
    if (!osDefaultFont) {
        return;
    }

    static std::map<QPointer<QWidget>, int> mapWidgetDefaultFontSizes;

    // QPointer becomes nullptr for objects that were deleted.
    // Remove them from mapDefaultFontSize and mapFontUpdates
    // before proceeding any further.
    size_t nRemovedDefaultFonts{0};
    auto itd = mapWidgetDefaultFontSizes.begin();
    while (itd != mapWidgetDefaultFontSizes.end()) {
        if (itd->first.isNull()) {
            itd = mapWidgetDefaultFontSizes.erase(itd);
            ++nRemovedDefaultFonts;
        } else {
            ++itd;
        }
    }

    size_t nRemovedFontUpdates{0};
    auto itn = mapFontUpdates.begin();
    while (itn != mapFontUpdates.end()) {
        if (itn->first.isNull()) {
            itn = mapFontUpdates.erase(itn);
            ++nRemovedFontUpdates;
        } else {
            ++itn;
        }
    }

    size_t nUpdatable{0}, nUpdated{0};
    std::map<QWidget*, QFont> mapWidgetFonts;
    // Loop through all widgets
    for (QWidget* w : qApp->allWidgets()) {
        std::vector<QString> vecIgnoreClasses{
            "QWidget", "QDialog", "QFrame", "QStackedWidget", "QDesktopWidget", "QDesktopScreenWidget",
            "QTipLabel", "QMessageBox", "QMenu", "QComboBoxPrivateScroller", "QComboBoxPrivateContainer",
            "QScrollBar", "QListView", "BitcoinGUI", "WalletView", "WalletFrame", "QVBoxLayout", "QGroupBox"
        };
        std::vector<QString> vecIgnoreObjects{
            "messagesWidget"
        };
        if (std::find(vecIgnoreClasses.begin(), vecIgnoreClasses.end(), w->metaObject()->className()) != vecIgnoreClasses.end() ||
            std::find(vecIgnoreObjects.begin(), vecIgnoreObjects.end(), w->objectName()) != vecIgnoreObjects.end()) {
            continue;
        }
        ++nUpdatable;

        QFont font = w->font();
        assert(font.pointSize() > 0);
        font.setFamily(qApp->font().family());
        font.setWeight(getFontWeightNormal());
        font.setStyleName(qApp->font().styleName());
        font.setStyle(qApp->font().style());

        // Insert/Get the default font size of the widget
        auto itDefault = mapWidgetDefaultFontSizes.emplace(w, font.pointSize());

        auto it = mapFontUpdates.find(w);
        if (it != mapFontUpdates.end()) {
            int nSize = std::get<2>(it->second);
            if (nSize == -1) {
                nSize = itDefault.first->second;
            }
            font = getFont(std::get<0>(it->second), std::get<1>(it->second), nSize);
        } else {
            font.setPointSizeF(getScaledFontSize(itDefault.first->second));
        }

        if (w->font() != font) {
            auto itWidgetFont = mapWidgetFonts.emplace(w, font);
            assert(itWidgetFont.second);
            ++nUpdated;
        }
    }
    qDebug().nospace() << qstrprintf("%s - widget counts: updated/updatable/total(%d/%d/%d), removed items: "
                                     "mapWidgetDefaultFontSizes/mapFontUpdates(%d/%d)",
                                     __func__, nUpdated, nUpdatable, qApp->allWidgets().size(), nRemovedDefaultFonts,
                                     nRemovedFontUpdates);

    // Perform the required font updates
    // NOTE: This is done as separate step to avoid scaling issues due to font inheritance
    //       hence all fonts are calculated and stored in mapWidgetFonts above.
    for (auto it : mapWidgetFonts) {
        it.first->setFont(it.second);
    }

    // Scale the global font size for the classes in the map below
    static std::map<std::string, int> mapClassFontUpdates{
        {"QTipLabel", -1}, {"QMenu", -1}, {"QMessageBox", -1}
    };
    for (auto& it : mapClassFontUpdates) {
        QFont fontClass = qApp->font(it.first.c_str());
        if (it.second == -1) {
            it.second = fontClass.pointSize();
        }
        double dSize = getScaledFontSize(it.second);
        if (fontClass.pointSizeF() != dSize) {
            fontClass.setPointSizeF(dSize);
            qApp->setFont(fontClass, it.first.c_str());
        }
    }
}

QFont getFont(FontFamily family, QFont::Weight qWeight, bool fItalic, int nPointSize)
{
    QFont font;
    if (!fontsLoaded()) {
        return font;
    }

    if (family == FontFamily::Montserrat) {
        assert(mapMontserrat.count(qWeight));
#ifdef Q_OS_MACOS
        font.setFamily(fontFamilyToString(FontFamily::Montserrat));
        font.setStyleName([&]() {
            std::string ret{mapMontserrat.at(qWeight).first};
            if (fItalic) {
                if (ret == "Regular") {
                    ret = "Italic";
                } else {
                    ret += " Italic";
                }
            }
            return QString::fromStdString(ret);
        }());
#else
        if (qWeight == QFont::Normal || qWeight == QFont::Bold) {
            font.setFamily(fontFamilyToString(FontFamily::Montserrat));
        } else {
            font.setFamily(fontFamilyToString(FontFamily::Montserrat) + QString{" "} + QString::fromStdString(mapMontserrat.at(qWeight).first));
        }
#endif // Q_OS_MACOS
    } else {
        font.setFamily(osDefaultFont->family());
    }

#ifdef Q_OS_MACOS
    if (family != FontFamily::Montserrat)
#endif // Q_OS_MACOS
    {
        font.setWeight(qWeight);
        font.setStyle(fItalic ? QFont::StyleItalic : QFont::StyleNormal);
    }

    if (nPointSize != -1) {
        font.setPointSizeF(getScaledFontSize(nPointSize));
    }

    if (gArgs.GetBoolArg("-debug-ui", false)) {
        qDebug() << qstrprintf("%s: font size: %d, family: %s, style: %s, weight: %d match %s", __func__,
                               font.pointSizeF(), font.family().toStdString(), font.styleName().toStdString(),
                               font.weight(), font.exactMatch() ? "true" : "false");
    }

    return font;
}

QFont getFont(QFont::Weight qWeight, bool fItalic, int nPointSize)
{
    return getFont(fontFamily, qWeight, fItalic, nPointSize);
}
QFont getFont(FontWeight weight, bool fItalic, int nPointSize)
{
    return getFont(toQFontWeight(weight), fItalic, nPointSize);
}

QFont getFontNormal()
{
    return getFont(FontWeight::Normal);
}

QFont getFontBold()
{
    return getFont(FontWeight::Bold);
}

QFont::Weight getSupportedFontWeightNormalDefault()
{
    if (!mapDefaultWeights.count(fontFamily)) {
        throw std::runtime_error(
            strprintf("%s: Font family not loaded: %s", __func__, fontFamilyToString(fontFamily).toStdString()));
    }
    return mapDefaultWeights[fontFamily].first;
}

QFont::Weight getSupportedFontWeightBoldDefault()
{
    if (!mapDefaultWeights.count(fontFamily)) {
        throw std::runtime_error(
            strprintf("%s: Font family not loaded: %s", __func__, fontFamilyToString(fontFamily).toStdString()));
    }
    return mapDefaultWeights[fontFamily].second;
}

std::vector<QFont::Weight> getSupportedWeights()
{
    assert(mapSupportedWeights.count(fontFamily));
    return mapSupportedWeights[fontFamily];
}

QFont::Weight supportedWeightFromIndex(int nIndex)
{
    auto vecWeights = getSupportedWeights();
    assert(vecWeights.size() > uint64_t(nIndex));
    return vecWeights[nIndex];
}

int supportedWeightToIndex(QFont::Weight weight)
{
    auto vecWeights = getSupportedWeights();
    for (uint64_t index = 0; index < vecWeights.size(); ++index) {
        if (weight == vecWeights[index]) {
            return index;
        }
    }
    return -1;
}

bool isSupportedWeight(const QFont::Weight weight)
{
    return supportedWeightToIndex(weight) != -1;
}
} // namespace GUIUtil
