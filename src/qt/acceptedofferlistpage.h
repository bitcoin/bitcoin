#ifndef ACCEPTEDOFFERLISTPAGE_H
#define ACCEPTEDOFFERLISTPAGE_H

#include <QDialog>
class PlatformStyle;
namespace Ui {
    class AcceptedOfferListPage;
}
class OfferAcceptTableModel;
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
class AcceptedOfferListPage : public QDialog
{
    Q_OBJECT

public:


    explicit AcceptedOfferListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~AcceptedOfferListPage();

    void setModel(WalletModel*, OfferAcceptTableModel *model);
    void setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void showEvent ( QShowEvent * event );
	void loadAliasList();
public Q_SLOTS:
    void done(int retval);

private:
	const PlatformStyle *platformStyle;
	ClientModel* clientModel;
	WalletModel *walletModel;
    Ui::AcceptedOfferListPage *ui;
    OfferAcceptTableModel *model;
    OptionsModel *optionsModel;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newOfferToSelect;

private Q_SLOTS:

    /** Copy cert of currently selected cert entry to clipboard */
    void on_copyOffer_clicked();
    /** Copy value of currently selected cert entry to clipboard (no button) */
    void onCopyOfferValueAction();
    /** Export button clicked */
    void on_exportButton_clicked();
	void on_refreshButton_clicked();
	void on_messageButton_clicked();
	void on_feedbackButton_clicked();
    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for cert book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to cert table */
    void selectNewOffer(const QModelIndex &parent, int begin, int /*end*/);
	void on_detailButton_clicked();
	void displayListChanged(const QString& alias);

};

#endif // ACCEPTEDOFFERLISTPAGE_H
