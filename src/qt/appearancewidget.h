// Copyright (c) 2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_QT_APPEARANCE_WIDGET_H
#define DASH_QT_APPEARANCE_WIDGET_H

#include <QWidget>

#include <qt/guiutil.h>

namespace Ui {
class AppearanceWidget;
}

class OptionsModel;

class QDataWidgetMapper;
class QSlider;
class QComboBox;

class AppearanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AppearanceWidget(QWidget* parent = 0);
    ~AppearanceWidget();

    void setModel(OptionsModel* model);

public Q_SLOTS:
    void accept();

private Q_SLOTS:
    void updateTheme(const QString& toTheme = QString());
    void updateFontFamily(int index);
    void updateFontScale(int nScale);
    void updateFontWeightNormal(int nValue, bool fForce = false);
    void updateFontWeightBold(int nValue, bool fForce = false);

private:
    Ui::AppearanceWidget* ui;
    QDataWidgetMapper* mapper;
    OptionsModel* model;
    bool fAcceptChanges;
    QString prevTheme;
    int prevScale;
    GUIUtil::FontFamily prevFontFamily;
    QFont::Weight prevWeightNormal;
    QFont::Weight prevWeightBold;

    void updateWeightSlider();
};

#endif // DASH_QT_APPEARANCE_WIDGET_H
