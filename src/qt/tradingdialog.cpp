#include "tradingdialog.h"
#include "ui_tradingdialog.h"
#include <qmessagebox.h>
#include <qtimer.h>

#include <QDebug>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QVariantMap>

#include <openssl/hmac.h>

#include <boost/lexical_cast.hpp>
#include <boost/xpressive/xpressive.hpp>

using namespace boost::xpressive;
using namespace std;

//Coinbase API, latest BTC orice
const QString apiCoinbasePrice = "https://www.bitstamp.net/api/ticker/";

//Bittrex API
const QString apiBittrexMarketSummary = "https://bittrex.com/api/v1.1/public/getmarketsummaries";
const QString apiBittrexTrades = "https://bittrex.com/api/v1.1/public/getmarkethistory?market=btc-liberta";
const QString apiBittrexOrders = "https://bittrex.com/api/v1.1/public/getorderbook?market=btc-liberta&type=both&depth=50";

QString dequote(QString s)
{
    string str(s.toStdString());
    sregex nums = sregex::compile(":\\\"(-?\\d*(\\.\\d+))\\\"");
    string nm(":$1");
    str = regex_replace(str, nums, nm);
    sregex tru = sregex::compile("\\\"true\\\"");
    string tr("true");
    str = regex_replace(str, tru, tr);
    sregex fal = sregex::compile("\\\"false\\\"");
    string fl("false");
    str = regex_replace(str, fal, fl);
    QString res = str.c_str();
    return res;
}

tradingDialog::tradingDialog(QWidget* parent) : QWidget(parent),
                                                ui(new Ui::tradingDialog)
{
    ui->setupUi(this);
    timerid = 0;
    // qDebug() <<  "Expected this";

    QPalette sample_palette;
    sample_palette.setColor(QPalette::Window, Qt::green);
    connect(ui->enable, SIGNAL(toggled(bool)), this, SLOT(enableTradingWindow(bool)));

    InitTrading();

    ui->BuyCostLabel->setPalette(sample_palette);
    ui->SellCostLabel->setPalette(sample_palette);
    ui->LBTAvailableLabel->setPalette(sample_palette);
    ui->BtcAvailableLbl_2->setPalette(sample_palette);
    //Set tabs to inactive
    ui->TradingTabWidget->setTabEnabled(2, false);
    ui->TradingTabWidget->setTabEnabled(3, false);
    ui->TradingTabWidget->setTabEnabled(4, false);
    ui->TradingTabWidget->setTabEnabled(5, false);
    ui->TradingTabWidget->setTabEnabled(6, false);

    /*OrderBook Table Init*/
    CreateOrderBookTables(*ui->BidsTable, QStringList() << "Total(BTC)"
                                                        << "LBT(SIZE)"
                                                        << "Bid(BTC)");
    ui->BidsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);                                                    
    CreateOrderBookTables(*ui->AsksTable, QStringList() << "Ask(BTC)"
                                                        << "LBT(SIZE)"
                                                        << "Total(BTC)");
    ui->AsksTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);                                                    
    /*OrderBook Table Init*/

    /*Market History Table Init*/
    ui->MarketHistoryTable->setColumnCount(5);
    ui->MarketHistoryTable->verticalHeader()->setVisible(false);

    ui->MarketHistoryTable->setHorizontalHeaderLabels(QStringList() << "Date"
                                                                    << "Buy/Sell"
                                                                    << "Bid/Ask"
                                                                    << "Total units(LBT)"
                                                                    << "Total cost(BTC");
    ui->MarketHistoryTable->setRowCount(0);

    int Cellwidth = ui->MarketHistoryTable->width() / 5;

    ui->MarketHistoryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->MarketHistoryTable->horizontalHeader()->resizeSection(1, Cellwidth); // column 1, width 50
    ui->MarketHistoryTable->horizontalHeader()->resizeSection(2, Cellwidth);
    ui->MarketHistoryTable->horizontalHeader()->resizeSection(3, Cellwidth);
    ui->MarketHistoryTable->horizontalHeader()->resizeSection(4, Cellwidth);
    ui->MarketHistoryTable->horizontalHeader()->resizeSection(5, Cellwidth);
    ui->MarketHistoryTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->MarketHistoryTable->horizontalHeader()->setStyleSheet("QHeaderView::section, QHeaderView::section * {font-weight :bold;}");
    /*Market History Table Init*/

    /*Account History Table Init*/
    ui->TradeHistoryTable->setColumnCount(13);

    ui->TradeHistoryTable->setHorizontalHeaderLabels(QStringList() << "Date Time"
                                                                   << "Exchange"
                                                                   << "OrderType"
                                                                   << "Limit"
                                                                   << "QTY"
                                                                   << "QTY_Rem"
                                                                   << "Price"
                                                                   << "PricePerUnit"
                                                                   << "Conditional"
                                                                   << "Condition"
                                                                   << "Condition Target"
                                                                   << "ImmediateOrCancel"
                                                                   << "Closed");
    ui->TradeHistoryTable->setRowCount(0);
    ui->TradeHistoryTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->TradeHistoryTable->horizontalHeader()->setStyleSheet("QHeaderView::section, QHeaderView::section * {font-weight :bold;}");
    ui->TradeHistoryTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    /*Account History Table Init*/

    /*Open Orders Table*/
    ui->OpenOrdersTable->setColumnCount(15);
    ui->OpenOrdersTable->setHorizontalHeaderLabels(QStringList() << "OrderId"
                                                                 << "Date Time"
                                                                 << "Exchange"
                                                                 << "OrderType"
                                                                 << "Limit"
                                                                 << "QTY"
                                                                 << "QTY_Rem"
                                                                 << "Price"
                                                                 << "PricePerUnit"
                                                                 << "CancelInitiated"
                                                                 << "Conditional"
                                                                 << "Condition"
                                                                 << "Condition Target"
                                                                 << "ImmediateOrCancel"
                                                                 << "Cancel Order");
    ui->OpenOrdersTable->setRowCount(0);
    ui->OpenOrdersTable->setColumnHidden(0, true);
    ui->OpenOrdersTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->OpenOrdersTable->horizontalHeader()->setStyleSheet("QHeaderView::section, QHeaderView::section * {font-weight :bold;}");

    ui->OpenOrdersTable->horizontalHeader()->resizeSection(1, 120); // column 1, width 50
    ui->OpenOrdersTable->horizontalHeader()->resizeSection(2, 70);
    ui->OpenOrdersTable->horizontalHeader()->resizeSection(3, 70);
    ui->OpenOrdersTable->horizontalHeader()->resizeSection(4, 70);
    ui->OpenOrdersTable->horizontalHeader()->resizeSection(5, 70);
    ui->OpenOrdersTable->horizontalHeader()->resizeSection(6, 70);
    ui->OpenOrdersTable->horizontalHeader()->resizeSection(7, 70);

    ui->OpenOrdersTable->setColumnHidden(9, true);
    ui->OpenOrdersTable->setColumnHidden(10, true);
    ui->OpenOrdersTable->setColumnHidden(11, true);
    ui->OpenOrdersTable->setColumnHidden(12, true);
    ui->OpenOrdersTable->setColumnHidden(13, true);

    connect(ui->OpenOrdersTable, SIGNAL(cellClicked(int, int)), this, SLOT(CancelOrderSlot(int, int)));
    /*Open Orders Table*/

    /*populate static combo values*/
    ui->BuyBidcomboBox->addItems(QStringList() << "Last"
                                               << "Bid"
                                               << "Ask");
    ui->buyOrdertypeCombo->addItems(QStringList() << "Limit"
                                                  << "Market");
    ui->SellBidcomboBox->addItems(QStringList() << "Last"
                                                << "Bid"
                                                << "Ask");
    ui->SellOrdertypeCombo->addItems(QStringList() << "Limit"
                                                   << "Market");
    //ui->BuyTimeInForceCombo-> addItems(QStringList()<<"Good 'Til Cancelled"<<"Immediate Or Cancel");
    //ui->BuyConditionCombo->   addItems(QStringList()<<"Greater Than Or Equal To"<<"Less Than Or Equal To");
    //ui->BuyConditionCombo->hide();
    //ui->BuyWhenPriceLabel->hide();
    //ui->ConditionLineEdit->hide();
    /*populate static combo values*/
}

