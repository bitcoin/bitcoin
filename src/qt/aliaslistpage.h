#ifndef ALIASLISTPAGE_H
#define ALIASLISTPAGE_H

#include <QDialog>
#include <map>
#include <utility>
class PlatformStyle;
namespace Ui {
    class AliasListPage;
}
class AliasTableModel;
class OptionsModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QMenu;
class QModelIndex;
class QKeyEvent;
QT_END_NAMESPACE

/** Widget that shows a list of owned aliases.
  */
class AliasListPage : public QDialog
{
    Q_OBJECT

public:
   

    explicit AliasListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~AliasListPage();


    void setModel(WalletModel*, AliasTableModel *model);
    void setOptionsModel(OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void keyPressEvent(QKeyEvent * event);
	void showEvent ( QShowEvent * event );
private:
    Ui::AliasListPage *ui;
    AliasTableModel *model;
    OptionsModel *optionsModel;
	WalletModel* walletModel;
    QString returnValue;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newAliasToSelect;
	std::map<int, std::pair<std::string, std::string> > pageMap;
	int currentPage;
private Q_SLOTS:
	void on_signMultisigButton_clicked();
	void on_searchAlias_clicked(std::string offer="");
	void on_prevButton_clicked();
	void on_nextButton_clicked();
    /** Create a new alias for receiving coins and / or add a new alias book entry */
    /** Copy alias of currently selected alias entry to clipboard */
    void on_copyAlias_clicked();
	void on_messageButton_clicked();

    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for alias book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to alias table */
    void selectNewAlias(const QModelIndex &parent, int begin, int /*end*/);


};

#endif // ALIASLISTPAGE_H
