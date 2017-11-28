#ifndef PRIVATEKEYWIDGET_H
#define PRIVATEKEYWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QToolTip>

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
