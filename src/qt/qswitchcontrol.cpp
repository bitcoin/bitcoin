#include "qswitchcontrol.h"

#include <QPushButton>
#include <QPropertyAnimation>
#include <QStyleOption>
#include <QPainter>

static const QSize FrameSize = QSize(68, 30);
static const QSize SwitchSize = QSize (26, 26);
static const int SwitchOffset = (FrameSize.height() - SwitchSize.height()) / 2;

static const QString CustomFrameOnStlye = QString("QAbstractButton { border: none; border-radius: %1; background-color: #4697d9;}").arg(FrameSize.height() / 2);
static const QString CustomFrameOffStlye = QString("QAbstractButton { border: none; border-radius: %1; background-color: #a09f9d;}").arg(FrameSize.height() / 2);
static const QString CustomButtonStlye = QString("QPushButton { min-width: 0em; border-radius: %1; background-color: white;}").arg(SwitchSize.height() / 2);

QSwitchControl::QSwitchControl(QWidget *parent):
    QAbstractButton(parent)
{
    this->setFixedSize(FrameSize);

    pbSwitch = new QPushButton(this);
    pbSwitch->setFixedSize(SwitchSize);
    pbSwitch->setStyleSheet(CustomButtonStlye);

    animation = new QPropertyAnimation(pbSwitch, "geometry", this);
    animation->setDuration(200);

    connect(this, &QSwitchControl::mouseClicked, this, &QSwitchControl::onStatusChanged);
    connect(pbSwitch, &QPushButton::clicked, this, &QSwitchControl::onStatusChanged);
    setCheckable(true);
    setChecked(false);
}

void QSwitchControl::setChecked(bool checked)
{
    if(checked)
    {
        pbSwitch->move(this->width() - pbSwitch->width() - SwitchOffset, this->y() + SwitchOffset);
        this->setStyleSheet(CustomFrameOnStlye);
    }
    else
    {
        pbSwitch->move(this->x() + SwitchOffset, this->y() + SwitchOffset);
        this->setStyleSheet(CustomFrameOffStlye);
    }

    QAbstractButton::setChecked(checked);
}

void QSwitchControl::onStatusChanged()
{
    bool checked = !isChecked();

    QRect currentGeometry(pbSwitch->x(), pbSwitch->y(), pbSwitch->width(), pbSwitch->height());

    if(animation->state() == QAbstractAnimation::Running)
        animation->stop();

    if(checked)
    {
        this->setStyleSheet(CustomFrameOnStlye);

        animation->setStartValue(currentGeometry);
        animation->setEndValue(QRect(this->width() - pbSwitch->width() - SwitchOffset, pbSwitch->y(), pbSwitch->width(), pbSwitch->height()));
    }
    else
    {
        this->setStyleSheet(CustomFrameOffStlye);

        animation->setStartValue(currentGeometry);
        animation->setEndValue(QRect(SwitchOffset, pbSwitch->y(), pbSwitch->width(), pbSwitch->height()));
    }
    animation->start();

    setChecked(checked);
    Q_EMIT clicked(checked);
}

void QSwitchControl::mousePressEvent(QMouseEvent *)
{
    Q_EMIT mouseClicked();
}

void QSwitchControl::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
