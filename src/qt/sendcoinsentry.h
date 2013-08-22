#ifndef SENDCOINSENTRY_H
#define SENDCOINSENTRY_H

#include <QStackedWidget>

#include "walletmodel.h"

namespace Ui {
    class SendCoinsEntry;
}
class WalletModel;

/**
 * A single entry in the dialog for sending bitcoins.
 * Stacked widget, with different UIs for payment requests
 * with a strong payee identity.
 */
class SendCoinsEntry : public QStackedWidget
{
    Q_OBJECT

public:
    explicit SendCoinsEntry(QWidget *parent = 0);
    ~SendCoinsEntry();

    void setModel(WalletModel *model);
    bool validate();
    SendCoinsRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const SendCoinsRecipient &value);
    void setAddress(const QString &address);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setFocus();

public slots:
    void setRemoveEnabled(bool enabled);
    void clear();

signals:
    void removeEntry(SendCoinsEntry *entry);

private slots:
    void on_deleteButton_clicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();

private:
    SendCoinsRecipient recipient;
    Ui::SendCoinsEntry *ui;
    WalletModel *model;
};

#endif // SENDCOINSENTRY_H
