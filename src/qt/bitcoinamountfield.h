#ifndef BITCOINFIELD_H
#define BITCOINFIELD_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QValidatedLineEdit;
class QValueComboBox;
QT_END_NAMESPACE

// Coin amount entry widget with separate parts for whole
// coins and decimals.
class BitcoinAmountField: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qint64 value READ value WRITE setValue NOTIFY textChanged USER true);
public:
    explicit BitcoinAmountField(QWidget *parent = 0);

    qint64 value(bool *valid=0) const;
    void setValue(qint64 value);

    // Mark current valid as invalid in UI
    void setValid(bool valid);
    bool validate();

    // Change current unit
    void setDisplayUnit(int unit);

    // Make field empty and ready for new input
    void clear();

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
    QValueComboBox *unit;
    int currentUnit;

    void setText(const QString &text);
    QString text() const;

private slots:
    void unitChanged(int idx);

};


#endif // BITCOINFIELD_H
