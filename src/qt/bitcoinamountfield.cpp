// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoinamountfield.h>

#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QVariant>

#include <cassert>

/**
 * Parse a string into a number of base monetary units and
 * return validity.
 * @note Must return 0 if !valid.
 */
static CAmount parse(const QString &text, BitcoinUnit nUnit, bool *valid_out= nullptr)
{
    CAmount val = 0;
    bool valid = BitcoinUnits::parse(nUnit, text, &val);
    if(valid)
    {
        if(val < 0 || val > BitcoinUnits::maxMoney())
            valid = false;
    }
    if(valid_out)
        *valid_out = valid;
    return valid ? val : 0;
}

/** Amount widget validator, checks for valid CAmount value.
 */
class AmountValidator : public QValidator
{
    Q_OBJECT
    BitcoinUnit currentUnit{BitcoinUnit::DASH};

public:
    explicit AmountValidator(QObject *parent) :
        QValidator(parent)
        {}

    State validate(QString &input, int &pos) const override
    {
        if(input.isEmpty())
            return QValidator::Intermediate;
        bool valid = false;
        parse(input, currentUnit, &valid);
        /* Make sure we return Intermediate so that fixup() is called on defocus */
        return valid ? QValidator::Intermediate : QValidator::Invalid;
    }

    void updateUnit(BitcoinUnit nUnit)
    {
        currentUnit = nUnit;
    }
};

/** QLineEdit that uses fixed-point numbers internally and uses our own
 * formatting/parsing functions.
 */
class AmountLineEdit: public QLineEdit
{
    Q_OBJECT
    AmountValidator* amountValidator;
public:
    explicit AmountLineEdit(QWidget *parent):
        QLineEdit(parent)
    {
        setAlignment(Qt::AlignLeft);
        amountValidator = new AmountValidator(this);
        setValidator(amountValidator);
        connect(this, &QLineEdit::textEdited, this, &AmountLineEdit::valueChanged);
    }

    void fixup(const QString &input)
    {
        bool valid;
        CAmount val;

        if (input.isEmpty() && !m_allow_empty) {
            valid = true;
            val = m_min_amount;
        } else {
            valid = false;
            val = parse(input, currentUnit, &valid);
        }

        if (valid) {
            val = qBound(m_min_amount, val, m_max_amount);
            setText(BitcoinUnits::format(currentUnit, val, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        }
    }

    CAmount value(bool *valid_out=nullptr) const
    {
        return parse(text(), currentUnit, valid_out);
    }

    void setValue(const CAmount& value)
    {
        setText(BitcoinUnits::format(currentUnit, value, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        Q_EMIT valueChanged();
    }

    void SetAllowEmpty(bool allow)
    {
        m_allow_empty = allow;
    }

    void SetMinValue(const CAmount& value)
    {
        m_min_amount = value;
    }

    void SetMaxValue(const CAmount& value)
    {
        m_max_amount = value;
    }

    void setDisplayUnit(BitcoinUnit unit)
    {
        bool valid = false;
        CAmount val = value(&valid);

        currentUnit = unit;
        amountValidator->updateUnit(unit);

        setPlaceholderText(BitcoinUnits::format(currentUnit, m_min_amount, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        if(valid)
            setValue(val);
        else
            clear();
    }

    QSize minimumSizeHint() const override
    {
        ensurePolished();
        const QFontMetrics fm(fontMetrics());
        int h = 0;
        int w = GUIUtil::TextWidth(fm, BitcoinUnits::format(BitcoinUnit::DASH, BitcoinUnits::maxMoney(), false, BitcoinUnits::SeparatorStyle::ALWAYS));
        w += 2; // cursor blinking space
        w += GUIUtil::dashThemeActive() ? 24 : 0; // counteract padding from css
        return QSize(w, h);
    }

private:
    BitcoinUnit currentUnit{BitcoinUnit::DASH};
    bool m_allow_empty{true};
    CAmount m_min_amount{CAmount(0)};
    CAmount m_max_amount{BitcoinUnits::maxMoney()};

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Comma)
            {
                // Translate a comma into a period
                QKeyEvent periodKeyEvent(event->type(), Qt::Key_Period, keyEvent->modifiers(), ".", keyEvent->isAutoRepeat(), keyEvent->count());
                return QLineEdit::event(&periodKeyEvent);
            }
            if(keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
            {
                clearFocus();
            }
        }
        if (event->type() == QEvent::FocusOut)
        {
            fixup(text());
        }
        return QLineEdit::event(event);
    }

Q_SIGNALS:
    void valueChanged();
};

#include <qt/bitcoinamountfield.moc>

BitcoinAmountField::BitcoinAmountField(QWidget *parent) :
    QWidget(parent),
    amount(nullptr)
{
    amount = new AmountLineEdit(this);
    amount->setLocale(QLocale::c());
    amount->installEventFilter(this);
    amount->setMaximumWidth(300);

    units = new BitcoinUnits(this);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(amount);

    setLayout(layout);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(amount);

    // If one if the widgets changes, the combined content changes as well
    connect(amount, &AmountLineEdit::valueChanged, this, &BitcoinAmountField::valueChanged);
}

void BitcoinAmountField::clear()
{
    amount->clear();
}

void BitcoinAmountField::setEnabled(bool fEnabled)
{
    amount->setEnabled(fEnabled);
}

bool BitcoinAmountField::validate()
{
    bool valid = false;
    value(&valid);
    setValid(valid);
    return valid;
}

void BitcoinAmountField::setValid(bool valid)
{
    if (valid)
        amount->setStyleSheet("");
    else
        amount->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_INVALID));
}

bool BitcoinAmountField::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn)
    {
        // Clear invalid flag on focus
        setValid(true);
    }
    return QWidget::eventFilter(object, event);
}

QWidget *BitcoinAmountField::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, amount);
    return amount;
}

CAmount BitcoinAmountField::value(bool *valid_out) const
{
    return amount->value(valid_out);
}

void BitcoinAmountField::setValue(const CAmount& value)
{
    amount->setValue(value);
}

void BitcoinAmountField::SetAllowEmpty(bool allow)
{
    amount->SetAllowEmpty(allow);
}

void BitcoinAmountField::SetMinValue(const CAmount& value)
{
    amount->SetMinValue(value);
}

void BitcoinAmountField::SetMaxValue(const CAmount& value)
{
    amount->SetMaxValue(value);
}

void BitcoinAmountField::setReadOnly(bool fReadOnly)
{
    amount->setReadOnly(fReadOnly);
}

void BitcoinAmountField::unitChanged(int idx)
{
    // Use description tooltip for current unit for the combobox
    amount->setToolTip(units->data(idx, Qt::ToolTipRole).toString());

    // Determine new unit ID
    QVariant new_unit = units->data(idx, BitcoinUnits::UnitRole);
    assert(new_unit.isValid());
    amount->setPlaceholderText(tr("Amount in %1").arg(units->data(idx, Qt::DisplayRole).toString()));
    amount->setDisplayUnit(new_unit.value<BitcoinUnit>());
}

void BitcoinAmountField::setDisplayUnit(BitcoinUnit new_unit)
{
    unitChanged(QVariant::fromValue(new_unit).toInt());
}
