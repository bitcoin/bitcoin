#ifndef EDITWHITELISTOFFERDIALOG_H
#define EDITWHITELISTOFFERDIALOG_H

#include <QDialog>
#include <QDialog>
class PlatformStyle;
namespace Ui {
    class EditWhitelistOfferDialog;
}
class OfferWhitelistTableModel;
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
class EditWhitelistOfferDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditWhitelistOfferDialog(const PlatformStyle *platformStyle, QModelIndex *idx,QWidget *parent = 0);
    ~EditWhitelistOfferDialog();

    void setModel(WalletModel*,OfferWhitelistTableModel *model);

protected:
	 void showEvent(QShowEvent *e);
private:
    Ui::EditWhitelistOfferDialog *ui;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    OfferWhitelistTableModel *model;
	WalletModel* walletModel;
    QString newEntryToSelect;
	QString offerGUID;
	QString offerCategory;
	QString offerTitle;
	QString offerQty;
	QString offerPrice;
	QString offerDescription;
	QString offerCurrency;
	QString offerPrivate;

private Q_SLOTS:
    /** Create a new cert */
    void on_newEntry_clicked();
    /** Copy cert of currently selected cert entry to clipboard */
    void on_copy();

    /** Export button clicked */
    void on_exportButton_clicked();
	void on_refreshButton_clicked();
	void on_removeButton_clicked();
	void on_removeAllButton_clicked();
    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for cert book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to cert table */
    void selectNewEntry(const QModelIndex &parent, int begin, int /*end*/);

};

#endif // EDITWHITELISTOFFERDIALOG_H