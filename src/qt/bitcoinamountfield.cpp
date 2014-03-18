// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinamountfield.h"

#include "bitcoinunits.h"
#include "guiconstants.h"
#include "qvaluecombobox.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <qmath.h> // for qPow()

/** QDoubleSpinBox that shows number group seperators.
 * In Qt 5.3+ this could be replaced with QAbstractSpinBox::setGroupSeparatorShown(true)
 * See https://bugreports.qt-project.org/browse/QTBUG-5142
 *
 * TODO: We should not use a QDoubleSpinBox at all but implement our own
 * spinbox for fixed-point numbers.
 */
class AmountSpinBox: public QDoubleSpinBox
{
public:
    explicit AmountSpinBox(QWidget *parent):
        QDoubleSpinBox(parent)
    {
    }
    QString textFromValue(double value) const
    {
        return QLocale().toString(value, 'f', decimals());
    }
    QValidator::State validate (QString &text, int &pos) const
    {
        bool ok = false;
        QValidator::State rv = QDoubleSpinBox::validate(text, pos);
        if (rv == QValidator::Acceptable)
        {
            // Make sure that we only return acceptable if group seperators
            // are in the right place. If not, a fixup step is needed first so
            // return Intermediate.
            QLocale().toDouble(text, &ok);
            if (!ok)
                return QValidator::Intermediate;
        }
        return rv;
    }

    double valueFromText(const QString& text) const
    {
        return QLocale().toDouble(text);
    }
};

BitcoinAmountField::BitcoinAmountField(QWidget *parent) :
    QWidget(parent),
    amount(0),
    currentUnit(-1)
{
    nSingleStep = 100000; // satoshis

    amount = new AmountSpinBox(this);
    amount->installEventFilter(this);
    amount->setMaximumWidth(170);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(amount);
    unit = new QValueComboBox(this);
    unit->setModel(new BitcoinUnits(this));
    layout->addWidget(unit);
    layout->addStretch(1);
    layout->setContentsMargins(0,0,0,0);

    setLayout(layout);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(amount);

    // If one if the widgets changes, the combined content changes as well
    connect(amount, SIGNAL(valueChanged(QString)), this, SIGNAL(textChanged()));
    connect(unit, SIGNAL(currentIndexChanged(int)), this, SLOT(unitChanged(int)));

    // Set default based on configuration
    unitChanged(unit->currentIndex());
}

void BitcoinAmountField::setText(const QString &text)
{
    if (text.isEmpty())
        amount->clear();
    else
        amount->setValue(QLocale().toDouble(text));
}

void BitcoinAmountField::clear()
{
    amount->clear();
    unit->setCurrentIndex(0);
}

bool BitcoinAmountField::validate()
{
    bool valid = true;
    if (amount->value() == 0.0)
        valid = false;
    else if (!BitcoinUnits::parse(currentUnit, text(), 0))
        valid = false;
    else if (amount->value() > BitcoinUnits::maxAmount(currentUnit))
        valid = false;

    setValid(valid);

    return valid;
}

void BitcoinAmountField::setValid(bool valid)
{
    if (valid)
        amount->setStyleSheet("");
    else
        amount->setStyleSheet(STYLE_INVALID);
}

QString BitcoinAmountField::text() const
{
    if (amount->text().isEmpty())
        return QString();
    else
        return amount->text();
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
    QWidget::setTabOrder(amount, unit);
    return unit;
}

qint64 BitcoinAmountField::value(bool *valid_out) const
{
    qint64 val_out = 0;
    bool valid = BitcoinUnits::parse(currentUnit, text(), &val_out);
    if (valid_out)
    {
        *valid_out = valid;
    }
    return val_out;
}

void BitcoinAmountField::setValue(qint64 value)
{
    setText(BitcoinUnits::format(currentUnit, value));
}

void BitcoinAmountField::setReadOnly(bool fReadOnly)
{
    amount->setReadOnly(fReadOnly);
    unit->setEnabled(!fReadOnly);
}

void BitcoinAmountField::unitChanged(int idx)
{
    // Use description tooltip for current unit for the combobox
    unit->setToolTip(unit->itemData(idx, Qt::ToolTipRole).toString());

    // Determine new unit ID
    int newUnit = unit->itemData(idx, BitcoinUnits::UnitRole).toInt();

    // Parse current value and convert to new unit
    bool valid = false;
    qint64 currentValue = value(&valid);

    currentUnit = newUnit;

    // Set max length after retrieving the value, to prevent truncation
    amount->setDecimals(BitcoinUnits::decimals(currentUnit));
    amount->setMaximum(qPow(10, BitcoinUnits::amountDigits(currentUnit)) - qPow(10, -amount->decimals()));
    amount->setSingleStep((double)nSingleStep / (double)BitcoinUnits::factor(currentUnit));

    if (valid)
    {
        // If value was valid, re-place it in the widget with the new unit
        setValue(currentValue);
    }
    else
    {
        // If current value is invalid, just clear field
        setText("");
    }
    setValid(true);
}

void BitcoinAmountField::setDisplayUnit(int newUnit)
{
    unit->setValue(newUnit);
}

void BitcoinAmountField::setSingleStep(qint64 step)
{
    nSingleStep = step;
    unitChanged(unit->currentIndex());
}
