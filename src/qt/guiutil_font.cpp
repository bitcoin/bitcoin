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
//! Instance of font database shared among calls
std::unique_ptr<QFontDatabase> g_font_db{nullptr};

//! loadFonts stores the SystemDefault font in g_default_font to be able to reference it later again
std::unique_ptr<QFont> g_default_font{nullptr};

//! Font scaling information for Qt classes
std::map<std::string, int> mapClassFontUpdates{
    {"QMenu", -1},
    {"QMessageBox", -1},
    {"QTipLabel", -1},
};

//! Contains all widgets and its font attributes (weight, italic, size) with font changes due to GUIUtil::setFont
std::map<QPointer<QWidget>, GUIUtil::FontAttrib> mapFontUpdates;

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

//! List of Qt classes to ignore when applying fonts
constexpr std::array<std::string_view, 18> vecIgnoreClasses{
    "BitcoinGUI",
    "QComboBoxPrivateContainer",
    "QComboBoxPrivateScroller",
    "QDesktopScreenWidget",
    "QDesktopWidget",
    "QDialog",
    "QFrame",
    "QGroupBox",
    "QListView",
    "QMenu",
    "QMessageBox",
    "QScrollBar",
    "QStackedWidget",
    "QTipLabel",
    "QVBoxLayout",
    "QWidget",
    "WalletFrame",
    "WalletView",
};

