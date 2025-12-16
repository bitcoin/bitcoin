// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GUIUTIL_FONT_H
#define BITCOIN_QT_GUIUTIL_FONT_H

#include <QFont>
#include <QString>
#include <QWidget>

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

FontFamily fontFamilyFromString(const QString& strFamily);
QString fontFamilyToString(FontFamily family);

/** set/get font family: GUIUtil::fontFamily */
FontFamily getFontFamilyDefault();
FontFamily getFontFamily();
void setFontFamily(FontFamily family);

/** Convert weight value from args (0-8) to QFont::Weight */
bool weightFromArg(int nArg, QFont::Weight& weight);
/** Convert QFont::Weight to an arg value (0-8) */
int weightToArg(const QFont::Weight weight);
/** Convert GUIUtil::FontWeight to QFont::Weight */
QFont::Weight toQFontWeight(FontWeight weight);

/** set/get normal font weight: GUIUtil::fontWeightNormal */
QFont::Weight getFontWeightNormalDefault();
QFont::Weight getFontWeightNormal();
void setFontWeightNormal(QFont::Weight weight);

/** set/get bold font weight: GUIUtil::fontWeightBold */
QFont::Weight getFontWeightBoldDefault();
QFont::Weight getFontWeightBold();
void setFontWeightBold(QFont::Weight weight);

/** set/get font scale: GUIUtil::fontScale */
int getFontScaleDefault();
int getFontScale();
void setFontScale(int nScale);

/** get font size with GUIUtil::fontScale applied */
double getScaledFontSize(int nSize);

/** Load dash specific appliciation fonts */
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

/** Return supported normal default for the current font family */
QFont::Weight getSupportedFontWeightNormalDefault();
/** Return supported bold default for the current font family */
QFont::Weight getSupportedFontWeightBoldDefault();
/** Return supported weights for the current font family */
std::vector<QFont::Weight> getSupportedWeights();
/** Convert an index to a weight in the supported weights vector */
QFont::Weight supportedWeightFromIndex(int nIndex);
/** Convert a weight to an index in the supported weights vector */
int supportedWeightToIndex(QFont::Weight weight);
/** Check if a weight is supported by the current font family */
bool isSupportedWeight(QFont::Weight weight);
} // namespace GUIUtil

#endif // BITCOIN_QT_GUIUTIL_FONT_H