void tradingDialog::InitTrading()
{
    //todo - add internet connection/socket error checking.

    //Get default exchange info for the qlabels
    UpdaterFunction();
    // qDebug() << "Updater called";
    if (this->timerid == 0) {
        //Timer is not set,lets create one.
        this->timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(UpdaterFunction()));
        this->timer->start(5000);
        this->timerid = this->timer->timerId();
    }
}


void tradingDialog::enableTradingWindow(bool state)
{
    if (state){
        
        InitTrading();
    }
    else
        ui->enable->setChecked(false);
}

void tradingDialog::UpdaterFunction()
{
    //First get the main exchange info in order to populate qLabels in maindialog. then get data
    //required for the current tab.
    if(ui->enable->isChecked())
    {
        int Retval = SetExchangeInfoTextLabels();

        if (Retval == 0) {
            ActionsOnSwitch(-1);
        }
    }
    
}

QString tradingDialog::GetNonce()
{
    // There must be a better way, lol.
    QString str = "";
    QDateTime currentDateTime = QDateTime::currentDateTime();
    qint64 nonce = currentDateTime.currentMSecsSinceEpoch();
    QString Response = str.setNum(nonce / 1000);
    return Response;
}

QString tradingDialog::GetMarketSummary()
{
    QString Response = sendRequest("https://bittrex.com/api/v1.1/public/getmarketsummary?market=btc-liberta");
    return dequote(Response);
}

QString tradingDialog::GetMarketHistory()
{
    QString Response = sendRequest("https://bittrex.com/api/v1.1/public/getmarkethistory?market=btc-liberta");
    return dequote(Response);
}

QString tradingDialog::GetOrderBook()
{
    QString Response = sendRequest("https://bittrex.com/api/v1.1/public/getorderbook?market=btc-liberta&type=both&depth=50");
    return dequote(Response);
}

QString tradingDialog::GetBalance(QString Currency)
{
    QString URL = "https://bittrex.com/api/v1.1/account/getbalance?apikey=";
    URL += this->ApiKey;
    URL += "&nonce=",
        URL += tradingDialog::GetNonce(),
        URL += "&currency=";
    URL += Currency;
    // QMessageBox::information(this,"URL",URL);

    QString Response = sendRequest(URL);
    return dequote(Response);
}

QString tradingDialog::GetOpenOrders()
{
    QString URL = "https://bittrex.com/api/v1.1/market/getopenorders?apikey=";
    URL += this->ApiKey;
    URL += "&nonce=",
        URL += tradingDialog::GetNonce(),
        URL += "&market=btc-liberta";

    QString Response = sendRequest(URL);
    return dequote(Response);
}

