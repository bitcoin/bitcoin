#include "bitcoinamountfield.h"
#include "qvalidatedlineedit.h"
#include "qvaluecombobox.h"
#include "bitcoinunits.h"

#include <QLabel>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QComboBox>

BitcoinAmountField::BitcoinAmountField(QWidget *parent):
        QWidget(parent), amount(0), decimals(0), currentUnit(-1)
{
    amount = new QValidatedLineEdit(this);
    amount->setValidator(new QRegExpValidator(QRegExp("[0-9]*"), this));
    amount->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    amount->installEventFilter(this);
    amount->setMaximumWidth(75);
    decimals = new QValidatedLineEdit(this);
    decimals->setValidator(new QRegExpValidator(QRegExp("[0-9]+"), this));
    decimals->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    decimals->setMaximumWidth(75);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->addWidget(amount);
    layout->addWidget(new QLabel(QString("<b>.</b>")));
    layout->addWidget(decimals);
    unit = new QValueComboBox(this);
    unit->setModel(new BitcoinUnits(this));
    layout->addWidget(unit);
    layout->addStretch(1);
    layout->setContentsMargins(0,0,0,0);

    setLayout(layout);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(amount);

    // If one if the widgets changes, the combined content changes as well
    connect(amount, SIGNAL(textChanged(QString)), this, SIGNAL(textChanged()));
    connect(decimals, SIGNAL(textChanged(QString)), this, SIGNAL(textChanged()));
    connect(unit, SIGNAL(currentIndexChanged(int)), this, SLOT(unitChanged(int)));

    // Set default based on configuration
    unitChanged(unit->currentIndex());
}

void BitcoinAmountField::setText(const QString &text)
{
    const QStringList parts = text.split(QString("."));
    if(parts.size() == 2)
    {
        amount->setText(parts[0]);
        decimals->setText(parts[1]);
    }
    else
    {
        amount->setText(QString());
        decimals->setText(QString());
    }
}

void BitcoinAmountField::clear()
{
    amount->clear();
    decimals->clear();
    unit->setCurrentIndex(0);
}

bool BitcoinAmountField::validate()
{
    bool valid = true;
    if(decimals->text().isEmpty())
    {
        decimals->setValid(false);
        valid = false;
    }
    if(!BitcoinUnits::parse(currentUnit, text(), 0))
    {
        setValid(false);
        valid = false;
    }

    return valid;
}

void BitcoinAmountField::setValid(bool valid)
{
    amount->setValid(valid);
    decimals->setValid(valid);
}

QString BitcoinAmountField::text() const
{
    if(decimals->text().isEmpty() && amount->text().isEmpty())
    {
        return QString();
    }
    return amount->text() + QString(".") + decimals->text();
}

// Intercept '.' and ',' keys, if pressed focus a specified widget
bool BitcoinAmountField::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);
    if(event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Period || keyEvent->key() == Qt::Key_Comma)
        {
            decimals->setFocus();
            decimals->selectAll();
        }
    }
    return false;
}

QWidget *BitcoinAmountField::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, amount);
    QWidget::setTabOrder(amount, decimals);
    return decimals;
}

qint64 BitcoinAmountField::value(bool *valid_out) const
{
    qint64 val_out = 0;
    bool valid = BitcoinUnits::parse(currentUnit, text(), &val_out);
    if(valid_out)
    {
        *valid_out = valid;
    }
    return val_out;
}

void BitcoinAmountField::setValue(qint64 value)
{
    setText(BitcoinUnits::format(currentUnit, value));
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
    amount->setMaxLength(BitcoinUnits::amountDigits(currentUnit));
    decimals->setMaxLength(BitcoinUnits::decimals(currentUnit));

    if(valid)
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
