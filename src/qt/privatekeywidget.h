#ifndef PRIVATEKEYWIDGET_H
#define PRIVATEKEYWIDGET_H

#include <QWidget>

class QHBoxLayout;
class QLineEdit;
class QPushButton;

class PrivateKeyWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PrivateKeyWidget(QString privKey, QWidget *parent = 0);
private:
    QHBoxLayout *layout;
    QLineEdit *lineEdit;
    QPushButton *show;
    QPushButton *copy;

signals:

public slots:
    void showClicked(bool show_hide);
    void copyClicked();
};

#endif // PRIVATEKEYWIDGET_H
