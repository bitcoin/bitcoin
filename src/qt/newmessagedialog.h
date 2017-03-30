#ifndef NEWMESSAGEDIALOG_H
#define NEWMESSAGEDIALOG_H

#include <QDialog>

namespace Ui {
    class NewMessageDialog;
}
class MessageTableModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

/** Dialog for editing an address and associated information.
 */
class NewMessageDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        NewMessage,
        ReplyMessage
    };
    explicit NewMessageDialog(Mode mode, const QString &to=QString(""), const QString &title=QString(""), QWidget *parent = 0);
    ~NewMessageDialog();

    void setModel(WalletModel*,MessageTableModel *model);
    void loadRow(int row);


public Q_SLOTS:
    void accept();

private:
    bool saveCurrentRow();
	void loadAliases();
    Ui::NewMessageDialog *ui;
    QDataWidgetMapper *mapper;
    Mode mode;
    MessageTableModel *model;
	WalletModel* walletModel;
    QString message;
};

#endif // NEWMESSAGEDIALOG_H