QString tradingDialog::BuyLBT(QString OrderType, double Quantity, double Rate)
{
    QString str = "";
    QString URL = "https://bittrex.com/api/v1.1/market/";
    URL += OrderType;
    URL += "?apikey=";
    URL += this->ApiKey;
    URL += "&nonce=",
        URL += tradingDialog::GetNonce(),
        URL += "&market=btc-liberta&quantity=";
    URL += str.number(Quantity, 'i', 8);
    URL += "&rate=";
    URL += str.number(Rate, 'i', 8);

    QString Response = sendRequest(URL);
    return dequote(Response);
}

QString tradingDialog::SellLBT(QString OrderType, double Quantity, double Rate)
{
    QString str = "";
    QString URL = "https://bittrex.com/api/v1.1/market/";
    URL += OrderType;
    URL += "?apikey=";
    URL += this->ApiKey;
    URL += "&nonce=",
        URL += tradingDialog::GetNonce(),
        URL += "&market=btc-liberta&quantity=";
    URL += str.number(Quantity, 'i', 8);
    URL += "&rate=";
    URL += str.number(Rate, 'i', 8);

    QString Response = sendRequest(URL);
    return dequote(Response);
}

QString tradingDialog::CancelOrder(QString OrderId)
{
    QString URL = "https://bittrex.com/api/v1.1/market/cancel?apikey=";
    URL += this->ApiKey;
    URL += "&nonce=",
        URL += tradingDialog::GetNonce(),
        URL += "&uuid=";
    URL += OrderId;

    QString Response = sendRequest(URL);
    return dequote(Response);
}

QString tradingDialog::GetDepositAddress()
{
    QString URL = "https://bittrex.com/api/v1.1/account/getdepositaddress?apikey=";
    URL += this->ApiKey;
    URL += "&nonce=",
        URL += tradingDialog::GetNonce(),
        URL += "&currency=LBT";

    QString Response = sendRequest(URL);
    return dequote(Response);
}

QString tradingDialog::GetAccountHistory()
{
    QString URL = "https://bittrex.com/api/v1.1/account/getorderhistory?apikey=";
    URL += this->ApiKey;
    URL += "&nonce=",
        URL += tradingDialog::GetNonce(),
        URL += "&market=btc-liberta";

    QString Response = sendRequest(URL);
    return dequote(Response);
}

int tradingDialog::SetExchangeInfoTextLabels()
{
    //Get the current exchange information + information for the current open tab if required.
    QString str = "";
    QString Response = GetMarketSummary();

    //Set the labels, parse the json result to get values.
    QJsonObject obj = GetResultObjectFromJSONArray(Response);

    //set labels to richtext to use css.
    ui->Bid->setTextFormat(Qt::RichText);
    ui->Ask->setTextFormat(Qt::RichText);
    ui->volumet->setTextFormat(Qt::RichText);
    ui->volumebtc->setTextFormat(Qt::RichText);

    ui->Ask->setText("<b>Ask:</b> <span style='font-weight:bold; font-size:11px; color:Red'>" + str.number(obj["Ask"].toDouble(), 'i', 8) + "</span> BTC");

    ui->Bid->setText("<b>Bid:</b> <span style='font-weight:bold; font-size:11px; color:Green;'>" + str.number(obj["Bid"].toDouble(), 'i', 8) + "</span> BTC");

    ui->volumet->setText("<b>LBT Volume:</b> <span style='font-weight:bold; font-size:11px; color:blue;'>" + str.number(obj["Volume"].toDouble(), 'i', 8) + "</span> LBT");

    ui->volumebtc->setText("<b>BTC Volume:</b> <span style='font-weight:bold; font-size:11px; color:blue;'>" + str.number(obj["BaseVolume"].toDouble(), 'i', 8) + "</span> BTC");

    obj.empty();

    return 0;
}

void tradingDialog::CreateOrderBookTables(QTableWidget& Table, QStringList TableHeader)
{
    Table.setColumnCount(3);
    Table.verticalHeader()->setVisible(false);

    Table.setHorizontalHeaderLabels(TableHeader);

    int Cellwidth = Table.width() / 3;

    Table.horizontalHeader()->resizeSection(1, Cellwidth); // column 1, width 50
    Table.horizontalHeader()->resizeSection(2, Cellwidth);
    Table.horizontalHeader()->resizeSection(3, Cellwidth);

    Table.setRowCount(0);

    Table.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    Table.horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    Table.horizontalHeader()->setStyleSheet("QHeaderView::section, QHeaderView::section * { font-weight :bold;}");
}

void tradingDialog::DisplayBalance(QLabel& BalanceLabel, QLabel& Available, QLabel& Pending, QString Currency, QString Response)
{
    QString str;

    BalanceLabel.setTextFormat(Qt::RichText);
    Available.setTextFormat(Qt::RichText);
    Pending.setTextFormat(Qt::RichText);

    //Set the labels, parse the json result to get values.
    QJsonObject ResultObject = GetResultObjectFromJSONObject(Response);

    BalanceLabel.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number(ResultObject["Balance"].toDouble(), 'i', 8) + "</span> " + Currency);
    Available.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number(ResultObject["Available"].toDouble(), 'i', 8) + "</span> " + Currency);
    Pending.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number(ResultObject["Pending"].toDouble(), 'i', 8) + "</span> " + Currency);
}

