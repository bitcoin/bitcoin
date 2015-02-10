#ifndef TRANSACTIONDESCDIALOG_H
#define TRANSACTIONDESCDIALOG_H

#include <QWidget>

namespace Ui {
    class TransactionDescDialog;
}
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog showing transaction details. */
class TransactionDescDialog : public QWidget
{
    Q_OBJECT
protected:
    void keyPressEvent(QKeyEvent *);
    void closeEvent(QCloseEvent *e);

public:
    explicit TransactionDescDialog(const QModelIndex &idx, QWidget *parent = 0);
    ~TransactionDescDialog();

private:
    Ui::TransactionDescDialog *ui;

signals:
    void stopExec();
};

#endif // TRANSACTIONDESCDIALOG_H
