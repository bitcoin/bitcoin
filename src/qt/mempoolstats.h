// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MEMPOOLSTATS_H
#define BITCOIN_QT_MEMPOOLSTATS_H

#include <QWidget>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>

#include <QCheckBox>
#include <QGraphicsProxyWidget>

#include <QEvent>

class ClientModel;

class ClickableTextItem : public QGraphicsTextItem
{
    Q_OBJECT
public:
    void setEnabled(bool state);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
Q_SIGNALS:
    void objectClicked(QGraphicsItem*);
};


namespace Ui {
    class MempoolStats;
}

class MempoolStats : public QWidget
{
    Q_OBJECT

public:
    MempoolStats(QWidget *parent = nullptr);
    ~MempoolStats();

    void setClientModel(ClientModel *model);

public Q_SLOTS:
    void drawChart();
    void objectClicked(QGraphicsItem *);

private:
    ClientModel *clientModel{nullptr};

    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;

    QGraphicsTextItem *titleItem{nullptr};
    QGraphicsLineItem *titleLine;
    QGraphicsTextItem *noDataItem;

    QGraphicsTextItem *dynMemUsageValueItem;
    QGraphicsTextItem *txCountValueItem;
    QGraphicsTextItem *minFeeValueItem;

    ClickableTextItem *last10MinLabel;
    ClickableTextItem *lastHourLabel;
    ClickableTextItem *lastDayLabel;
    ClickableTextItem *allDataLabel;

    QGraphicsProxyWidget *txCountSwitch;
    QGraphicsProxyWidget *minFeeSwitch;
    QGraphicsProxyWidget *dynMemUsageSwitch;

    QGraphicsScene *scene{nullptr};
    QVector<QGraphicsItem*> redrawItems;

    QCheckBox *cbShowMemUsage;
    QCheckBox *cbShowNumTxns;
    QCheckBox *cbShowMinFeerate;

    int64_t timeFilter;

    Ui::MempoolStats *ui;
};

#endif // BITCOIN_QT_MEMPOOLSTATS_H
