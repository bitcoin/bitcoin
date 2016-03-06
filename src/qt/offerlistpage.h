#ifndef OFFERLISTPAGE_H
#define OFFERLISTPAGE_H

#include <QDialog>

namespace Ui {
    class OfferListPage;
}
class OfferTableModel;
class OfferView;
class OptionsModel;
class WalletModel;
class PlatformStyle;
QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QSortFilterProxyModel;
class QMenu;
class QModelIndex;
class QKeyEvent;
QT_END_NAMESPACE

/** Widget that shows a list of owned certs.
  */
class OfferListPage : public QDialog
{
    Q_OBJECT

public:
   

    explicit OfferListPage(const PlatformStyle *platformStyle, OfferView *parent);
    ~OfferListPage();


    void setModel(WalletModel*, OfferTableModel *model);
    void setOptionsModel(OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void keyPressEvent(QKeyEvent * event);
	void showEvent ( QShowEvent * event );
private:
    Ui::OfferListPage *ui;
    OfferTableModel *model;
    OptionsModel *optionsModel;
	WalletModel* walletModel;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newOfferToSelect;
	OfferView* offerView;
private Q_SLOTS:
	void on_resellButton_clicked();
    void on_searchOffer_clicked();
	void on_purchaseButton_clicked();
	void on_messageButton_clicked();
    /** Create a new Offer for receiving coins and / or add a new Offer book entry */
    /** Copy Offer of currently selected Offer entry to clipboard */
    void on_copyOffer_clicked();
    /** Copy value of currently selected Offer entry to clipboard (no button) */
    void onCopyOfferValueAction();
	void onCopyOfferDescriptionAction();
    /** Export button clicked */
    void on_exportButton_clicked();

    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for Offer book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to Offer table */
    void selectNewOffer(const QModelIndex &parent, int begin, int /*end*/);

};

#endif // OFFERLISTPAGE_H