void tradingDialog::ParseAndPopulateOpenOrdersTable(QString Response)
{
    int itteration = 0, RowCount = 0;

    QJsonArray jsonArray = GetResultArrayFromJSONObject(Response);
    QJsonObject obj;

    ui->OpenOrdersTable->setRowCount(0);

    Q_FOREACH (const QJsonValue& value, jsonArray) {
        QString str = "";
        obj = value.toObject();

        RowCount = ui->OpenOrdersTable->rowCount();

        QString ios;
        QString IsConditional;
        QString ConditionTarget;
        QString CancelInitiated;

        obj["ImmediateOrCancel"].toBool() == true ? (ios = "true") : (ios = "false");
        obj["IsConditional"].toBool() == true ? (IsConditional = "true") : (IsConditional = "false");
        obj["ConditionTarget"].toBool() == true ? (ConditionTarget = "true") : (ConditionTarget = "false");
        obj["CancelInitiated"].toBool() == true ? (CancelInitiated = "true") : (CancelInitiated = "false");

        ui->OpenOrdersTable->insertRow(RowCount);
        ui->OpenOrdersTable->setItem(itteration, 0, new QTableWidgetItem(obj["OrderUuid"].toString()));
        ui->OpenOrdersTable->setItem(itteration, 1, new QTableWidgetItem(BittrexTimeStampToReadable(obj["Opened"].toString())));
        ui->OpenOrdersTable->setItem(itteration, 2, new QTableWidgetItem(obj["Exchange"].toString()));
        ui->OpenOrdersTable->setItem(itteration, 3, new QTableWidgetItem(obj["OrderType"].toString()));
        ui->OpenOrdersTable->setItem(itteration, 4, new QTableWidgetItem(str.number(obj["Limit"].toDouble(), 'i', 8)));
        ui->OpenOrdersTable->setItem(itteration, 5, new QTableWidgetItem(str.number(obj["Quantity"].toDouble(), 'i', 8)));
        ui->OpenOrdersTable->setItem(itteration, 6, new QTableWidgetItem(str.number(obj["QuantityRemaining"].toDouble(), 'i', 8)));
        ui->OpenOrdersTable->setItem(itteration, 7, new QTableWidgetItem(str.number(obj["Price"].toDouble(), 'i', 8)));
        ui->OpenOrdersTable->setItem(itteration, 8, new QTableWidgetItem(str.number(obj["PricePerUnit"].toDouble(), 'i', 8)));
        ui->OpenOrdersTable->setItem(itteration, 9, new QTableWidgetItem(CancelInitiated));
        ui->OpenOrdersTable->setItem(itteration, 10, new QTableWidgetItem(IsConditional));
        ui->OpenOrdersTable->setItem(itteration, 11, new QTableWidgetItem(obj["Condition"].toString()));
        ui->OpenOrdersTable->setItem(itteration, 12, new QTableWidgetItem(ConditionTarget));
        ui->OpenOrdersTable->setItem(itteration, 13, new QTableWidgetItem(ios));
        ui->OpenOrdersTable->setItem(itteration, 14, new QTableWidgetItem(tr("Cancel Order")));

        //Handle the cancel link in open orders table
        QTableWidgetItem* CancelCell;
        CancelCell = ui->OpenOrdersTable->item(itteration, 14);     //Set the wtablewidget item to the cancel cell item.
        CancelCell->setForeground(QColor::fromRgb(255, 0, 0, 127)); //make this item red.
        CancelCell->setTextAlignment(Qt::AlignCenter);
    }
}

void tradingDialog::CancelOrderSlot(int row, int col)
{
    QString OrderId = ui->OpenOrdersTable->model()->data(ui->OpenOrdersTable->model()->index(row, 0)).toString();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Cancel Order", "Are you sure you want to cancel the order?", QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QString Response = CancelOrder(OrderId);

        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());
        QJsonObject ResponseObject = jsonResponse.object();

        if (ResponseObject["success"].toBool() == false) {
            QMessageBox::information(this, "Failed To Cancel Order", ResponseObject["message"].toString());

        } else if (ResponseObject["success"].toBool() == true) {
            ui->OpenOrdersTable->model()->removeRow(row);
            QMessageBox::information(this, "Success", "You're order was cancelled.");
        }
    } else {
        // qDebug() << "Do Nothing";
    }
}

void tradingDialog::ParseAndPopulateAccountHistoryTable(QString Response)
{
    int itteration = 0, RowCount = 0;

    QJsonArray jsonArray = GetResultArrayFromJSONObject(Response);
    QJsonObject obj;

    ui->TradeHistoryTable->setRowCount(0);

    Q_FOREACH (const QJsonValue& value, jsonArray) {
        QString str = "";
        obj = value.toObject();

        RowCount = ui->TradeHistoryTable->rowCount();

        QString ios;
        QString IsConditional;
        QString ConditionTarget;

        obj["ImmediateOrCancel"].toBool() == true ? (ios = "true") : (ios = "false");
        obj["IsConditional"].toBool() == true ? (IsConditional = "true") : (IsConditional = "false");
        obj["ConditionTarget"].toBool() == true ? (ConditionTarget = "true") : (ConditionTarget = "false");

        ui->TradeHistoryTable->insertRow(RowCount);
        ui->TradeHistoryTable->setItem(itteration, 0, new QTableWidgetItem(BittrexTimeStampToReadable(obj["TimeStamp"].toString())));
        ui->TradeHistoryTable->setItem(itteration, 1, new QTableWidgetItem(obj["Exchange"].toString()));
        ui->TradeHistoryTable->setItem(itteration, 2, new QTableWidgetItem(obj["OrderType"].toString()));
        ui->TradeHistoryTable->setItem(itteration, 3, new QTableWidgetItem(str.number(obj["Limit"].toDouble(), 'i', 8)));
        ui->TradeHistoryTable->setItem(itteration, 4, new QTableWidgetItem(str.number(obj["Quantity"].toDouble(), 'i', 8)));
        ui->TradeHistoryTable->setItem(itteration, 5, new QTableWidgetItem(str.number(obj["QuantityRemaining"].toDouble(), 'i', 8)));
        ui->TradeHistoryTable->setItem(itteration, 6, new QTableWidgetItem(str.number(obj["Price"].toDouble(), 'i', 8)));
        ui->TradeHistoryTable->setItem(itteration, 7, new QTableWidgetItem(str.number(obj["PricePerUnit"].toDouble(), 'i', 8)));
        ui->TradeHistoryTable->setItem(itteration, 8, new QTableWidgetItem(IsConditional));
        ui->TradeHistoryTable->setItem(itteration, 9, new QTableWidgetItem(obj["Condition"].toString()));
        ui->TradeHistoryTable->setItem(itteration, 10, new QTableWidgetItem(ConditionTarget));
        ui->TradeHistoryTable->setItem(itteration, 11, new QTableWidgetItem(ios));
        ui->TradeHistoryTable->setItem(itteration, 12, new QTableWidgetItem(obj["Closed"].toString()));
        itteration++;
    }

    obj.empty();
}

