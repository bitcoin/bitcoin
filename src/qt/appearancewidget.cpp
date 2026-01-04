// Copyright (c) 2020-2026 The Dash Core developers
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
#include <QFontDialog>
#include <QFontInfo>
#include <QSettings>
#include <QSlider>

int setFontChoice(QComboBox* cb, const OptionsModel::FontChoice& fc)
{
    int i;
    for (i = cb->count(); --i >= 0; ) {
        QVariant item_data = cb->itemData(i);
        if (!item_data.canConvert<OptionsModel::FontChoice>()) continue;
        if (item_data.value<OptionsModel::FontChoice>() == fc) {
            break;
        }
    }
    if (i == -1) {
        // New item needed
        QFont chosen_font = OptionsModel::getFontForChoice(fc);
        QSignalBlocker block_currentindexchanged_signal(cb);  // avoid triggering QFontDialog
        cb->insertItem(0, QFontInfo(chosen_font).family(), QVariant::fromValue(fc));
        i = 0;
    }

    cb->setCurrentIndex(i);
    return i;
}

void setupFontOptions(QComboBox* cb)
{
    QFont embedded_font{GUIUtil::fixedPitchFont(true)};
    QFont system_font{GUIUtil::fixedPitchFont(false)};
    cb->addItem(QObject::tr("Default monospace font \"%1\"").arg(QFontInfo(system_font).family()), QVariant::fromValue(OptionsModel::FontChoice{OptionsModel::FontChoiceAbstract::BestSystemFont}));
    cb->addItem(QObject::tr("Embedded \"%1\"").arg(QFontInfo(embedded_font).family()), QVariant::fromValue(OptionsModel::FontChoice{OptionsModel::FontChoiceAbstract::EmbeddedFont}));
    cb->addItem(QObject::tr("Use existing font"), QVariant::fromValue(OptionsModel::FontChoice{OptionsModel::FontChoiceAbstract::ApplicationFont}));
    cb->addItem(QObject::tr("Custom…"));

    const auto& on_font_choice_changed = [cb](int index) {
        static int previous_index = -1;
        QVariant item_data = cb->itemData(index);
        if (item_data.canConvert<OptionsModel::FontChoice>()) {
            // Valid predefined choice, nothing to do
        } else {
            // "Custom..." was selected, show font dialog
            bool ok;
            QFont f = QFontDialog::getFont(&ok, GUIUtil::fixedPitchFont(false), cb->parentWidget());
            if (!ok) {
                cb->setCurrentIndex(previous_index);
                return;
            }
            index = setFontChoice(cb, OptionsModel::FontChoice{f});
        }
        previous_index = index;
    };
    QObject::connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), on_font_choice_changed);
    on_font_choice_changed(cb->currentIndex());
}

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
        const auto& [font, selectable] = GUIUtil::g_fonts_known[idx];
        if (selectable) { ui->fontFamily->addItem(font, QVariant((uint16_t)idx)); }
    }

    updateWeightSlider();

    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);

    setupFontOptions(ui->moneyFont);

    connect(ui->fontFamily, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AppearanceWidget::updateFontFamily);
    connect(ui->fontFamily, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() { Q_EMIT appearanceChanged(); });

    connect(ui->fontScaleSlider, &QSlider::sliderReleased, [this]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontScaleSlider, &QSlider::valueChanged, this, &AppearanceWidget::updateFontScale);

    connect(ui->fontWeightBoldSlider, &QSlider::sliderReleased, [this]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontWeightBoldSlider, &QSlider::valueChanged, [this](auto nValue) { updateFontWeightBold(nValue); });

    connect(ui->fontWeightNormalSlider, &QSlider::sliderReleased, [this]() { Q_EMIT appearanceChanged(); });
    connect(ui->fontWeightNormalSlider, &QSlider::valueChanged, [this](auto nValue) { updateFontWeightNormal(nValue); });

    connect(ui->moneyFont, &QComboBox::currentTextChanged, [this]() { Q_EMIT appearanceChanged(); });
    connect(ui->moneyFont, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AppearanceWidget::updateMoneyFont);

    connect(ui->theme, &QComboBox::currentTextChanged, this, &AppearanceWidget::updateTheme);
    connect(ui->theme, &QComboBox::currentTextChanged, [this]() { Q_EMIT appearanceChanged(); });
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
        // Restore monospace font if cancelled
        if (model) {
            const auto& current_money_font = model->data(model->index(OptionsModel::FontForMoney, 0), Qt::EditRole).value<OptionsModel::FontChoice>();
            if (current_money_font != prevMoneyFont) {
                model->setData(model->index(OptionsModel::FontForMoney, 0), QVariant::fromValue(prevMoneyFont));
            }
        }
        GUIUtil::setApplicationFont();
        GUIUtil::updateFonts();
    }
    delete ui;
}

void AppearanceWidget::setModel(OptionsModel* _model)
{
    this->model = _model;
    if (!_model) return;

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

    const auto& font_for_money = _model->data(_model->index(OptionsModel::FontForMoney, 0), Qt::EditRole).value<OptionsModel::FontChoice>();
    prevMoneyFont = font_for_money;  // Save original value for cancel
    setFontChoice(ui->moneyFont, font_for_money);

    const bool override_family{_model->isOptionOverridden("-font-family")};
    if (override_family) {
        ui->fontFamily->setEnabled(false);
        if (const auto idx{ui->fontFamily->findText(GUIUtil::g_font_registry.GetFont())}; idx != -1) {
            ui->fontFamily->setCurrentIndex(idx);
        }
    }

    if (_model->isOptionOverridden("-font-scale")) {
        ui->fontScaleSlider->setEnabled(false);
        ui->fontScaleSlider->setValue(GUIUtil::g_font_registry.GetFontScale());
    }

    if (bool is_overridden{_model->isOptionOverridden("-font-weight-normal")}; is_overridden || override_family) {
        if (is_overridden) {
            ui->fontWeightNormalSlider->setEnabled(false);
        }
        if (const auto idx{GUIUtil::g_font_registry.WeightToIdx(GUIUtil::g_font_registry.GetWeightNormal())}; idx != -1) {
            ui->fontWeightNormalSlider->setValue(idx);
        }
    }

    if (bool is_overridden{_model->isOptionOverridden("-font-weight-bold")}; is_overridden || override_family) {
        if (is_overridden) {
            ui->fontWeightBoldSlider->setEnabled(false);
        }
        if (const auto idx{GUIUtil::g_font_registry.WeightToIdx(GUIUtil::g_font_registry.GetWeightBold())}; idx != -1) {
            ui->fontWeightBoldSlider->setValue(idx);
        }
    }
}

void AppearanceWidget::accept()
{
    fAcceptChanges = true;
    // Note: FontForMoney is now updated immediately via updateMoneyFont()
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
    const bool setfont_ret{GUIUtil::g_font_registry.SetFont(GUIUtil::g_fonts_known[ui->fontFamily->itemData(index).toInt()].first)};
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
    GUIUtil::setApplicationFont();
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
    GUIUtil::setApplicationFont();
    GUIUtil::updateFonts();
}

void AppearanceWidget::updateMoneyFont(int index)
{
    if (!model) {
        return;
    }
    QVariant item_data = ui->moneyFont->itemData(index);
    if (!item_data.canConvert<OptionsModel::FontChoice>()) {
        return;
    }
    // Update the model immediately to trigger live preview in Overview page
    model->setData(model->index(OptionsModel::FontForMoney, 0), item_data);
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
