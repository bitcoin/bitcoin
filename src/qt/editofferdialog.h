#ifndef EDITOFFERDIALOG_H
#define EDITOFFERDIALOG_H

#include <QDialog>

namespace Ui {
    class EditOfferDialog;
}
class OfferTableModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
class QString;
QT_END_NAMESPACE

/** Dialog for editing an offer
 */
class EditOfferDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        NewOffer,
        EditOffer,
		NewCertOffer
    };

    explicit EditOfferDialog(Mode mode, const QString &cert="", QWidget *parent = 0);
    ~EditOfferDialog();

    void setModel(WalletModel*,OfferTableModel *model);
    void loadRow(int row);

    QString getOffer() const;
    void setOffer(const QString &offer);

public Q_SLOTS:
    void accept();
	void certChanged(int);
	void on_aliasPegEdit_textChanged(const QString& text);
private:
    bool saveCurrentRow();
	void loadCerts();
	void loadAliases();
    Ui::EditOfferDialog *ui;
    QDataWidgetMapper *mapper;
    Mode mode;
    OfferTableModel *model;
	WalletModel* walletModel;
    QString offer;
	QString cert;
};

#endif // EDITOFFERDIALOG_H
