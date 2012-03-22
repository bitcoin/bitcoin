#ifndef COINCONTROLPAGE_H
#define COINCONTROLPAGE_H

#include <QDialog>
#include <QTableWidgetItem>

namespace Ui {
    class CoinControlPage;
}
class BitcoinGUI;
class OptionsModel;

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class CoinControlPage : public QDialog
{
    Q_OBJECT

public:
    explicit CoinControlPage(QWidget *parent = 0);
    ~CoinControlPage();

    void setModel(OptionsModel *model);

    void UpdateTable();
    std::string selectedAddresses();
    void clearSelection();

private:
    Ui::CoinControlPage *ui;
    BitcoinGUI *gui;
    OptionsModel *model;

private slots:
    void sendFromSelectedAddress(QTableWidgetItem *item);
    void updateDisplayUnit();
};

#endif // COINCONTROLPAGE_H
