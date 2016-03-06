#ifndef RESELLOFFERDIALOG_H
#define RESELLOFFERDIALOG_H

#include <QDialog>

namespace Ui {
    class ResellOfferDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog for editing whitelist entry
 */
class ResellOfferDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResellOfferDialog(QModelIndex *idx, QWidget *parent = 0);
    ~ResellOfferDialog();
	void loadAliases();
public Q_SLOTS:
    void accept();

private:
    bool saveCurrentRow();
    Ui::ResellOfferDialog *ui;
};

#endif // RESELLOFFERDIALOG_H
