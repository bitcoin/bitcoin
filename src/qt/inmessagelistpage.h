#ifndef INMESSAGELISTPAGE_H
#define INMESSAGELISTPAGE_H

#include <QDialog>
class PlatformStyle;
namespace Ui {
    class MessageListPage;
}
class MessageTableModel;
class OptionsModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QSortFilterProxyModel;
class QMenu;
class QModelIndex;
class QKeyEvent;
QT_END_NAMESPACE

/** Widget that shows a list of owned messagees.
  */
class InMessageListPage : public QDialog
{
    Q_OBJECT

public:
   

    explicit InMessageListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~InMessageListPage();


    void setModel(WalletModel*, MessageTableModel *model);
    void setOptionsModel(OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void keyPressEvent(QKeyEvent * event);
	void showEvent ( QShowEvent * event );
private:
    Ui::MessageListPage *ui;
    MessageTableModel *model;
    OptionsModel *optionsModel;
	WalletModel* walletModel;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newMessageToSelect;

private Q_SLOTS:
    /** Copy message of currently selected entry to clipboard */
    void on_copyMessage_clicked();
	void on_copySubject_clicked();
	void on_copyGuid_clicked();
	void on_replyMessage_clicked();
	void on_newMessage_clicked();
	void on_refreshButton_clicked();
	void on_detailButton_clicked();
    /** Export button clicked */
    void on_exportButton_clicked();

    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for message book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to message table */
    void selectNewMessage(const QModelIndex &parent, int begin, int /*end*/);
	
};

#endif // INMESSAGELISTPAGE_H
