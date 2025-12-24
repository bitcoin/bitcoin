// Copyright (c) 2020-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/forms/ui_appearancewidget.h>

#include <qt/appearancewidget.h>
#include <qt/optionsmodel.h>

#include <util/system.h>

#include <QComboBox>
#include <QDataWidgetMapper>
#include <QSettings>
#include <QSlider>

AppearanceWidget::AppearanceWidget(QWidget* parent) :
    QWidget(parent),
    ui{new Ui::AppearanceWidget()},
    prevTheme{GUIUtil::getActiveTheme()},
    prevScale{GUIUtil::g_font_registry.GetFontScale()},
    prevFontFamily{GUIUtil::g_font_registry.GetFont()},
    prevWeightNormal{GUIUtil::g_font_registry.GetWeightNormal()},
    prevWeightBold{GUIUtil::g_font_registry.GetWeightBold()}
{
    ui->setupUi(this);

    for (const QString& entry : GUIUtil::listThemes()) {
        ui->theme->addItem(entry, QVariant(entry));
    }

    for (size_t idx{0}; idx < GUIUtil::g_fonts_known.size(); idx++) {
        ui->fontFamily->addItem(GUIUtil::g_fonts_known[idx], QVariant((uint16_t)idx));
    }

    updateWeightSlider();

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);

    connect(ui->theme, &QComboBox::currentTextChanged, this, &AppearanceWidget::updateTheme);
    connect(ui->fontFamily, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AppearanceWidget::updateFontFamily);
    connect(ui->fontScaleSlider, &QSlider::valueChanged, this, &AppearanceWidget::updateFontScale);
    connect(ui->fontWeightNormalSlider, &QSlider::valueChanged, [this](auto nValue) { updateFontWeightNormal(nValue); });
    connect(ui->fontWeightBoldSlider, &QSlider::valueChanged, [this](auto nValue) { updateFontWeightBold(nValue); });

    connect(ui->theme, &QComboBox::currentTextChanged, [this]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontFamily, &QComboBox::currentTextChanged, [this]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontScaleSlider, &QSlider::sliderReleased, [this]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontWeightNormalSlider, &QSlider::sliderReleased, [this]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontWeightBoldSlider, &QSlider::sliderReleased, [this]() { Q_EMIT appearanceChanged(); });
}

AppearanceWidget::~AppearanceWidget()
{
    if (fAcceptChanges) {
        mapper->submit();
    } else {
        if (prevTheme != GUIUtil::getActiveTheme()) {
            updateTheme(prevTheme);
        }
        if (prevFontFamily != GUIUtil::g_font_registry.GetFont()) {
            const bool setfont_ret{GUIUtil::g_font_registry.SetFont(prevFontFamily)};
            assert(setfont_ret);
            GUIUtil::setApplicationFont();
        }
        if (prevScale != GUIUtil::g_font_registry.GetFontScale()) {
            GUIUtil::g_font_registry.SetFontScale(prevScale);
        }
        if (prevWeightNormal != GUIUtil::g_font_registry.GetWeightNormal()) {
            GUIUtil::g_font_registry.SetWeightNormal(prevWeightNormal);
        }
        if (prevWeightBold != GUIUtil::g_font_registry.GetWeightBold()) {
            GUIUtil::g_font_registry.SetWeightBold(prevWeightBold);
        }
        GUIUtil::updateFonts();
    }
    delete ui;
}