void tradingDialog::ParseAndPopulateOrderBookTables(QString OrderBook)
{
    QString str;
    QJsonObject obj;
    QJsonObject ResultObject = GetResultObjectFromJSONObject(OrderBook);

    int BuyItteration = 0, SellItteration = 0, BidRows = 0, AskRows = 0;

    QJsonArray BuyArray = ResultObject.value("buy").toArray();   //get buy/sell object from result object
    QJsonArray SellArray = ResultObject.value("sell").toArray(); //get buy/sell object from result object

    double LBTSupply = 0;
    double LBTDemand = 0;
    double BtcSupply = 0;
    double BtcDemand = 0;

    ui->AsksTable->setRowCount(0);

    Q_FOREACH (const QJsonValue& value, SellArray) {
        obj = value.toObject();

        double x = obj["Rate"].toDouble(); //would like to use int64 here
        double y = obj["Quantity"].toDouble();
        double a = (x * y);

        LBTSupply = LBTSupply + y;
        BtcSupply = BtcSupply + a;

        AskRows = ui->AsksTable->rowCount();
        ui->AsksTable->insertRow(AskRows);

        ui->AsksTable->setItem(SellItteration, 0, new QTableWidgetItem(str.number(x, 'i', 8)));
        ui->AsksTable->setItem(SellItteration, 1, new QTableWidgetItem(str.number(y, 'i', 8)));
        ui->AsksTable->setItem(SellItteration, 2, new QTableWidgetItem(str.number(a, 'i', 8)));
        SellItteration++;
    }

    ui->BidsTable->setRowCount(0);

    Q_FOREACH (const QJsonValue& value, BuyArray) {
        obj = value.toObject();

        double x = obj["Rate"].toDouble(); //would like to use int64 here
        double y = obj["Quantity"].toDouble();
        double a = (x * y);

        LBTDemand = LBTDemand + y;
        BtcDemand = BtcDemand + a;

        BidRows = ui->BidsTable->rowCount();
        ui->BidsTable->insertRow(BidRows);
        ui->BidsTable->setItem(BuyItteration, 0, new QTableWidgetItem(str.number(a, 'i', 8)));
        ui->BidsTable->setItem(BuyItteration, 1, new QTableWidgetItem(str.number(y, 'i', 8)));
        ui->BidsTable->setItem(BuyItteration, 2, new QTableWidgetItem(str.number(x, 'i', 8)));
        BuyItteration++;
    }

    ui->LBTSupply->setText("<b>Supply:</b> <span style='font-weight:bold; font-size:11px; color:blue'>" + str.number(LBTSupply, 'i', 8) + "</span><b> LBT</b>");
    ui->BtcSupply->setText("<span style='font-weight:bold; font-size:11px; color:blue'>" + str.number(BtcSupply, 'i', 8) + "</span><b> BTC</b>");
    ui->AsksCount->setText("<b>#Asks :</b> <span style='font-weight:bold; font-size:11px; color:blue'>" + str.number(ui->AsksTable->rowCount()) + "</span>");


    ui->LBTDemand->setText("<b>Demand:</b> <span style='font-weight:bold; font-size:11px; color:blue'>" + str.number(LBTDemand, 'i', 8) + "</span><b> LBT</b>");
    ui->BtcDemand->setText("<span style='font-weight:bold; font-size:11px; color:blue'>" + str.number(BtcDemand, 'i', 8) + "</span><b> BTC</b>");
    ui->BidsCount->setText("<b>#Bids :</b> <span style='font-weight:bold; font-size:11px; color:blue'>" + str.number(ui->BidsTable->rowCount()) + "</span>");
    obj.empty();
}

