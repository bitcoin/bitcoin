#include "bitcoinamountfield.h"

#include <QLabel>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QHBoxLayout>
#include <QKeyEvent>

BitcoinAmountField::BitcoinAmountField(QWidget *parent):
        QWidget(parent), amount(0), decimals(0)
{
    amount = new QLineEdit(this);
    amount->setValidator(new QRegExpValidator(QRegExp("[0-9]+"), this));
    amount->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    amount->installEventFilter(this);
    amount->setMaximumWidth(80);
    decimals = new QLineEdit(this);
    decimals->setValidator(new QRegExpValidator(QRegExp("[0-9]+"), this));
    decimals->setMaxLength(8);
    decimals->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    decimals->setMaximumWidth(75);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->addWidget(amount);
    layout->addWidget(new QLabel(QString(".")));
    layout->addWidget(decimals);
    layout->addStretch(1);
    layout->setContentsMargins(0,0,0,0);

    setFocusPolicy(Qt::TabFocus);
    setLayout(layout);
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
}

QString BitcoinAmountField::text() const
{
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
        }
    }
    return false;
}
