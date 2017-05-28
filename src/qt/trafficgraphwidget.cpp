// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "trafficgraphwidget.h"
#include "clientmodel.h"

#include <boost/bind.hpp>

#include <QPainter>
#include <QColor>
#include <QTimer>

#include <cmath>

#define XMARGIN                 10
#define YMARGIN                 10

#define DEFAULT_SAMPLE_HEIGHT    1.1f

TrafficGraphWidget::TrafficGraphWidget(QWidget *parent) :
    QWidget(parent),
    timer(0),
    fMax(DEFAULT_SAMPLE_HEIGHT),
    nMins(0),
    clientModel(0),
    trafficGraphData(TrafficGraphData::Range_30m)
{
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(updateRates()));
    timer->setInterval(TrafficGraphData::SMALLEST_SAMPLE_PERIOD);
    timer->start();
}

void TrafficGraphWidget::setClientModel(ClientModel *model)
{
    clientModel = model;
    if(model) {
        trafficGraphData.setLastBytes(model->getTotalBytesRecv(), model->getTotalBytesSent());
    }
}

int TrafficGraphWidget::getGraphRangeMins() const
{
    return nMins;
}


void TrafficGraphWidget::paintPath(QPainterPath &path, const TrafficGraphData::SampleQueue &queue, SampleChooser chooser)
{
    int h = height() - YMARGIN * 2, w = width() - XMARGIN * 2;
    int sampleCount = queue.size(), x = XMARGIN + w, y;
    if(sampleCount > 0) {
        path.moveTo(x, YMARGIN + h);
        for(int i = 0; i < sampleCount; ++i) {
            x = XMARGIN + w - w * i / TrafficGraphData::DESIRED_DATA_SAMPLES;
            y = YMARGIN + h - (int)(h * chooser(queue.at(i)) / fMax);
            path.lineTo(x, y);
        }
        path.lineTo(x, YMARGIN + h);
    }
}

namespace
{
    float chooseIn(const TrafficSample& sample)
    {
        return sample.in;
    }
    float chooseOut(const TrafficSample& sample)
    {
        return sample.out;
    }
}

void TrafficGraphWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if(fMax <= 0.0f) return;

    QColor axisCol(Qt::gray);
    int h = height() - YMARGIN * 2;
    painter.setPen(axisCol);
    painter.drawLine(XMARGIN, YMARGIN + h, width() - XMARGIN, YMARGIN + h);

    // decide what order of magnitude we are
    int base = floor(log10(fMax));
    float val = pow(10.0f, base);

    const QString units     = tr("KB/s");
    const float yMarginText = 2.0;
    
    // draw lines
    painter.setPen(axisCol);
    painter.drawText(XMARGIN, YMARGIN + h - h * val / fMax-yMarginText, QString("%1 %2").arg(val).arg(units));
    for(float y = val; y < fMax; y += val) {
        int yy = YMARGIN + h - h * y / fMax;
        painter.drawLine(XMARGIN, yy, width() - XMARGIN, yy);
    }
    // if we drew 3 or fewer lines, break them up at the next lower order of magnitude
    if(fMax / val <= 3.0f) {
        axisCol = axisCol.darker();
        val = pow(10.0f, base - 1);
        painter.setPen(axisCol);
        painter.drawText(XMARGIN, YMARGIN + h - h * val / fMax-yMarginText, QString("%1 %2").arg(val).arg(units));
        int count = 1;
        for(float y = val; y < fMax; y += val, count++) {
            // don't overwrite lines drawn above
            if(count % 10 == 0)
                continue;
            int yy = YMARGIN + h - h * y / fMax;
            painter.drawLine(XMARGIN, yy, width() - XMARGIN, yy);
        }
    }

    const TrafficGraphData::SampleQueue& queue = trafficGraphData.getCurrentRangeQueue();

    if(!queue.empty()) {
        QPainterPath pIn;
        paintPath(pIn, queue, boost::bind(chooseIn,_1));
        painter.fillPath(pIn, QColor(0, 255, 0, 128));
        painter.setPen(Qt::green);
        painter.drawPath(pIn);

        QPainterPath pOut;
        paintPath(pOut, queue, boost::bind(chooseOut,_1));
        painter.fillPath(pOut, QColor(255, 0, 0, 128));
        painter.setPen(Qt::red);
        painter.drawPath(pOut);
    }
}

void TrafficGraphWidget::updateRates()
{
    if(!clientModel) return;

    bool updated = trafficGraphData.update(clientModel->getTotalBytesRecv(),clientModel->getTotalBytesSent());

    if (updated){
        float tmax = DEFAULT_SAMPLE_HEIGHT;
        Q_FOREACH(const TrafficSample& sample, trafficGraphData.getCurrentRangeQueue()) {
            if(sample.in > tmax) tmax = sample.in;
            if(sample.out > tmax) tmax = sample.out;
        }
        fMax = tmax;
        update();
    }
}

void TrafficGraphWidget::setGraphRangeMins(int value)
{
    trafficGraphData.switchRange(static_cast<TrafficGraphData::GraphRange>(value));
    update();
}

void TrafficGraphWidget::clear()
{
    trafficGraphData.clear();
    fMax = DEFAULT_SAMPLE_HEIGHT;
    if(clientModel) {
        trafficGraphData.setLastBytes(clientModel->getTotalBytesRecv(), clientModel->getTotalBytesSent());
    }
    update();
}