void tradingDialog::ParseAndPopulateMarketHistoryTable(QString Response)
{
    int counter = 0, RowCount = 0;
    QJsonArray jsonArray = GetResultArrayFromJSONObject(Response);
    QJsonObject obj;

    ui->MarketHistoryTable->setRowCount(0);

    Q_FOREACH (const QJsonValue& value, jsonArray) {
        QString str = "";
        obj = value.toObject();

        RowCount = ui->MarketHistoryTable->rowCount();

        ui->MarketHistoryTable->insertRow(RowCount);
        ui->MarketHistoryTable->setItem(counter, 0, new QTableWidgetItem(BittrexTimeStampToReadable(obj["TimeStamp"].toString())));
        ui->MarketHistoryTable->setItem(counter, 1, new QTableWidgetItem(obj["OrderType"].toString()));
        ui->MarketHistoryTable->setItem(counter, 2, new QTableWidgetItem(str.number(obj["Price"].toDouble(), 'i', 8)));
        ui->MarketHistoryTable->setItem(counter, 3, new QTableWidgetItem(str.number(obj["Quantity"].toDouble(), 'i', 8)));
        ui->MarketHistoryTable->setItem(counter, 4, new QTableWidgetItem(str.number(obj["Total"].toDouble(), 'i', 8)));
        ui->MarketHistoryTable->item(counter, 1)->setBackgroundColor((obj["OrderType"] == QStringLiteral("BUY")) ? (QColor(0, 205, 0, 127)) : (QColor(255, 99, 71, 127)));
        counter++;
    }
    obj.empty();
}

void tradingDialog::ActionsOnSwitch(int index = -1)
{
    QString Response = "";

    if (index == -1)
        index = ui->TradingTabWidget->currentIndex();

    switch (index) {
    case 0: // Order book tab is the current tab - update the info
        Response = GetOrderBook();
        if (Response.size() > 0 && Response != "Error") {
            ParseAndPopulateOrderBookTables(Response);
        }
        break;

    case 1: // Market history tab
        Response = GetMarketHistory();
        if (Response.size() > 0 && Response != "Error") {
            ParseAndPopulateMarketHistoryTable(Response);
        }
        break;

    case 2: // Open orders tab
        Response = GetOpenOrders();
        if (Response.size() > 0 && Response != "Error") {
            ParseAndPopulateOpenOrdersTable(Response);
        }
        break;

    case 3: // Account history tab
        Response = GetAccountHistory();
        if (Response.size() > 0 && Response != "Error") {
            ParseAndPopulateAccountHistoryTable(Response);
        }
        break;

    case 4: // Buy tab is active
        Response = GetMarketSummary();
        if (Response.size() > 0 && Response != "Error") {
            QString balance = GetBalance("BTC");

            QString str;
            QJsonObject ResultObject = GetResultObjectFromJSONObject(balance);

            ui->BtcAvailableLbl->setText(str.number(ResultObject["Available"].toDouble(), 'i', 8));
        }
        break;

    case 5: // Sell tab is active
        Response = GetMarketSummary();
        if (Response.size() > 0 && Response != "Error") {
            QString balance = GetBalance("LBT");
            QString str;
            QJsonObject ResultObject = GetResultObjectFromJSONObject(balance);

            ui->LBTAvailableLabel->setText(str.number(ResultObject["Available"].toDouble(), 'i', 8));
        }
        break;

    case 6: // Show balance tab
        Response = GetBalance("BTC");
        if (Response.size() > 0 && Response != "Error") {
            DisplayBalance(*ui->LibertaBalanceLabel, *ui->LibertaAvailableLabel, *ui->LibertaPendingLabel, QString::fromUtf8("BTC"), Response);
        }

        Response = GetBalance("LBT");

        if (Response.size() > 0 && Response != "Error") {
            DisplayBalance(*ui->LBTBalanceLabel, *ui->LBTAvailableLabel, *ui->LBTPendingLabel, QString::fromUtf8("LBT"), Response);
        }
        break;

    case 7:

        break;
    }
}

void tradingDialog::on_TradingTabWidget_tabBarClicked(int index)
{
    //tab was clicked, interrupt the timer and restart after action completed.

    // qDebug() << "Stopping timer";
    if (this->timerid != 0) {
        this->timer->stop();
    }
    // qDebug() << "Switching";
    ActionsOnSwitch(index);

    // qDebug() << "Restarting timer";
    if (this->timerid != 0) {
        this->timer->start();
    }
}

QString tradingDialog::sendRequest(QString url)
{
    QString Response = "";
    QString Secret = this->SecretKey;

    // create custom temporary event loop on stack
    QEventLoop eventLoop;

    // "quit()" the event-loop, when the network request "finished()"
    QNetworkAccessManager mgr;
    QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

    // the HTTP request
    QNetworkRequest req = QNetworkRequest(QUrl(url));

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    //make this conditional,depending if we are using private api call
    req.setRawHeader("apisign", HMAC_SHA512_SIGNER(url, Secret).toStdString().c_str()); //set header for bittrex

    QNetworkReply* reply = mgr.get(req);
    eventLoop.exec(); // blocks stack until "finished()" has been called

    if (reply->error() == QNetworkReply::NoError) {
        //success
        Response = reply->readAll();
        // QMessageBox::information(this,"Success",Response);
        delete reply;
    } else {
        //failure
        // qDebug() << "Failure" <<reply->errorString();
        Response = "Error";
        // QMessageBox::information(this,"Error",reply->errorString());
        delete reply;
    }

    return Response;
}

QString tradingDialog::BittrexTimeStampToReadable(QString DateTime)
{
    //Separate Time and date.
    // int TPos = DateTime.indexOf("T");
    // int sPos = DateTime.indexOf(".");
    int _Pos = DateTime.indexOf("T");
    QDateTime Date = QDateTime::fromString(DateTime.left(_Pos), "yyyy-MM-dd"); //format to convert from
    DateTime.remove(0, _Pos + 1);
    QDateTime Time = QDateTime::fromString(DateTime.right(_Pos), "hh:mm:ss");
    //Reconstruct time and date in our own format, one that QDateTime will recognise.
    QString DisplayDate = Date.toString("dd/MM/yyyy") + " " + Time.toString("hh:mm:ss A"); //formats to convert to

    return DisplayDate;
}

