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

//! Weights considered when testing for weights supported by a font
const auto vecWeightConsider = []() {
    std::vector<QFont::Weight> ret;
    for (const auto& [key, _] : mapWeightArgs.second) {
        ret.push_back(key);
    }
    return ret;
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
// Contains all widgets and its font attributes (weight, italic, size) with font changes due to GUIUtil::setFont
static std::map<QPointer<QWidget>, std::tuple<FontWeight, bool, int>> mapFontUpdates;
// Fonts known by the client
std::vector<QString> g_fonts_known{
    OS_FONT_STR.toUtf8(),
    MONTSERRAT_FONT_STR.toUtf8(),
};

FontRegistry g_font_registry;

FontInfo::FontInfo(const QString& font_name)
{
    CalcSupportedWeights(font_name);
    CalcDefaultWeights(font_name);
    m_bold = m_bold_default;
    m_normal = m_normal_default;
}

FontInfo::~FontInfo() = default;

void FontRegistry::RegisterFont(const QString& font)
{
    m_weights.emplace(font, FontInfo(font));
}

void FontRegistry::SetFont(const QString& font)
{
    if (!m_weights.count(font)) {
        throw std::runtime_error(strprintf("%s: Font family not loaded: %s", __func__, font.toStdString()));
    }
    m_font = font;
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

void FontInfo::CalcSupportedWeights(const QString& font_name)
{
    auto getTestWidth = [](const QString& font_name, QFont::Weight weight) -> int {
        QFont font = getFont(font_name, weight, false, FontRegistry::DEFAULT_FONT_SIZE);
        return TextWidth(QFontMetrics(font), ("Check the width of this text to see if the weight change has an impact!"));
    };
    QFont::Weight prevWeight = vecWeightConsider.front();
    bool isFirst = true;
    for (const auto& weight : vecWeightConsider) {
        if (isFirst) {
            isFirst = false;
            continue;
        }
        if (getTestWidth(font_name, prevWeight) != getTestWidth(font_name, weight)) {
            if (m_supported_weights.empty()) {
                m_supported_weights.push_back(prevWeight);
            }
            m_supported_weights.push_back(weight);
        }
        prevWeight = weight;
    }
    if (m_supported_weights.empty()) {
        m_supported_weights.push_back(QFont::Normal);
    }
}

QFont::Weight FontInfo::GetBestMatch(const QString& font_name, QFont::Weight target)
{
    assert(!m_supported_weights.empty());

    QFont::Weight bestWeight = m_supported_weights.front();
    int nBestDiff = abs(bestWeight - target);
    for (const auto& weight : m_supported_weights) {
        int nDiff = abs(weight - target);
        if (nDiff < nBestDiff) {
            bestWeight = weight;
            nBestDiff = nDiff;
        }
    }
    return bestWeight;
}

void FontInfo::CalcDefaultWeights(const QString& font_name)
{
    assert(!m_supported_weights.empty());

    m_normal_default = GetBestMatch(font_name, FontRegistry::TARGET_WEIGHT_NORMAL);
    m_bold_default = GetBestMatch(font_name, FontRegistry::TARGET_WEIGHT_BOLD);
    if (m_normal_default == m_bold_default) {
        // If the results are the same use the next possible weight for bold font
        auto it = std::find(m_supported_weights.begin(), m_supported_weights.end(), m_normal_default);
        if (++it != m_supported_weights.end()) {
            m_bold_default = *it;
        }
    }
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

    // Import the italic Montserrat variant as it doesn't map to a weight
    importFont(qstrprintf(":fonts/%s-Italic", MONTSERRAT_FONT_STR.toUtf8().toStdString()));
    // Import the rest of Montserrat variants
    for (const auto& [_, val] : mapMontserrat) {
        const auto& [variant, can_italic] = val;
        importFont(qstrprintf(":fonts/%s-%s", MONTSERRAT_FONT_STR.toUtf8().toStdString(), variant));
        if (can_italic) {
            importFont(qstrprintf(":fonts/%s-%sItalic", MONTSERRAT_FONT_STR.toUtf8().toStdString(), variant));
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
        if (f.contains(MONTSERRAT_FONT_STR)) {
            for (const QString& style : database.styles(f)) {
                qDebug() << qstrprintf("%s: Family: %s, Style: %s", __func__, f.toStdString(), style.toStdString());
            }
        }
    }
#endif // QT_NO_DEBUG

    // Initialize supported font weights for all available fonts
    // Generate a vector with supported font weights by comparing the width of a certain test text for all font weights
    for (const auto& fonts : g_fonts_known) {
        g_font_registry.RegisterFont(fonts);
    }

    setApplicationFont();

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

    if (auto family = g_font_registry.GetFont(); family == MONTSERRAT_FONT_STR) {
#ifdef Q_OS_MACOS
        if (g_font_registry.GetWeightNormal() != FontRegistry::TARGET_WEIGHT_NORMAL) {
            font = std::make_unique<QFont>(getFontNormal());
        } else {
            font = std::make_unique<QFont>(family);
            font->setWeight(FontRegistry::TARGET_WEIGHT_NORMAL);
        }
#else
        font = std::make_unique<QFont>(family);
        font->setWeight(g_font_registry.GetWeightNormal());
#endif
    } else {
        font = std::make_unique<QFont>(*osDefaultFont);
    }

    font->setPointSizeF(g_font_registry.GetFontSize());
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
        font.setWeight(g_font_registry.GetWeightNormal());
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
            font.setPointSizeF(g_font_registry.GetScaledFontSize(itDefault.first->second));
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
        double dSize = g_font_registry.GetScaledFontSize(it.second);
        if (fontClass.pointSizeF() != dSize) {
            fontClass.setPointSizeF(dSize);
            qApp->setFont(fontClass, it.first.c_str());
        }
    }
}

QFont getFont(const QString& font_name, QFont::Weight weight, bool italic, int point_sz)
{
    QFont font;
    if (!fontsLoaded()) {
        return font;
    }

    if (font_name == MONTSERRAT_FONT_STR) {
        assert(mapMontserrat.count(weight));
#ifdef Q_OS_MACOS
        font.setFamily(font_name);
        font.setStyleName([&]() {
            std::string ret{mapMontserrat.at(weight).first};
            if (italic) {
                if (ret == "Regular") {
                    ret = "Italic";
                } else {
                    ret += " Italic";
                }
            }
            return QString::fromStdString(ret);
        }());
#else
        if (weight == QFont::Normal || weight == QFont::Bold) {
            font.setFamily(font_name);
        } else {
            font.setFamily(font_name + QString{" "} + QString::fromStdString(mapMontserrat.at(weight).first));
        }
#endif // Q_OS_MACOS
    } else if (font_name == OS_FONT_STR) {
        font.setFamily(osDefaultFont->family());
    } else {
        font.setFamily(font_name);
    }

#ifdef Q_OS_MACOS
    if (font_name != MONTSERRAT_FONT_STR)
#endif // Q_OS_MACOS
    {
        font.setWeight(weight);
        font.setStyle(italic ? QFont::StyleItalic : QFont::StyleNormal);
    }

    if (point_sz != -1) {
        font.setPointSizeF(g_font_registry.GetScaledFontSize(point_sz));
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
    return getFont(g_font_registry.GetFont(), qWeight, fItalic, nPointSize);
}

QFont getFont(FontWeight weight, bool fItalic, int nPointSize)
{
    return getFont(weight == FontWeight::Bold ? g_font_registry.GetWeightBold()
                                              : g_font_registry.GetWeightNormal(), fItalic, nPointSize);
}

QFont getFontNormal()
{
    return getFont(FontWeight::Normal);
}

QFont getFontBold()
{
    return getFont(FontWeight::Bold);
}

QFont::Weight FontRegistry::IdxToWeight(int index) const
{
    const auto vecWeights = GetSupportedWeights();
    assert(vecWeights.size() > uint64_t(index));
    return vecWeights.at(index);
}

int FontRegistry::WeightToIdx(const QFont::Weight& weight) const
{
    const auto vecWeights = GetSupportedWeights();
    for (uint64_t index = 0; index < vecWeights.size(); ++index) {
        if (weight == vecWeights.at(index)) {
            return index;
        }
    }
    return -1;
}
} // namespace GUIUtil
