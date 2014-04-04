// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINAMOUNTFIELD_H
#define BITCOINAMOUNTFIELD_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QValueComboBox;
QT_END_NAMESPACE

/** Widget for entering bitcoin amounts.
  */
class BitcoinAmountField: public QWidget
{
    Q_OBJECT

    Q_PROPERTY(qint64 value READ value WRITE setValue NOTIFY textChanged USER true)

public:
    explicit BitcoinAmountField(QWidget *parent = 0);

    qint64 value(bool *valid=0) const;
    void setValue(qint64 value);

    /** Set single step in satoshis **/
    void setSingleStep(qint64 step);

    /** Make read-only **/
    void setReadOnly(bool fReadOnly);

    /** Mark current value as invalid in UI. */
    void setValid(bool valid);
    /** Perform input validation, mark field as invalid if entered value is not valid. */
    bool validate();

    /** Change unit used to display amount. */
    void setDisplayUnit(int unit);

    /** Make field empty and ready for new input. */
    void clear();

    /** Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907),
        in these cases we have to set it up manually.
    */
    QWidget *setupTabChain(QWidget *prev);

signals:
    void textChanged();

protected:
    /** Intercept focus-in event and ',' key presses */
    bool eventFilter(QObject *object, QEvent *event);

private:
    QDoubleSpinBox *amount;
    QValueComboBox *unit;
    int currentUnit;
    qint64 nSingleStep;

    void setText(const QString &text);
    QString text() const;

private slots:
    void unitChanged(int idx);

};

#endif // BITCOINAMOUNTFIELD_H
