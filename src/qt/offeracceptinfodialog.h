#ifndef OFFERACCEPTINFODIALOG_H
#define OFFERACCEPTINFODIALOG_H
#include <QDialog>
class PlatformStyle;
class QDataWidgetMapper;
class UniValue;
namespace Ui {
    class OfferAcceptInfoDialog;
}
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
/** Dialog for editing an address and associated information.
 */
class OfferAcceptInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OfferAcceptInfoDialog(const PlatformStyle *platformStyle, const QModelIndex &idx, QWidget *parent=0);
    ~OfferAcceptInfoDialog();
private:
	bool lookup();
	void SetFeedbackUI(const UniValue &feedbackObj, const QString &userType, const QString& buyer, const QString& seller);
private Q_SLOTS:
	void on_okButton_clicked();
private:
	QDataWidgetMapper *mapper;
    Ui::OfferAcceptInfoDialog *ui;
	QString offerGUID;
	QString offerAcceptGUID;

};

#endif // OFFERACCEPTINFODIALOG_H
