#ifndef QSWITCHCONTROL_H
#define QSWITCHCONTROL_H

#include <QAbstractButton>

class QPushButton;
class QPropertyAnimation;

class QSwitchControl : public QAbstractButton
{
    Q_OBJECT
public:
    QSwitchControl(QWidget *parent = nullptr);

public Q_SLOTS:
    void setChecked(bool);
    void onStatusChanged();

Q_SIGNALS:
    void mouseClicked();

protected:
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    QPushButton *pbSwitch;
    QPropertyAnimation *animation;
};

#endif // QSWITCHCONTROL_H
