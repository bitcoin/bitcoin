#ifndef BITCOINFIELD_H
#define BITCOINFIELD_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

class BitcoinAmountField: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged USER true);
public:
    explicit BitcoinAmountField(QWidget *parent = 0);

    void setText(const QString &text);
    QString text() const;

signals:
    void textChanged();

protected:
    // Intercept '.' and ',' keys, if pressed focus a specified widget
    bool eventFilter(QObject *object, QEvent *event);

private:
    QLineEdit *amount;
    QLineEdit *decimals;
};


#endif // BITCOINFIELD_H
