#ifndef EDITALIASDIALOG_H
#define EDITALIASDIALOG_H

#include <QDialog>

namespace Ui {
    class EditAliasDialog;
}
class AliasTableModel;
class WalletModel;
QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

/** Dialog for editing an address and associated information.
 */
class EditAliasDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        NewDataAlias,
        NewAlias,
        EditDataAlias,
        EditAlias,
		TransferAlias
    };

    explicit EditAliasDialog(Mode mode, QWidget *parent = 0);
    ~EditAliasDialog();

    void setModel(WalletModel*,AliasTableModel *model);
    void loadRow(int row);

    QString getAlias() const;
    void setAlias(const QString &alias);

public Q_SLOTS:
    void accept();

private:
    bool saveCurrentRow();

    Ui::EditAliasDialog *ui;
    QDataWidgetMapper *mapper;
    Mode mode;
    AliasTableModel *model;
	WalletModel* walletModel;
    QString alias;
};

#endif // EDITALIASDIALOG_H
