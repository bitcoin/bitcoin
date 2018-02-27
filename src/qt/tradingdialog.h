
#ifndef TRADINGDIALOG_H
#define TRADINGDIALOG_H

#include "ui_tradingdialog.h"
#include <QWidget>
#include <QObject>
#include <stdint.h>

#include "walletmodel.h"
#include <QJsonArray>
#include <QJsonObject>

namespace Ui
{
class tradingDialog;
}

extern double _dPivPriceLast;
extern double _dBtcPriceCurrent;
extern double _dBtcPriceLast;


class tradingDialog : public QWidget
{
    Q_OBJECT

public:
    explicit tradingDialog(QWidget* parent = 0);
    ~tradingDialog();

    void setModel(WalletModel* model);

private Q_SLOTS:

    void InitTrading();
    void on_TradingTabWidget_tabBarClicked(int index);
    void ParseAndPopulateOrderBookTables(QString Response);
    void ParseAndPopulateMarketHistoryTable(QString Response);
    void ParseAndPopulateAccountHistoryTable(QString Response);
    void ParseAndPopulateOpenOrdersTable(QString Response);
    void UpdaterFunction();
    void CreateOrderBookTables(QTableWidget& Table, QStringList TableHeader);
    void CalculateBuyCostLabel();
    void CalculateSellCostLabel();
    void DisplayBalance(QLabel& BalanceLabel, QLabel& Available, QLabel& Pending, QString Currency, QString Response);
    void ActionsOnSwitch(int index);
    void CancelOrderSlot(int row, int col);
    void on_UpdateKeys_clicked();
    void on_GenDepositBTN_clicked();
    void on_Buy_Max_Amount_clicked();
    void on_buyOrdertypeCombo_activated(const QString& arg1);
    void on_BuyBidcomboBox_currentIndexChanged(const QString& arg1);
    void on_UnitsInput_textChanged(const QString& arg1);
    void on_BuyBidPriceEdit_textChanged(const QString& arg1);
    void on_BuyLBT_clicked();
    void on_SellLBTBTN_clicked();
    void on_SellBidcomboBox_currentIndexChanged(const QString& arg1);
    void on_Sell_Max_Amount_clicked();
    void on_UnitsInputLBT_textChanged(const QString& arg1);
    void on_SellBidPriceEdit_textChanged(const QString& arg1);
    void on_AdvancedView_stateChanged(int arg1);
    void enableTradingWindow(bool state);

    int SetExchangeInfoTextLabels();
    QString BittrexTimeStampToReadable(QString DateTime);
    qint64 BittrexTimeStampToSeconds(QString DateTime);
    QString CancelOrder(QString Orderid);
    QString BuyLBT(QString OrderType, double Quantity, double Rate);
    QString SellLBT(QString OrderType, double Quantity, double Rate);
    QString GetMarketHistory();
    QString GetMarketSummary();
    QString GetOrderBook();
    QString GetOpenOrders();
    QString GetAccountHistory();
    QString GetBalance(QString Currency);
    QString GetDepositAddress();
    QString GetNonce();
    QString HMAC_SHA512_SIGNER(QString UrlToSign, QString Secretkey);
    QString sendRequest(QString url);
    QJsonObject GetResultObjectFromJSONObject(QString response);
    QJsonObject GetResultObjectFromJSONArray(QString response);
    QJsonArray GetResultArrayFromJSONObject(QString response);

public Q_SLOTS:


private:
    Ui::tradingDialog* ui;
    //Socket *socket;
    int timerid;
    QTimer* timer;
    QString ApiKey;
    QString SecretKey;
    QString Currency;
    WalletModel* model;
};

#endif // TRADINGDIALOG_H
