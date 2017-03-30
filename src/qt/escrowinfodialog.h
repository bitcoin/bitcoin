#ifndef ESCROWINFODIALOG_H
#define ESCROWINFODIALOG_H
#include <QDialog>
class PlatformStyle;
class QDataWidgetMapper;
class UniValue;
class QString;
namespace Ui {
    class EscrowInfoDialog;
}
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
/** Dialog for editing an address and associated information.
 */
class EscrowInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EscrowInfoDialog(const PlatformStyle *platformStyle, const QModelIndex &idx, QWidget *parent=0);
    ~EscrowInfoDialog();
private:
	bool lookup();
	void SetFeedbackUI(const UniValue &escrowFeedback, const QString &userType, const QString& buyer, const QString& seller, const QString& arbiter);
private Q_SLOTS:
	void on_okButton_clicked();
private:
	QDataWidgetMapper *mapper;
    Ui::EscrowInfoDialog *ui;
	QString GUID;
};

#endif // ESCROWINFODIALOG_H