//! List of Qt objects to ignore when applying fonts
constexpr std::array<std::string_view, 2> vecIgnoreObjects{
    "messagesWidget",
    "moneyFont_preview",
};

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
//! Fonts known by the client
std::vector<std::pair<QString, /*selectable=*/bool>> g_fonts_known{
    {MONTSERRAT_FONT_STR.toUtf8(), true},
    {OS_FONT_STR.toUtf8(), true},
    {OS_MONO_FONT_STR.toUtf8(), false},
    {ROBOTO_MONO_FONT_STR.toUtf8(), false},
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

bool FontRegistry::RegisterFont(const QString& font, bool selectable, bool skip_checks)
{
    const auto font_strs{getFonts(/*selectable_only=*/false)};
    auto font_it{std::find(font_strs.begin(), font_strs.end(), font)};
    if (m_weights.count(font)) {
        // Font's already registered
        assert(font_it != font_strs.end());
        // Overwrite selectable flag
        g_fonts_known.at(std::distance(font_strs.begin(), font_it)).second = selectable;
        return true;
    }
    if (!skip_checks) {
        if (!g_font_db) { g_font_db = std::make_unique<QFontDatabase>(); }
        if (!g_font_db->families().contains(font, Qt::CaseInsensitive)) {
            // Font doesn't exist
            return false;
        }
    }
    m_weights.emplace(font, FontInfo(font));
    if (font_it == font_strs.end()) {
        g_fonts_known.emplace_back(font, selectable);
    }
    return true;
}

bool FontRegistry::SetFont(const QString& font)
{
    if (!m_weights.count(font)) {
        return false;
    }
    m_font = font;
    return true;
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
        QFont font = getFont({font_name, weight, FontRegistry::DEFAULT_FONT_SIZE, false});
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

FontAttrib::FontAttrib(QString font, QFont::Weight weight, double point_size, bool is_italic) :
    m_font{font},
    m_weight{weight},
    m_point_size{point_size},
    m_is_italic{is_italic}
{
}

FontAttrib::FontAttrib(QFont::Weight weight, double point_size, bool is_italic) :
    m_font{g_font_registry.GetFont()},
    m_weight{weight},
    m_point_size{point_size},
    m_is_italic{is_italic}
{
}

FontAttrib::~FontAttrib() = default;

bool loadFonts()
{
    // Before any font changes store the applications default font to use it as SystemDefault.
    g_default_font = std::make_unique<QFont>(QApplication::font());

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
        g_default_font = nullptr;
        return false;
    }

#ifndef QT_NO_DEBUG
    if (!g_font_db) { g_font_db = std::make_unique<QFontDatabase>(); }

    // Print debug logs for added fonts fetched by the added ids
    for (const auto& i : vecFontIds) {
        for (const QString& f : QFontDatabase::applicationFontFamilies(i)) {
            qDebug() << qstrprintf("%s: - Font id %d is family: %s", __func__, i, f.toStdString());
            for (const QString& style : g_font_db->styles(f)) {
                qDebug() << qstrprintf("%s: Style for family %s with id: %d is %s", __func__, f.toStdString(), i,
                                       style.toStdString());
            }
        }
    }

    // Print debug logs for added fonts fetched by the family name
    for (const QString& f : g_font_db->families()) {
        if (f.contains(MONTSERRAT_FONT_STR)) {
            for (const QString& style : g_font_db->styles(f)) {
                qDebug() << qstrprintf("%s: Family: %s, Style: %s", __func__, f.toStdString(), style.toStdString());
            }
        }
    }
#endif // QT_NO_DEBUG

    for (const auto& [fonts, selectable] : g_fonts_known) {
        assert(g_font_registry.RegisterFont(fonts, selectable, /*skip_checks=*/true));
    }

    return true;
}

bool fontsLoaded()
{
    return g_default_font != nullptr;
}

void setApplicationFont()
{
    if (!fontsLoaded()) {
        return;
    }

    std::unique_ptr<QFont> font;

    auto family = g_font_registry.GetFont();
    if (family == MONTSERRAT_FONT_STR) {
#ifdef Q_OS_MACOS
        font = std::make_unique<QFont>(getFontNormal());
#else
        font = std::make_unique<QFont>(family);
        font->setWeight(g_font_registry.GetWeightNormal());
#endif
    } else if (family == OS_FONT_STR) {
        font = std::make_unique<QFont>(*g_default_font);
    } else {
        font = std::make_unique<QFont>(family);
    }

    font->setPointSizeF(g_font_registry.GetFontSize());
    qApp->setFont(*font);

    qDebug() << qstrprintf("%s: %s family: %s, style: %s match: %s", __func__, qApp->font().toString().toStdString(),
                           qApp->font().family().toStdString(), qApp->font().styleName().toStdString(),
                           qApp->font().exactMatch() ? "true" : "false");
}

void setFont(const std::vector<QWidget*>& vecWidgets, const FontAttrib& font_attrib)
{
    for (auto it : vecWidgets) {
        auto itFontUpdate = mapFontUpdates.emplace(std::make_pair(it, font_attrib));
        if (!itFontUpdate.second) {
            itFontUpdate.first->second = font_attrib;
        }
    }
}

void updateFonts()
{
    // Fonts need to be loaded by GUIUtil::loadFonts(), if not just return.
    if (!g_default_font) {
        return;
    }

    static std::map<QPointer<QWidget>, double> mapWidgetDefaultFontSizes;

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
        if (std::find(vecIgnoreClasses.begin(), vecIgnoreClasses.end(), w->metaObject()->className()) != vecIgnoreClasses.end() ||
            std::find(vecIgnoreObjects.begin(), vecIgnoreObjects.end(), w->objectName().toStdString()) != vecIgnoreObjects.end()) {
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
            double nSize = it->second.m_point_size;
            if (nSize == -1) {
                nSize = itDefault.first->second;
            }
            font = getFont({it->second.m_font, it->second.m_weight, nSize, it->second.m_is_italic});
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

QFont getFont(const FontAttrib& font_attrib)
{
    QFont font;
    if (!fontsLoaded()) {
        return font;
    }

    if (font_attrib.m_font == MONTSERRAT_FONT_STR) {
        assert(mapMontserrat.count(font_attrib.m_weight));
#ifdef Q_OS_MACOS
        font.setFamily(font_attrib.m_font);
        font.setStyleName([&]() {
            std::string ret{mapMontserrat.at(font_attrib.m_weight).first};
            if (font_attrib.m_is_italic) {
                if (ret == "Regular") {
                    ret = "Italic";
                } else {
                    ret += " Italic";
                }
            }
            return QString::fromStdString(ret);
        }());
#else
        if (font_attrib.m_weight == QFont::Normal || font_attrib.m_weight == QFont::Bold) {
            font.setFamily(font_attrib.m_font);
        } else {
            font.setFamily(qstrprintf("%s %s", font_attrib.m_font.toStdString(), mapMontserrat.at(font_attrib.m_weight).first));
        }
#endif // Q_OS_MACOS
    } else if (font_attrib.m_font == OS_FONT_STR) {
        font.setFamily(g_default_font->family());
    } else if (font_attrib.m_font == OS_MONO_FONT_STR) {
        font.setFamily(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
    } else {
        font.setFamily(font_attrib.m_font);
    }

    if (font_attrib.m_font == ROBOTO_MONO_FONT_STR || font_attrib.m_font == OS_MONO_FONT_STR) {
        font.setStyleHint(QFont::Monospace);
    }

#ifdef Q_OS_MACOS
    if (font_attrib.m_font != MONTSERRAT_FONT_STR)
#endif // Q_OS_MACOS
    {
        font.setWeight(font_attrib.m_weight);
        font.setStyle(font_attrib.m_is_italic ? QFont::StyleItalic : QFont::StyleNormal);
    }

    if (font_attrib.m_point_size != -1) {
        font.setPointSizeF(g_font_registry.GetScaledFontSize(font_attrib.m_point_size));
    }

    if (gArgs.GetBoolArg("-debug-ui", false)) {
        qDebug() << qstrprintf("%s: font size: %d, family: %s, style: %s, weight: %d match %s", __func__,
                               font.pointSizeF(), font.family().toStdString(), font.styleName().toStdString(),
                               font.weight(), font.exactMatch() ? "true" : "false");
    }

    return font;
}

std::vector<QString> getFonts(bool selectable_only)
{
    std::vector<QString> ret;
    for (const auto& [font, selectable] : g_fonts_known) {
        if (selectable || !selectable_only) { ret.emplace_back(font); }
    }
    return ret;
}

QFont getFontNormal()
{
    return getFont({g_font_registry.GetWeightNormal()});
}

QFont getFontBold()
{
    return getFont({g_font_registry.GetWeightBold()});
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

QFont fixedPitchFont(bool use_embedded_font)
{
    return getFont({
        use_embedded_font ? ROBOTO_MONO_FONT_STR.toUtf8() : OS_MONO_FONT_STR.toUtf8(),
        g_font_registry.GetWeightNormal()
    });
}
} // namespace GUIUtil
