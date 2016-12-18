#ifndef OFFERESCROWDIALOG_H
#define OFFERESCROWDIALOG_H

#include <QDialog>
class PlatformStyle;
namespace Ui {
    class OfferEscrowDialog;
}

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class OfferEscrowDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OfferEscrowDialog(const PlatformStyle *platformStyle, QString title, QString quantity, QString price, QString currency, QWidget *parent = 0);
    ~OfferEscrowDialog();


private:
	Ui::OfferEscrowDialog *ui;
 
};

#endif // OFFERESCROWDIALOG_H
