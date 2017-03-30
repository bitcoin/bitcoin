#ifndef MYOFFERLISTPAGE_H
#define MYOFFERLISTPAGE_H

#include <QDialog>
class PlatformStyle;
namespace Ui {
    class MyOfferListPage;
}
class OfferWhitelistTableModel;
class OfferTableModel;
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

/** Widget that shows a list of owned certes.
  */
class MyOfferListPage : public QDialog
{
    Q_OBJECT

public:


    explicit MyOfferListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MyOfferListPage();

    void setModel(WalletModel*, OfferTableModel *model);
    void setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void showEvent ( QShowEvent * event );
	void loadAliasList();
public Q_SLOTS:
    void done(int retval);

private:
	ClientModel* clientModel;
	WalletModel *walletModel;
    Ui::MyOfferListPage *ui;
    OfferTableModel *model;
	OfferWhitelistTableModel* offerWhitelistTableModel;
    OptionsModel *optionsModel;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newOfferToSelect;
	const PlatformStyle *platformStyle;
private Q_SLOTS:
	void onToggleShowSoldOut(bool toggled);
	void onToggleShowDigitalOffers(bool toggled);
	void on_whitelistButton_clicked();
	void onEditWhitelistAction();
    void on_newOffer_clicked();
    void on_copyOffer_clicked();
	void onCopyOfferDescriptionAction();
    void onCopyOfferValueAction();
    void on_editButton_clicked();
    /** Export button clicked */
    void on_exportButton_clicked();
	void on_refreshButton_clicked();
    /** Set button states based on selected tab and selection */
    void selectionChanged();

    void contextualMenu(const QPoint &point);

    void selectNewOffer(const QModelIndex &parent, int begin, int /*end*/);
	void displayListChanged(const QString& alias);

};

#endif // MYOFFERLISTPAGE_H
