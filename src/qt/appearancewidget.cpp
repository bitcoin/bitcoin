// Copyright (c) 2020-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif

#include <qt/forms/ui_appearancewidget.h>

#include <qt/appearancewidget.h>
#include <qt/optionsmodel.h>

#include <util.h>

#include <QComboBox>
#include <QDataWidgetMapper>
#include <QSettings>
#include <QSlider>

AppearanceWidget::AppearanceWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::AppearanceWidget),
    fAcceptChanges(false),
    prevTheme(GUIUtil::getActiveTheme()),
    prevFontFamily(GUIUtil::getFontFamily()),
    prevScale(GUIUtil::getFontScale()),
    prevWeightNormal(GUIUtil::getFontWeightNormal()),
    prevWeightBold(GUIUtil::getFontWeightBold())
{
    ui->setupUi(this);

    for (const QString& entry : GUIUtil::listThemes()) {
        ui->theme->addItem(entry, QVariant(entry));
    }

    GUIUtil::FontFamily fontSystem = GUIUtil::FontFamily::SystemDefault;
    GUIUtil::FontFamily fontMontserrat = GUIUtil::FontFamily::Montserrat;

    ui->fontFamily->addItem(GUIUtil::fontFamilyToString(fontSystem), QVariant(static_cast<int>(fontSystem)));
    ui->fontFamily->addItem(GUIUtil::fontFamilyToString(fontMontserrat), QVariant(static_cast<int>(fontMontserrat)));

    updateWeightSlider();

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);

    connect(ui->theme, SIGNAL(currentTextChanged(const QString&)), this, SLOT(updateTheme(const QString&)));
    connect(ui->fontFamily, SIGNAL(activated(int)), this, SLOT(updateFontFamily(int)));
    connect(ui->fontScaleSlider, SIGNAL(valueChanged(int)), this, SLOT(updateFontScale(int)));
    connect(ui->fontWeightNormalSlider, SIGNAL(valueChanged(int)), this, SLOT(updateFontWeightNormal(int)));
    connect(ui->fontWeightBoldSlider, SIGNAL(valueChanged(int)), this, SLOT(updateFontWeightBold(int)));

    connect(ui->theme, &QComboBox::currentTextChanged, [=]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontFamily, &QComboBox::currentTextChanged, [=]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontScaleSlider, &QSlider::sliderReleased, [=]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontWeightNormalSlider, &QSlider::sliderReleased, [=]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontWeightBoldSlider, &QSlider::sliderReleased, [=]() { Q_EMIT appearanceChanged(); });
}

AppearanceWidget::~AppearanceWidget()
{
    if (fAcceptChanges) {
        mapper->submit();
    } else {
        if (prevTheme != GUIUtil::getActiveTheme()) {
            updateTheme(prevTheme);
        }
        if (prevFontFamily != GUIUtil::getFontFamily()) {
            GUIUtil::setFontFamily(prevFontFamily);
        }
        if (prevScale != GUIUtil::getFontScale()) {
            GUIUtil::setFontScale(prevScale);
        }
        if (prevWeightNormal != GUIUtil::getFontWeightNormal()) {
            GUIUtil::setFontWeightNormal(prevWeightNormal);
        }
        if (prevWeightBold != GUIUtil::getFontWeightBold()) {
            GUIUtil::setFontWeightBold(prevWeightBold);
        }
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
    GUIUtil::setFontFamily(static_cast<GUIUtil::FontFamily>(ui->fontFamily->itemData(index).toInt()));
    updateWeightSlider();
}

void AppearanceWidget::updateFontScale(int nScale)
{
    GUIUtil::setFontScale(nScale);
}

void AppearanceWidget::updateFontWeightNormal(int nValue, bool fForce)
{
    int nSliderValue = nValue;
    if (nValue > ui->fontWeightBoldSlider->value() && !fForce) {
        nSliderValue = ui->fontWeightBoldSlider->value();
    }
    const QSignalBlocker blocker(ui->fontWeightNormalSlider);
    ui->fontWeightNormalSlider->setValue(nSliderValue);
    GUIUtil::setFontWeightNormal(GUIUtil::supportedWeightFromIndex(ui->fontWeightNormalSlider->value()));
}

void AppearanceWidget::updateFontWeightBold(int nValue, bool fForce)
{
    int nSliderValue = nValue;
    if (nValue < ui->fontWeightNormalSlider->value() && !fForce) {
        nSliderValue = ui->fontWeightNormalSlider->value();
    }
    const QSignalBlocker blocker(ui->fontWeightBoldSlider);
    ui->fontWeightBoldSlider->setValue(nSliderValue);
    GUIUtil::setFontWeightBold(GUIUtil::supportedWeightFromIndex(ui->fontWeightBoldSlider->value()));
}

void AppearanceWidget::updateWeightSlider()
{
    int nMaximum = GUIUtil::getSupportedWeights().size() - 1;

    ui->fontWeightNormalSlider->setMinimum(0);
    ui->fontWeightNormalSlider->setMaximum(nMaximum);

    ui->fontWeightBoldSlider->setMinimum(0);
    ui->fontWeightBoldSlider->setMaximum(nMaximum);

    if (nMaximum < 4) {
        updateFontWeightNormal(0, true);
        updateFontWeightBold(nMaximum, true);
    } else {
        updateFontWeightNormal(1, true);
        updateFontWeightBold(4, true);
    }
}
