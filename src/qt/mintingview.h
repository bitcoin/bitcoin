#ifndef MINTINGVIEW_H
#define MINTINGVIEW_H

#include <QWidget>

class WalletModel;

QT_BEGIN_NAMESPACE
class QTableView;
QT_END_NAMESPACE

class MintingView : public QWidget
{
    Q_OBJECT
public:
    explicit MintingView(QWidget *parent = 0);
    void setModel(WalletModel *model);

private:
    WalletModel *model;
    QTableView *mintingView;
signals:

public slots:
    void exportClicked();
};

#endif // MINTINGVIEW_H
