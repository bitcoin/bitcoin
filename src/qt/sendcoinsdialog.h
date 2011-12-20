#ifndef SENDCOINSDIALOG_H
#define SENDCOINSDIALOG_H

#include <QtGlobal>
#include <QDialog>

namespace Ui {
    class SendCoinsDialog;
}
class WalletModel;
class SendCoinsEntry;
class SendCoinsRecipient;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

/** Dialog for sending bitcoins */
class SendCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendCoinsDialog(QWidget *parent = 0);
    ~SendCoinsDialog();

    void setModel(WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue http://bugreports.qt.nokia.com/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void pasteEntry(const SendCoinsRecipient &rv);
    void handleURL(const QUrl *url);

public slots:
    void clear();
    void reject();
    void accept();
    SendCoinsEntry *addEntry();
    void updateRemoveEnabled();
    void setBalance(qint64 balance, qint64 unconfirmedBalance);

private:
    Ui::SendCoinsDialog *ui;
    WalletModel *model;

private slots:
    void on_sendButton_clicked();

    void removeEntry(SendCoinsEntry* entry);
};

#endif // SENDCOINSDIALOG_H