qint64 tradingDialog::BittrexTimeStampToSeconds(QString DateTime)
{
    QDateTime date = QDateTime::fromString(DateTime, "yyyy-MM-dd hh:mm:ss"); //format to convert from
    return date.toTime_t();
}

void tradingDialog::CalculateBuyCostLabel()
{
    double price = ui->BuyBidPriceEdit->text().toDouble();
    double Quantity = ui->UnitsInput->text().toDouble();
    double cost = ((price * Quantity) + ((price * Quantity / 100) * 0.25));

    QString Str = "";
    ui->BuyCostLabel->setText(Str.number(cost, 'i', 8));
}

void tradingDialog::CalculateSellCostLabel()
{
    double price = ui->SellBidPriceEdit->text().toDouble();
    double Quantity = ui->UnitsInputLBT->text().toDouble();
    double cost = ((price * Quantity) - ((price * Quantity / 100) * 0.25));

    QString Str = "";
    ui->SellCostLabel->setText(Str.number(cost, 'i', 8));
}

void tradingDialog::on_UpdateKeys_clicked()
{
    this->ApiKey = ui->ApiKeyInput->text();
    this->SecretKey = ui->SecretKeyInput->text();
    this->Currency = "LBT";


    QJsonDocument jsonResponse = QJsonDocument::fromJson(GetBalance(this->Currency).toUtf8()); //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                                        //get json obj

    if (ResponseObject.value("success").toBool() == false)

    {
        QMessageBox::information(this, "API Configuration Failed", "Api configuration was unsuccesful.");

    } else if (ResponseObject.value("success").toBool() == true) {
        QMessageBox::information(this, "API Configuration Complete", "Api connection has been successfully configured and tested.");
        ui->ApiKeyInput->setEchoMode(QLineEdit::Password);
        ui->SecretKeyInput->setEchoMode(QLineEdit::Password);
        ui->TradingTabWidget->setTabEnabled(0, true);
        ui->TradingTabWidget->setTabEnabled(1, true);
        ui->TradingTabWidget->setTabEnabled(3, true);
        ui->TradingTabWidget->setTabEnabled(4, true);
        ui->TradingTabWidget->setTabEnabled(5, true);
        ui->TradingTabWidget->setTabEnabled(6, true);
    }
}

void tradingDialog::on_GenDepositBTN_clicked()
{
    QString response = GetDepositAddress();
    QJsonObject ResultObject = GetResultObjectFromJSONObject(response);
    ui->DepositAddressLabel->setText(ResultObject["Address"].toString());
}

void tradingDialog::on_Sell_Max_Amount_clicked()
{
    //calculate amount of BTC that can be gained from selling LBT available balance
    QString responseA = GetBalance("LBT");
    QString str;
    QJsonObject ResultObject = GetResultObjectFromJSONObject(responseA);

    double AvailableLBT = ResultObject["Available"].toDouble();

    ui->UnitsInputLBT->setText(str.number(AvailableLBT, 'i', 8));
}

void tradingDialog::on_Buy_Max_Amount_clicked()
{
    //calculate amount of currency than can be brought with the LBT balance available
    QString responseA = GetBalance("BTC");
    QString responseB = GetMarketSummary();
    QString str;

    QJsonObject ResultObject = GetResultObjectFromJSONObject(responseA);
    QJsonObject ResultObj = GetResultObjectFromJSONArray(responseB);

    //Get the Bid ask or last value from combo
    QString value = ui->BuyBidcomboBox->currentText();

    double AvailableBTC = ResultObject["Available"].toDouble();
    double CurrentASK = ResultObj[value].toDouble();
    double Result = (AvailableBTC / CurrentASK);
    double percentofnumber = (Result * 0.0025);

    Result = Result - percentofnumber;
    ui->UnitsInput->setText(str.number(Result, 'i', 8));
}

void tradingDialog::on_buyOrdertypeCombo_activated(const QString& arg1)
{
    if (arg1 == "Conditional") {
        //ui->BuyWhenPriceLabel->show();
        //ui->BuyConditionCombo->show();
        //ui->ConditionLineEdit->show();
        //ui->Conditionlabel->show();
    } else if (arg1 == "Limit") {
        //  ui->BuyWhenPriceLabel->hide();
        //  ui->BuyConditionCombo->hide();
        //  ui->ConditionLineEdit->hide();
        //  ui->Conditionlabel->hide();
    }
}

QJsonObject tradingDialog::GetResultObjectFromJSONObject(QString response)
{
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());       //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                            //get json obj
    QJsonObject ResultObject = ResponseObject.value(QString("result")).toObject(); //get result object

    return ResultObject;
}

QJsonObject tradingDialog::GetResultObjectFromJSONArray(QString response)
{
    QJsonDocument jsonResponsea = QJsonDocument::fromJson(response.toUtf8());
    QJsonObject jsonObjecta = jsonResponsea.object();
    QJsonArray jsonArraya = jsonObjecta["result"].toArray();
    QJsonObject obj;

    Q_FOREACH (const QJsonValue& value, jsonArraya) {
        obj = value.toObject();
    }

    return obj;
}

QJsonArray tradingDialog::GetResultArrayFromJSONObject(QString response)
{
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jsonArray = jsonObject["result"].toArray();

    return jsonArray;
}

