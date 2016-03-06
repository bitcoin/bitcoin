#ifndef CERTLISTPAGE_H
#define CERTLISTPAGE_H

#include <QDialog>
class PlatformStyle;
namespace Ui {
    class CertListPage;
}
class CertTableModel;
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

/** Widget that shows a list of owned certs.
  */
class CertListPage : public QDialog
{
    Q_OBJECT

public:
   

    explicit CertListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~CertListPage();


    void setModel(WalletModel*, CertTableModel *model);
    void setOptionsModel(OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void keyPressEvent(QKeyEvent * event);
	void showEvent ( QShowEvent * event );
private:
    Ui::CertListPage *ui;
    CertTableModel *model;
    OptionsModel *optionsModel;
	WalletModel* walletModel;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newCertToSelect;

private Q_SLOTS:
    void on_searchCert_clicked();
    /** Create a new Cert for receiving coins and / or add a new Cert book entry */
    /** Copy Cert of currently selected Cert entry to clipboard */
    void on_copyCert_clicked();
    /** Copy value of currently selected Cert entry to clipboard (no button) */
    void onCopyCertValueAction();

    /** Export button clicked */
    void on_exportButton_clicked();

    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for Cert book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to Cert table */
    void selectNewCert(const QModelIndex &parent, int begin, int /*end*/);
};

#endif // CERTLISTPAGE_H
