#ifndef MYCERTLISTPAGE_H
#define MYCERTLISTPAGE_H

#include <QDialog>
class PlatformStyle;
namespace Ui {
    class MyCertListPage;
}
class CertTableModel;
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
class MyCertListPage : public QDialog
{
    Q_OBJECT

public:


    explicit MyCertListPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MyCertListPage();

    void setModel(WalletModel*, CertTableModel *model);
    void setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
	void showEvent ( QShowEvent * event );
	void loadAliasList();
public Q_SLOTS:
    void done(int retval);

private:
	ClientModel* clientModel;
	WalletModel *walletModel;
    Ui::MyCertListPage *ui;
    CertTableModel *model;
    OptionsModel *optionsModel;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newCertToSelect;

private Q_SLOTS:
	void on_sellCertButton_clicked();
    /** Create a new cert */
    void on_newCert_clicked();
    /** Copy cert of currently selected cert entry to clipboard */
    void on_copyCert_clicked();

    /** Copy value of currently selected cert entry to clipboard (no button) */
    void onCopyCertValueAction();
    /** Edit currently selected cert entry (no button) */
    void on_editButton_clicked();
    /** Export button clicked */
    void on_exportButton_clicked();
    /** transfer the cert to a syscoin address  */
    void on_transferButton_clicked();
	void on_refreshButton_clicked();

    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for cert book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to cert table */
    void selectNewCert(const QModelIndex &parent, int begin, int /*end*/);
	void displayListChanged(const QString& alias);
Q_SIGNALS:
    void transferCert(QString addr);
};

#endif // MYCERTLISTPAGE_H
