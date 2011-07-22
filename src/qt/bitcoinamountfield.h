#ifndef BITCOINFIELD_H
#define BITCOINFIELD_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QValidatedLineEdit;
QT_END_NAMESPACE

// Coin amount entry widget with separate parts for whole
// coins and decimals.
class BitcoinAmountField: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged USER true);
public:
    explicit BitcoinAmountField(QWidget *parent = 0);

    void setText(const QString &text);
    QString text() const;

    void clear();
    bool validate();
    // Qt messes up the tab chain by default in some cases (issue http://bugreports.qt.nokia.com/browse/QTBUG-10907)
    // Hence we have to set it up manually
    QWidget *setupTabChain(QWidget *prev);

signals:
    void textChanged();

protected:
    // Intercept '.' and ',' keys, if pressed focus a specified widget
    bool eventFilter(QObject *object, QEvent *event);

private:
    QValidatedLineEdit *amount;
    QValidatedLineEdit *decimals;
};


#endif // BITCOINFIELD_H
