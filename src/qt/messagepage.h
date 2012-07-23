#ifndef MESSAGEPAGE_H
#define MESSAGEPAGE_H

#include <QDialog>

namespace Ui {
    class MessagePage;
}
class WalletModel;

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class MessagePage : public QDialog
{
    Q_OBJECT

public:
    explicit MessagePage(QWidget *parent = 0);
    ~MessagePage();

    void setModel(WalletModel *model);

    void setAddress(QString);

private:
    Ui::MessagePage *ui;
    WalletModel *model;

private slots:
    void on_pasteButton_clicked();
    void on_addressBookButton_clicked();

    void on_signMessage_clicked();
    void on_copyToClipboard_clicked();
};

#endif // MESSAGEPAGE_H
