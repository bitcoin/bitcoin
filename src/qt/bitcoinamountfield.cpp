#include "bitcoinamountfield.h"
#include "qvalidatedlineedit.h"

#include <QLabel>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QHBoxLayout>
#include <QKeyEvent>

BitcoinAmountField::BitcoinAmountField(QWidget *parent):
        QWidget(parent), amount(0), decimals(0)
{
    amount = new QValidatedLineEdit(this);
    amount->setValidator(new QRegExpValidator(QRegExp("[0-9]?"), this));
    amount->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    amount->installEventFilter(this);
    amount->setMaximumWidth(100);
    decimals = new QValidatedLineEdit(this);
    decimals->setValidator(new QRegExpValidator(QRegExp("[0-9]+"), this));
    decimals->setMaxLength(8);
    decimals->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    decimals->setMaximumWidth(75);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->addWidget(amount);
    layout->addWidget(new QLabel(QString(".")));
    layout->addWidget(decimals);
    layout->addWidget(new QLabel(QString(" BTC")));
    layout->addStretch(1);
    layout->setContentsMargins(0,0,0,0);

    setLayout(layout);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(amount);

    // If one if the widgets changes, the combined content changes as well
    connect(amount, SIGNAL(textChanged(QString)), this, SIGNAL(textChanged()));
    connect(decimals, SIGNAL(textChanged(QString)), this, SIGNAL(textChanged()));
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
}

bool BitcoinAmountField::validate()
{
    bool valid = true;
    if(decimals->text().isEmpty())
    {
        decimals->setValid(false);
        valid = false;
    }
    return valid;
}

QString BitcoinAmountField::text() const
{
    if(decimals->text().isEmpty())
    {
        return QString();
    }
    if(amount->text().isEmpty())
    {
        return QString("0.") + decimals->text();
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