void AppearanceWidget::setModel(OptionsModel* _model)
{
    this->model = _model;

    if (_model) {
        mapper->setModel(_model);
        mapper->addMapping(ui->theme, OptionsModel::Theme);
        mapper->addMapping(ui->fontFamily, OptionsModel::FontFamily);
        mapper->addMapping(ui->fontScaleSlider, OptionsModel::FontScale);
        mapper->addMapping(ui->fontWeightNormalSlider, OptionsModel::FontWeightNormal);
        mapper->addMapping(ui->fontWeightBoldSlider, OptionsModel::FontWeightBold);

        const QSignalBlocker fontFamilyBlocker(ui->fontFamily);
        const QSignalBlocker fontScaleBlocker(ui->fontScaleSlider);
        const QSignalBlocker fontWeightNormalBlocker(ui->fontWeightNormalSlider);
        const QSignalBlocker fontWeightBoldBlocker(ui->fontWeightBoldSlider);

        mapper->toFirst();
    }
}

void AppearanceWidget::accept()
{
    fAcceptChanges = true;
}

void AppearanceWidget::updateTheme(const QString& theme)
{
    QString newValue = theme.isEmpty() ? ui->theme->currentData().toString() : theme;
    if (GUIUtil::getActiveTheme() != newValue) {
        QSettings().setValue("theme", newValue);
        // Force loading the theme
        if (model) {
            GUIUtil::loadTheme(true);
        }
    }
}

void AppearanceWidget::updateFontFamily(int index)
{
    const bool setfont_ret{GUIUtil::g_font_registry.SetFont(GUIUtil::g_fonts_known[ui->fontFamily->itemData(index).toInt()])};
    assert(setfont_ret);
    GUIUtil::setApplicationFont();
    GUIUtil::updateFonts();
    updateWeightSlider(true);
}

void AppearanceWidget::updateFontScale(int nScale)
{
    GUIUtil::g_font_registry.SetFontScale(nScale);
    GUIUtil::updateFonts();
}

void AppearanceWidget::updateFontWeightNormal(int nValue, bool fForce)
{
    int nSliderValue = nValue;
    if (nValue > ui->fontWeightBoldSlider->value() && !fForce) {
        nSliderValue = ui->fontWeightBoldSlider->value();
    }
    const QSignalBlocker blocker(ui->fontWeightNormalSlider);
    ui->fontWeightNormalSlider->setValue(nSliderValue);
    GUIUtil::g_font_registry.SetWeightNormal(GUIUtil::g_font_registry.IdxToWeight(ui->fontWeightNormalSlider->value()));
    GUIUtil::updateFonts();
}

void AppearanceWidget::updateFontWeightBold(int nValue, bool fForce)
{
    int nSliderValue = nValue;
    if (nValue < ui->fontWeightNormalSlider->value() && !fForce) {
        nSliderValue = ui->fontWeightNormalSlider->value();
    }
    const QSignalBlocker blocker(ui->fontWeightBoldSlider);
    ui->fontWeightBoldSlider->setValue(nSliderValue);
    GUIUtil::g_font_registry.SetWeightBold(GUIUtil::g_font_registry.IdxToWeight(ui->fontWeightBoldSlider->value()));
    GUIUtil::updateFonts();
}

void AppearanceWidget::updateWeightSlider(const bool fForce)
{
    int nMaximum = GUIUtil::g_font_registry.GetSupportedWeights().size() - 1;

    ui->fontWeightNormalSlider->setMinimum(0);
    ui->fontWeightNormalSlider->setMaximum(nMaximum);

    ui->fontWeightBoldSlider->setMinimum(0);
    ui->fontWeightBoldSlider->setMaximum(nMaximum);

    if (fForce || !GUIUtil::g_font_registry.IsValidWeight(prevWeightNormal) || !GUIUtil::g_font_registry.IsValidWeight(prevWeightBold)) {
        int nIndexNormal = GUIUtil::g_font_registry.WeightToIdx(GUIUtil::g_font_registry.GetWeightNormalDefault());
        int nIndexBold = GUIUtil::g_font_registry.WeightToIdx(GUIUtil::g_font_registry.GetWeightBoldDefault());
        assert(nIndexNormal != -1 && nIndexBold != -1);
        updateFontWeightNormal(nIndexNormal, true);
        updateFontWeightBold(nIndexBold, true);
    }
}
