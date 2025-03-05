// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TRAFFICGRAPHWIDGET_H
#define BITCOIN_QT_TRAFFICGRAPHWIDGET_H

#include <QWidget>
#include <QQueue>

#include <chrono>

class ClientModel;

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QTimer;
QT_END_NAMESPACE

class TrafficGraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrafficGraphWidget(QWidget *parent = nullptr);
    void setClientModel(ClientModel *model);
    std::chrono::minutes getGraphRange() const;

protected:
    void paintEvent(QPaintEvent *) override;
    int y_value(float value);
    void mousePressEvent(QMouseEvent *event) override;
    bool fToggle = true;
    void mouseMoveEvent(QMouseEvent *event) override;
    int ttpoint = -1;
    int x_offset = 0;
    int y_offset = 0;
    int64_t tt_time = 0;

public Q_SLOTS:
    void updateRates();
    void updateToolTip();
    void setGraphRange(std::chrono::minutes new_range);
    void clear();

private:
    void paintPath(QPainterPath &path, QQueue<float> &samples);

    QTimer* timer{nullptr};
    QTimer* tt_timer{nullptr};
    float fMax{0.0f};
    std::chrono::minutes m_range{0};
    QQueue<float> vSamplesIn;
    QQueue<float> vSamplesOut;
    QQueue<int64_t> vTimeStamp;
    quint64 nLastBytesIn{0};
    quint64 nLastBytesOut{0};
    ClientModel* clientModel{nullptr};
};

#endif // BITCOIN_QT_TRAFFICGRAPHWIDGET_H
