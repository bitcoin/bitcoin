#ifndef MYWHITELISTOFFERDIALOG_H
#define MYWHITELISTOFFERDIALOG_H

#include <QDialog>
class PlatformStyle;
namespace Ui {
    class MyWhitelistOfferDialog;
}
class MyOfferWhitelistTableModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QMessageBox;
class QSortFilterProxyModel;
class QMenu;
class QModelIndex;
class QClipboard;
QT_END_NAMESPACE

/** Dialog for editing whitelists
 */
class MyWhitelistOfferDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MyWhitelistOfferDialog(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MyWhitelistOfferDialog();

    void setModel(WalletModel*);

protected:
	 void showEvent(QShowEvent *e);
private:
    Ui::MyWhitelistOfferDialog *ui;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    MyOfferWhitelistTableModel *model;
	WalletModel* walletModel;
    QString newEntryToSelect;

private Q_SLOTS:
    /** Copy alias of currently selected cert entry to clipboard */
    void on_copy();

    /** Export button clicked */
    void on_exportButton_clicked();
	void on_refreshButton_clicked();

    /** Spawn contextual menu (right mouse menu) for alias book entry */
    void contextualMenu(const QPoint &point);


};

#endif // MYWHITELISTOFFERDIALOG_H