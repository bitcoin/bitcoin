#ifndef ESCROWLISTPAGE_H
#define ESCROWLISTPAGE_H

#include <QDialog>
#include <map>
#include <utility>
class PlatformStyle;
namespace Ui {
    class EscrowListPage;
}
class EscrowTableModel;
class OptionsModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QMenu;
class QModelIndex;
class QKeyEvent;
QT_END_NAMESPACE

/** Widget that shows a list of owned escrowes.
  */
class EscrowListPage : public QDialog
{
    Q_OBJECT

public:
   

    explicit EscrowListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~EscrowListPage();


    void setModel(WalletModel*, EscrowTableModel *model);
    void setOptionsModel(OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void keyPressEvent(QKeyEvent * event);
	void showEvent ( QShowEvent * event );
private:
    Ui::EscrowListPage *ui;
    EscrowTableModel *model;
    OptionsModel *optionsModel;
	WalletModel* walletModel;
    QString returnValue;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newEscrowToSelect;
	std::map<int, std::pair<std::string, std::string> > pageMap;
	int currentPage;
	const PlatformStyle *platformStyle;
private Q_SLOTS:
	void on_detailButton_clicked();
	void on_manageButton_clicked();
	void on_searchEscrow_clicked(std::string offer="");
	void on_prevButton_clicked();
	void on_nextButton_clicked();
    /** Create a new escrow for receiving coins and / or add a new escrow book entry */
    /** Copy escrow of currently selected escrow entry to clipboard */
    void on_copyEscrow_clicked();
    /** Copy value of currently selected escrow entry to clipboard (no button) */
    void on_copyOffer_clicked();

    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for escrow book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to escrow table */
    void selectNewEscrow(const QModelIndex &parent, int begin, int /*end*/);
	void on_ackButton_clicked();


};

#endif // ESCROWLISTPAGE_H
