#ifndef SENDMESSAGESENTRY_H
#define SENDMESSAGESENTRY_H

#include <QFrame>

namespace Ui {
    class SendMessagesEntry;
}

class MessageModel;
class SendMessagesRecipient;
class WalletModel;
class PlatformStyle;

/** A single entry in the dialog for sending messages. */
class SendMessagesEntry : public QFrame
{
    Q_OBJECT

public:
    explicit SendMessagesEntry(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~SendMessagesEntry();

    void setModel(MessageModel *model);
    void setWalletModel(WalletModel *walletmodel);
    void loadRow(int row);
    void loadInvoice(QString message, QString to_address);
    bool validate();
    SendMessagesRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const SendMessagesRecipient &value);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setFocus();

public Q_SLOTS:
    void setRemoveEnabled(bool enabled);
    void clear();
    
Q_SIGNALS:
    void removeEntry(SendMessagesEntry *entry);

private Q_SLOTS:
    void on_deleteButton_clicked();
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void on_sendTo_textChanged(const QString &address);

private:
    Ui::SendMessagesEntry *ui;
    WalletModel *walletmodel;
    MessageModel *model;
    const PlatformStyle *platformStyle;

};

#endif // SENDMESSAGESENTRY_H
