// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GUIUTIL_FONT_H
#define BITCOIN_QT_GUIUTIL_FONT_H

#include <QFont>
#include <QString>
#include <QStringView>
#include <QTextEdit>
#include <QWidget>

#include <cmath>
#include <cstdint>
#include <map>
#include <vector>

namespace GUIUtil {
// TODO: Switch to QUtf8StringView when we switch to Qt 6
constexpr QStringView MONTSERRAT_FONT_STR{u"Montserrat"};
constexpr QStringView OS_FONT_STR{u"SystemDefault"};
constexpr QStringView OS_MONO_FONT_STR{u"SystemMonospace"};
constexpr QStringView ROBOTO_MONO_FONT_STR{u"Roboto Mono"};

extern std::vector<std::pair<QString, /*selectable=*/bool>> g_fonts_known;

enum class FontWeight : uint8_t {
    Normal,
    Bold,
};

struct FontInfo {
    QFont::Weight m_bold;
    QFont::Weight m_bold_default;
    QFont::Weight m_normal;
    QFont::Weight m_normal_default;
    std::vector<QFont::Weight> m_supported_weights;

    FontInfo() = delete;
    explicit FontInfo(const QString& font_name);
    ~FontInfo();

private:
    QFont::Weight GetBestMatch(const QString& font_name, QFont::Weight target);
    void CalcDefaultWeights(const QString& font_name);
    void CalcSupportedWeights(const QString& font_name);
};

class FontRegistry {
public:
    static constexpr int DEFAULT_FONT_SCALE{0};
    static constexpr int DEFAULT_FONT_SIZE{12};
    static constexpr QStringView DEFAULT_FONT{OS_FONT_STR};
    static constexpr QFont::Weight TARGET_WEIGHT_BOLD{QFont::Medium};
    static constexpr QFont::Weight TARGET_WEIGHT_NORMAL{
#ifdef Q_OS_MACOS
        QFont::ExtraLight
#else
        QFont::Light
#endif // Q_OS_MACOS
    };

public:
    [[nodiscard]] bool RegisterFont(const QString& font, bool selectable, bool skip_checks = false);

    bool IsValidWeight(const QFont::Weight& weight) const { return WeightToIdx(weight) != -1; }
    int WeightToIdx(const QFont::Weight& weight) const;
    QFont::Weight IdxToWeight(int index) const;

    [[nodiscard]] bool SetFont(const QString& font);
    void SetFontScale(int font_scale) { m_font_scale = font_scale; }
    void SetWeightBold(const QFont::Weight& bold)
    {
        assert(m_weights.count(m_font));
        m_weights.at(m_font).m_bold = bold;
    }
    void SetWeightNormal(const QFont::Weight& normal)
    {
        assert(m_weights.count(m_font));
        m_weights.at(m_font).m_normal = normal;
    }

    double GetScaleSteps() const { return m_scale_steps; }
    double GetScaledFontSize(double size) const { return std::round(size * (1 + (m_font_scale * m_scale_steps)) * 4) / 4.0; }
    QString GetFont() const { return m_font; }
    int GetFontScale() const { return m_font_scale; }
    int GetFontSize() const { return m_font_size; }
    QFont::Weight GetWeightBold() const
    {
        if (auto it = m_weights.find(m_font); it != m_weights.end()) { return it->second.m_bold; }
        return TARGET_WEIGHT_BOLD;
    }
    QFont::Weight GetWeightNormal() const
    {
        if (auto it = m_weights.find(m_font); it != m_weights.end()) { return it->second.m_normal; }
        return TARGET_WEIGHT_NORMAL;
    }
    QFont::Weight GetWeightBoldDefault() const
    {
        if (auto it = m_weights.find(m_font); it != m_weights.end()) { return it->second.m_bold_default; }
        return TARGET_WEIGHT_BOLD;
    }
    QFont::Weight GetWeightNormalDefault() const
    {
        if (auto it = m_weights.find(m_font); it != m_weights.end()) { return it->second.m_normal_default; }
        return TARGET_WEIGHT_NORMAL;
    }
    std::vector<QFont::Weight> GetSupportedWeights() const
    {
        if (auto it = m_weights.find(m_font); it != m_weights.end()) { return it->second.m_supported_weights; }
        return {TARGET_WEIGHT_NORMAL, TARGET_WEIGHT_BOLD};
    }

private:
    double m_scale_steps{0.01};
    QString m_font{DEFAULT_FONT.toUtf8()};
    int m_font_scale{DEFAULT_FONT_SCALE};
    int m_font_size{DEFAULT_FONT_SIZE};
    std::map<QString, FontInfo> m_weights;
};

extern FontRegistry g_font_registry;

struct FontAttrib {
    QString m_font;
    FontWeight m_weight_type;
    double m_point_size{-1};
    bool m_is_italic{false};

    FontAttrib(QString font, FontWeight weight_type, double point_size = -1, bool is_italic = false);
    // cppcheck-suppress noExplicitConstructor
    FontAttrib(FontWeight weight_type, double point_size = -1, bool is_italic = false);
    ~FontAttrib();
};

/** Convert weight value from args (0-8) to QFont::Weight */
bool weightFromArg(int nArg, QFont::Weight& weight);

/** Convert QFont::Weight to an arg value (0-8) */
int weightToArg(const QFont::Weight weight);

/** Load dash specific application fonts */
bool loadFonts();

/** Check if the fonts have been loaded successfully */
bool fontsLoaded();

/** Register a QTextEdit for font styling. Applies immediately and updates when fonts change. */
void registerWidget(QTextEdit* widget, const QString& html);

/** Set an application wide default font, depends on the selected theme */
void setApplicationFont();

/** Workaround to set correct font styles in all themes since there is a bug in macOS which leads to
    issues loading variations of montserrat in css it also keeps track of the set fonts to update on
    theme changes. */
void setFont(const std::vector<QWidget*>& vecWidgets, const FontAttrib& font_attrib);

/** Update the font of all widgets where a custom font has been set with
    GUIUtil::setFont */
void updateFonts();

/** Get list of all selectable fonts */
std::vector<QString> getFonts(bool selectable_only);

/** Get the default bold QFont */
QFont getFontBold();

/** Get the default normal QFont */
QFont getFontNormal();

/** Get a scaled font with the specified base size, weight, and optional multiplier. */
QFont getScaledFont(double baseSize, bool bold, double multiplier = 1);

/** (Bitcoin) Return a monospace font */
QFont fixedPitchFont(bool use_embedded_font = false);
} // namespace GUIUtil

#endif // BITCOIN_QT_GUIUTIL_FONT_H
