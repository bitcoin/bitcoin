#ifndef EDITCERTDIALOG_H
#define EDITCERTDIALOG_H

#include <QDialog>

namespace Ui {
    class EditCertDialog;
}
class CertTableModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

/** Dialog for editing an address and associated information.
 */
class EditCertDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        NewCert,
        EditCert,
		TransferCert
    };

    explicit EditCertDialog(Mode mode, QWidget *parent = 0);
    ~EditCertDialog();

    void setModel(WalletModel*,CertTableModel *model);
    void loadRow(int row, const QString &privatecert="");

    QString getCert() const;
    void setCert(const QString &cert);

public Q_SLOTS:
    void accept();

private:
    bool saveCurrentRow();
	void loadAliases();
    Ui::EditCertDialog *ui;
    QDataWidgetMapper *mapper;
    Mode mode;
    CertTableModel *model;
	WalletModel* walletModel;
    QString cert;
};

#endif // EDITCERTDIALOG_H
