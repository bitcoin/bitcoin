#ifndef MESSAGEINFODIALOG_H
#define MESSAGEINFODIALOG_H

#include <QDialog>

namespace Ui {
    class MessageInfoDialog;
}
class MessageTableModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

/** Dialog for editing an address and associated information.
 */
class MessageInfoDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        MessageInfo,
        ReplyMessage
    };
    explicit MessageInfoDialog(QWidget *parent = 0);
    ~MessageInfoDialog();

    void setModel(WalletModel*,MessageTableModel *model);
    void loadRow(int row);


public Q_SLOTS:
    void accept();

private:
    Ui::MessageInfoDialog *ui;
    QDataWidgetMapper *mapper;
    MessageTableModel *model;
	WalletModel* walletModel;
    QString message;
};

#endif // MESSAGEINFODIALOG_H
