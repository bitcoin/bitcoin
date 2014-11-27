#ifndef MINTINGVIEW_H
#define MINTINGVIEW_H

#include <QWidget>
#include <QComboBox>
#include "mintingfilterproxy.h"

class WalletModel;


QT_BEGIN_NAMESPACE
class QTableView;
class QMenu;
QT_END_NAMESPACE

class MintingView : public QWidget
{
    Q_OBJECT
public:
    explicit MintingView(QWidget *parent = 0);
    void setModel(WalletModel *model);

    enum MintingEnum
    {
        Minting10min,
        Minting1day,
        Minting7days,
        Minting30days,
        Minting60days,
        Minting90days
    };

private:
    WalletModel *model;
    QTableView *mintingView;

    QComboBox *mintingCombo;

    MintingFilterProxy *mintingProxyModel;

    QMenu *contextMenu;

signals:

public slots:
    void exportClicked();
    void chooseMintingInterval(int idx);
    void copyTxID();
    void copyAddress();
    void showHideAddress();
    void showHideTxID();
    void contextualMenu(const QPoint &point);
};

#endif // MINTINGVIEW_H