QString tradingDialog::HMAC_SHA512_SIGNER(QString UrlToSign, QString Secret)
{
    QString retval = "";

    QByteArray byteArray = UrlToSign.toUtf8();
    const char* URL = byteArray.constData();

    QByteArray byteArrayB = Secret.toUtf8();
    const char* Secretkey = byteArrayB.constData();

    unsigned char* digest;

    // Using sha512 hash engine here.
    digest = HMAC(EVP_sha512(), Secretkey, strlen(Secretkey), (unsigned char*)URL, strlen(URL), NULL, NULL);

    // Be careful of the length of string with the choosen hash engine. SHA1 produces a 20-byte hash value which rendered as 40 characters.
    // Change the length accordingly with your choosen hash engine
    // Update: and 0-terminate it!
    char mdString[129] = {0};

    for (int i = 0; i < 64; i++) {
        sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);
    }
    retval = mdString;
    //qDebug() << "HMAC digest:"<< retval;

    return retval;
}

void tradingDialog::on_SellBidcomboBox_currentIndexChanged(const QString& arg1)
{
    QString response = GetMarketSummary();
    QJsonObject ResultObject = GetResultObjectFromJSONArray(response);
    QString Str;

    //Get the Bid ask or last value from combo
    ui->SellBidPriceEdit->setText(Str.number(ResultObject[arg1].toDouble(), 'i', 8));

    CalculateSellCostLabel(); //update cost
}

void tradingDialog::on_BuyBidcomboBox_currentIndexChanged(const QString& arg1)
{
    QString response = GetMarketSummary();
    QJsonObject ResultObject = GetResultObjectFromJSONArray(response);
    QString Str;

    //Get the Bid ask or last value from combo
    ui->BuyBidPriceEdit->setText(Str.number(ResultObject[arg1].toDouble(), 'i', 8));

    CalculateBuyCostLabel(); //update cost
}

void tradingDialog::on_BuyLBT_clicked()
{
    double Rate;
    double Quantity;

    Rate = ui->BuyBidPriceEdit->text().toDouble();
    Quantity = ui->UnitsInput->text().toDouble();

    QString OrderType = ui->buyOrdertypeCombo->currentText();
    QString Order;

    if (OrderType == "Limit") {
        Order = "buylimit";
    } else if (OrderType == "Market") {
        Order = "buymarket";
    }

    QString Msg = "Are you sure you want to buy ";
    Msg += ui->UnitsInput->text();
    Msg += "LBT @ ";
    Msg += ui->BuyBidPriceEdit->text();
    Msg += " BTC Each";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Buy Order", Msg, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QString Response = BuyLBT(Order, Quantity, Rate);

        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8()); //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                      //get json obj

        if (ResponseObject["success"].toBool() == false) {
            QMessageBox::information(this, "Buy Order Failed", ResponseObject["message"].toString());

        } else if (ResponseObject["success"].toBool() == true) {
            QMessageBox::information(this, "Buy Order Initiated", "You Placed an order");
        }
    } else {
        //do nothing
    }
}

void tradingDialog::on_SellLBTBTN_clicked()
{
    double Rate;
    double Quantity;

    Rate = ui->SellBidPriceEdit->text().toDouble();
    Quantity = ui->UnitsInputLBT->text().toDouble();

    QString OrderType = ui->SellOrdertypeCombo->currentText();
    QString Order;

    if (OrderType == "Limit") {
        Order = "selllimit";
    } else if (OrderType == "Market") {
        Order = "sellmarket";
    }

    QString Msg = "Are you sure you want to Sell ";
    Msg += ui->UnitsInputLBT->text();
    Msg += " LBT @ ";
    Msg += ui->SellBidPriceEdit->text();
    Msg += " BTC Each";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Sell Order", Msg, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QString Response = SellLBT(Order, Quantity, Rate);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8()); //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                      //get json obj

        if (ResponseObject["success"].toBool() == false) {
            QMessageBox::information(this, "Sell Order Failed", ResponseObject["message"].toString());

        } else if (ResponseObject["success"].toBool() == true) {
            QMessageBox::information(this, "Sell Order Initiated", "You Placed an order");
        }
    } else {
        //do nothing
    }
}

void tradingDialog::on_AdvancedView_stateChanged(int arg1)
{
    //Show or hide columns in OpenOrders Table depending on checkbox state
    if (arg1 == 2) {
        ui->OpenOrdersTable->setColumnHidden(9, false);
        ui->OpenOrdersTable->setColumnHidden(10, false);
        ui->OpenOrdersTable->setColumnHidden(11, false);
        ui->OpenOrdersTable->setColumnHidden(12, false);
        ui->OpenOrdersTable->setColumnHidden(13, false);
    } else if (arg1 == 0) {
        ui->OpenOrdersTable->setColumnHidden(9, true);
        ui->OpenOrdersTable->setColumnHidden(10, true);
        ui->OpenOrdersTable->setColumnHidden(11, true);
        ui->OpenOrdersTable->setColumnHidden(12, true);
        ui->OpenOrdersTable->setColumnHidden(13, true);
    }
}

void tradingDialog::on_UnitsInputLBT_textChanged(const QString& arg1)
{
    CalculateSellCostLabel(); //update cost
}

void tradingDialog::on_UnitsInput_textChanged(const QString& arg1)
{
    CalculateBuyCostLabel(); //update cost
}

void tradingDialog::on_BuyBidPriceEdit_textChanged(const QString& arg1)
{
    CalculateBuyCostLabel(); //update cost
}

void tradingDialog::on_SellBidPriceEdit_textChanged(const QString& arg1)
{
    CalculateSellCostLabel();
}

void tradingDialog::setModel(WalletModel* model)
{
    this->model = model;
}

tradingDialog::~tradingDialog()
{
    delete ui;
}
