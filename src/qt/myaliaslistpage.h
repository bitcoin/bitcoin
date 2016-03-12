#ifndef MYALIASLISTPAGE_H
#define MYALIASLISTPAGE_H

#include <QDialog>
class PlatformStyle;
namespace Ui {
    class MyAliasListPage;
}
class AliasTableModel;
class OptionsModel;
class ClientModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QSortFilterProxyModel;
class QMenu;
class QModelIndex;
QT_END_NAMESPACE

/** Widget that shows a list of owned aliases.
  */
class MyAliasListPage : public QDialog
{
    Q_OBJECT

public:


    explicit MyAliasListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MyAliasListPage();

    void setModel(WalletModel*, AliasTableModel *model);
    void setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void showEvent ( QShowEvent * event );
public Q_SLOTS:
    void done(int retval);

private:
	ClientModel* clientModel;
	WalletModel *walletModel;
    Ui::MyAliasListPage *ui;
    AliasTableModel *model;
    OptionsModel *optionsModel;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newAliasToSelect;
	const PlatformStyle *platformStyle;
private Q_SLOTS:
    /** Create a new alias */
    void on_newAlias_clicked();
    /** Copy alias of currently selected alias entry to clipboard */
    void on_copyAlias_clicked();

    /** Copy value of currently selected alias entry to clipboard (no button) */
    void onCopyAliasValueAction();
    /** Edit currently selected alias entry (no button) */
    void on_editButton_clicked();
    /** Export button clicked */
    void on_exportButton_clicked();
    /** transfer the alias to a syscoin address  */
    void on_transferButton_clicked();
	void on_refreshButton_clicked();
	void on_newPubKey_clicked();
	void on_whitelistButton_clicked();
    /** Set button states based onf selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for alias book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to alias table */
    void selectNewAlias(const QModelIndex &parent, int begin, int /*end*/);

Q_SIGNALS:
    void transferAlias(QString addr);
};

#endif // MYALIASLISTPAGE_H
