#ifndef SENDCOINSDIALOG_H
#define SENDCOINSDIALOG_H

#include <QDialog>

namespace Ui {
    class SendCoinsDialog;
}
class WalletModel;
class SendCoinsEntry;

class SendCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendCoinsDialog(QWidget *parent = 0);
    ~SendCoinsDialog();

    void setModel(WalletModel *model);

    // Qt messes up the tab chain by default in some cases (issue http://bugreports.qt.nokia.com/browse/QTBUG-10907)
    // Hence we have to set it up manually
    QWidget *setupTabChain(QWidget *prev);

public slots:
    void clear();
    void reject();
    void accept();
    void addEntry();
    void updateRemoveEnabled();

private:
    Ui::SendCoinsDialog *ui;
    WalletModel *model;

private slots:
    void on_sendButton_clicked();

    void removeEntry(SendCoinsEntry* entry);
};

#endif // SENDCOINSDIALOG_H
