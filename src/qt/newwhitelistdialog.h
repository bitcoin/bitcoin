#ifndef NEWWHITELISTDIALOG_H
#define NEWWHITELISTDIALOG_H

#include <QDialog>

namespace Ui {
    class NewWhitelistDialog;
}
class OfferWhitelistTableModel;
class WalletModel;
QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

/** Dialog for editing whitelist entry
 */
class NewWhitelistDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewWhitelistDialog(const QString& offerGUID, QWidget *parent = 0);
    ~NewWhitelistDialog();

    void setModel(WalletModel*,OfferWhitelistTableModel *model);
    QString getEntry() const;

public Q_SLOTS:
    void accept();

private:
    bool saveCurrentRow();

    Ui::NewWhitelistDialog *ui;
    OfferWhitelistTableModel *model;
	WalletModel* walletModel;
    QString entry;
};

#endif // NEWWHITELISTDIALOG_H
