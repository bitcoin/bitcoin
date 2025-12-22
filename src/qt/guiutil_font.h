// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GUIUTIL_FONT_H
#define BITCOIN_QT_GUIUTIL_FONT_H

#include <QFont>
#include <QString>
#include <QWidget>

#include <cmath>
#include <cstdint>
#include <vector>

namespace GUIUtil {
enum class FontFamily : uint8_t {
    SystemDefault,
    Montserrat,
};

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
    static constexpr FontFamily DEFAULT_FONT{FontFamily::SystemDefault};
    static constexpr QFont::Weight TARGET_WEIGHT_BOLD{QFont::Medium};
    static constexpr QFont::Weight TARGET_WEIGHT_NORMAL{
#ifdef Q_OS_MACOS
        QFont::ExtraLight
#else
        QFont::Light
#endif // Q_OS_MACOS
    };

public:
    void RegisterFont(const FontFamily& font);

    bool IsValidWeight(const QFont::Weight& weight) const { return WeightToIdx(weight) != -1; }
    int WeightToIdx(const QFont::Weight& weight) const;
    QFont::Weight IdxToWeight(int index) const;

    void SetFont(const FontFamily& font);
    void SetFontScale(int font_scale) { m_font_scale = font_scale; }
    void SetWeightBold(const QFont::Weight& bold) { m_weights.at(m_font).m_bold = bold; }
    void SetWeightNormal(const QFont::Weight& normal) { m_weights.at(m_font).m_normal = normal; }

    double GetScaleSteps() const { return m_scale_steps; }
    double GetScaledFontSize(int size) const { return std::round(size * (1 + (m_font_scale * m_scale_steps)) * 4) / 4.0; }
    FontFamily GetFont() const { return m_font; }
    int GetFontScale() const { return m_font_scale; }
    int GetFontSize() const { return m_font_size; }
    QFont::Weight GetWeightBold() const { return m_weights.at(m_font).m_bold; }
    QFont::Weight GetWeightNormal() const { return m_weights.at(m_font).m_normal; }
    QFont::Weight GetWeightBoldDefault() const { return m_weights.at(m_font).m_bold_default; }
    QFont::Weight GetWeightNormalDefault() const { return m_weights.at(m_font).m_normal_default; }
    std::vector<QFont::Weight> GetSupportedWeights() const { return m_weights.at(m_font).m_supported_weights; }

private:
    double m_scale_steps{0.01};
    FontFamily m_font{DEFAULT_FONT};
    int m_font_scale{DEFAULT_FONT_SCALE};
    int m_font_size{DEFAULT_FONT_SIZE};
    std::map<FontFamily, FontInfo> m_weights;
};

extern FontRegistry g_font_registry;

FontFamily fontFamilyFromString(const QString& strFamily);
QString fontFamilyToString(FontFamily family);

/** Convert weight value from args (0-8) to QFont::Weight */
bool weightFromArg(int nArg, QFont::Weight& weight);
/** Convert QFont::Weight to an arg value (0-8) */
int weightToArg(const QFont::Weight weight);

/** Load dash specific application fonts */
bool loadFonts();
/** Check if the fonts have been loaded successfully */
bool fontsLoaded();

/** Set an application wide default font, depends on the selected theme */
void setApplicationFont();

/** Workaround to set correct font styles in all themes since there is a bug in macOS which leads to
    issues loading variations of montserrat in css it also keeps track of the set fonts to update on
    theme changes. */
void setFont(const std::vector<QWidget*>& vecWidgets, FontWeight weight, int nPointSize = -1, bool fItalic = false);

/** Update the font of all widgets where a custom font has been set with
    GUIUtil::setFont */
void updateFonts();

/** Get a properly weighted QFont object with the selected font. */
QFont getFont(const QString& font_name, QFont::Weight weight, bool italic = false, int point_sz = -1);
QFont getFont(QFont::Weight qWeight, bool fItalic = false, int nPointSize = -1);
QFont getFont(FontWeight weight, bool fItalic = false, int nPointSize = -1);

/** Get the default normal QFont */
QFont getFontNormal();

/** Get the default bold QFont */
QFont getFontBold();
} // namespace GUIUtil

#endif // BITCOIN_QT_GUIUTIL_FONT_H
