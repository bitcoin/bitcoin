#ifndef TRANSACTIONVIEW_H
#define TRANSACTIONVIEW_H

#include <QWidget>

class TransactionTableModel;
class TransactionFilterProxy;

QT_BEGIN_NAMESPACE
class QTableView;
class QComboBox;
class QLineEdit;
class QModelIndex;
QT_END_NAMESPACE

class TransactionView : public QWidget
{
    Q_OBJECT
public:
    explicit TransactionView(QWidget *parent = 0);

    void setModel(TransactionTableModel *model);

    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        LastMonth,
        ThisYear,
        Range
    };

private:
    TransactionTableModel *model;
    TransactionFilterProxy *transactionProxyModel;
    QTableView *transactionView;

    QComboBox *dateWidget;
    QComboBox *typeWidget;
    QLineEdit *addressWidget;
    QLineEdit *amountWidget;

signals:
    void doubleClicked(const QModelIndex&);

public slots:
    void chooseDate(int idx);
    void chooseType(int idx);
    void changedPrefix(const QString &prefix);
    void changedAmount(const QString &amount);
    void exportClicked();

};

#endif // TRANSACTIONVIEW_H
