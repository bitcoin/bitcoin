#ifndef SENDMESSAGESPAGE_H
#define SENDMESSAGESPAGE_H

#include <QWidget>
#include <QString>

namespace Ui {
    class SendMessagesPage;
}

class MessageModel;
class SendMessagesEntry;
class SendMessagesRecipient;
class WalletModel;
class PlatformStyle;

//QT_BEGIN_NAMESPACE
//class QUrl;
//QT_END_NAMESPACE

/** Page for sending messages */
class SendMessagesPage : public QWidget
{
    Q_OBJECT

public:

    enum Mode {
        Encrypted,
        Anonymous,
    };

    enum Type {
        Page,
        Dialog,
    };

    explicit SendMessagesPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~SendMessagesPage();

    void setModel (MessageModel *model);
    void setWalletModel(WalletModel *walletmodel);
    void loadRow(int row);
    void loadInvoice(QString message, QString from_address = "", QString to_address = "");
    bool checkMode(Mode mode);
    bool validate ();

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void pasteEntry(const SendMessagesRecipient &rv);

public Q_SLOTS:
    void done(int retval);
    void clear();
    void reject();
    void accept();
    SendMessagesEntry *addEntry();
    void updateRemoveEnabled();

private:
    Ui::SendMessagesPage *ui;
    WalletModel *walletmodel;
    MessageModel *model;
    bool fNewRecipientAllowed;
    Mode mode;
    Type type;
    bool fEnableSendMessageConf;
    const PlatformStyle *platformStyle;

private Q_SLOTS:
    void enableSendMessageConf(bool enabled);
    void on_sendButton_clicked();
    void removeEntry(SendMessagesEntry* entry);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void on_inboxButton_clicked();
};

#endif // SENDMESSAGESPAGE_H
