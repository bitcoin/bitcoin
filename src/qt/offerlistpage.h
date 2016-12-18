#ifndef OFFERLISTPAGE_H
#define OFFERLISTPAGE_H

#include <QDialog>
#include <map>
#include <utility>
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
class QMenu;
class QModelIndex;
class QKeyEvent;
class QStandardItemModel;
QT_END_NAMESPACE

/** Widget that shows a list of owned certs.
  */
class OfferListPage : public QDialog
{
    Q_OBJECT

public:
   

    explicit OfferListPage(const PlatformStyle *platformStyle, OfferView *parent);
    ~OfferListPage();

    void addParentItem(QStandardItemModel * model, const QString& text, const QVariant& data );
    void addChildItem( QStandardItemModel * model, const QString& text, const QVariant& data );


    void setModel(WalletModel*, OfferTableModel *model);
    void setOptionsModel(OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void keyPressEvent(QKeyEvent * event);
	void showEvent ( QShowEvent * event );
private:
	void loadCategories();
    Ui::OfferListPage *ui;
    OfferTableModel *model;
    OptionsModel *optionsModel;
	WalletModel* walletModel;
    QString returnValue;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newOfferToSelect;
	OfferView* offerView;
	std::map<int, std::pair<std::string, std::string> > pageMap;
	int currentPage;
private Q_SLOTS:
	void on_resellButton_clicked();
	void on_searchOffer_clicked(std::string offer="");
	void on_prevButton_clicked();
	void on_nextButton_clicked();
	void on_purchaseButton_clicked();
	void on_messageButton_clicked();
    /** Create a new Offer for receiving coins and / or add a new Offer book entry */
    /** Copy Offer of currently selected Offer entry to clipboard */
    void on_copyOffer_clicked();
    /** Copy value of currently selected Offer entry to clipboard (no button) */
    void onCopyOfferValueAction();
	void onCopyOfferDescriptionAction();

    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for Offer book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to Offer table */
    void selectNewOffer(const QModelIndex &parent, int begin, int /*end*/);

};

#endif // OFFERLISTPAGE_H
