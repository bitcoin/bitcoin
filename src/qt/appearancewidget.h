// Copyright (c) 2020-2026 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_APPEARANCEWIDGET_H
#define BITCOIN_QT_APPEARANCEWIDGET_H

#include <QWidget>

#include <qt/guiutil.h>
#include <qt/guiutil_font.h>
#include <qt/optionsmodel.h>

namespace Ui {
class AppearanceWidget;
}

class QDataWidgetMapper;
class QSlider;
class QComboBox;

class AppearanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AppearanceWidget(QWidget* parent = nullptr);
    ~AppearanceWidget();

    void setModel(OptionsModel* model);

Q_SIGNALS:
    void appearanceChanged();

public Q_SLOTS:
    void accept();

private Q_SLOTS:
    void updateTheme(const QString& toTheme = QString());
    void updateFontFamily(int index);
    void updateFontScale(int nScale);
    void updateFontWeightNormal(int nValue, bool fForce = false);
    void updateFontWeightBold(int nValue, bool fForce = false);
    void updateMoneyFont(int index);

private:
    Ui::AppearanceWidget* ui;
    QDataWidgetMapper* mapper;
    OptionsModel* model;
    bool fAcceptChanges{false};
    QString prevTheme;
    int prevScale;
    QString prevFontFamily;
    QFont::Weight prevWeightNormal;
    QFont::Weight prevWeightBold;
    OptionsModel::FontChoice prevMoneyFont{OptionsModel::FontChoiceAbstract::ApplicationFont};

    void updateWeightSlider(bool fForce = false);
};

#endif // BITCOIN_QT_APPEARANCEWIDGET_H
