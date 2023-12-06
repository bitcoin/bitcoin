// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/node.h>
#include <qt/trafficgraphwidget.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>

#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QTimer>

#include <cmath>

#define XMARGIN                 10
#define YMARGIN                 10

#define DEFAULT_SAMPLE_HEIGHT    1.1f

TrafficGraphWidget::TrafficGraphWidget(QWidget *parent) :
    QWidget(parent),
    timer(nullptr),
    fMax(DEFAULT_SAMPLE_HEIGHT),
    nMins(0),
    clientModel(nullptr),
    trafficGraphData(TrafficGraphData::Range_30m)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TrafficGraphWidget::updateRates);
    timer->setInterval(TrafficGraphData::SMALLEST_SAMPLE_PERIOD);
    timer->start();
}

void TrafficGraphWidget::setClientModel(ClientModel *model)
{
    clientModel = model;
    if(model) {
        trafficGraphData.setLastBytes(model->node().getTotalBytesRecv(), model->node().getTotalBytesSent());
    }
}

int TrafficGraphWidget::getGraphRangeMins() const
{
    return nMins;
}


void TrafficGraphWidget::paintPath(QPainterPath &path, const TrafficGraphData::SampleQueue &queue, SampleChooser chooser)
{
    int sampleCount = queue.size();
    if(sampleCount > 0) {
        int h = height() - YMARGIN * 2, w = width() - XMARGIN * 2;
        int x = XMARGIN + w;
        path.moveTo(x, YMARGIN + h);
        for(int i = 0; i < sampleCount; ++i) {
            x = XMARGIN + w - w * i / TrafficGraphData::DESIRED_DATA_SAMPLES;
            int y = YMARGIN + h - (int)(h * chooser(queue.at(i)) / fMax);
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
    QRect drawRect = rect();
    // First draw the border
    painter.fillRect(drawRect, GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BORDER_WIDGET));
    painter.fillRect(drawRect.adjusted(1, 1, -1, -1), GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BACKGROUND_WIDGET));

    if(fMax <= 0.0f) return;

    QColor green = GUIUtil::getThemedQColor(GUIUtil::ThemedColor::GREEN);
    QColor red = GUIUtil::getThemedQColor(GUIUtil::ThemedColor::RED);
    QColor axisCol(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::DEFAULT));
    QColor axisCol2;
    int h = height() - YMARGIN * 2;
    painter.setPen(axisCol);
    painter.drawLine(XMARGIN, YMARGIN + h, width() - XMARGIN, YMARGIN + h);

    // decide what order of magnitude we are
    int base = floor(log10(fMax));
    float val = pow(10.0f, base);
    float val2 = val;

    const QString units     = tr("KB/s");
    const float yMarginText = 2.0;

    // draw lines
    painter.setPen(axisCol);
    for(float y = val; y < fMax; y += val) {
        int yy = YMARGIN + h - h * y / fMax;
        painter.drawLine(XMARGIN, yy, width() - XMARGIN, yy);
    }
    // if we drew 3 or fewer lines, break them up at the next lower order of magnitude
    if(fMax / val <= 3.0f) {
        axisCol2 = axisCol.darker();
        val2 = pow(10.0f, base - 1);
        painter.setPen(axisCol2);
        int count = 1;
        for(float y = val2; y < fMax; y += val2, count++) {
            // don't overwrite lines drawn above
            if(count % 10 == 0)
                continue;
            int yy = YMARGIN + h - h * y / fMax;
            painter.drawLine(XMARGIN, yy, width() - XMARGIN, yy);
        }
    }

    const TrafficGraphData::SampleQueue& queue = trafficGraphData.getCurrentRangeQueueWithAverageBandwidth();

    if(!queue.empty()) {
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath pIn;
        QColor lucentGreen = green;
        lucentGreen.setAlpha(128);

        paintPath(pIn, queue, std::bind(chooseIn, std::placeholders::_1));
        painter.fillPath(pIn, lucentGreen);
        painter.setPen(green);
        painter.drawPath(pIn);

        QPainterPath pOut;
        QColor lucentRed = red;
        lucentRed.setAlpha(128);

        paintPath(pOut, queue, std::bind(chooseOut, std::placeholders::_1));
        painter.fillPath(pOut, lucentRed);
        painter.setPen(red);
        painter.drawPath(pOut);
        painter.setRenderHint(QPainter::Antialiasing, false);
    }

    // draw text
    QRect textRect = painter.boundingRect(QRect(XMARGIN, YMARGIN + h - (h * val / fMax) - yMarginText, 0, 0), Qt::AlignLeft, QString("%1 %2").arg(val).arg(units));
    textRect.translate(0, -textRect.height());
    painter.fillRect(textRect, GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BACKGROUND_WIDGET));
    painter.setPen(axisCol);
    painter.drawText(textRect, Qt::AlignLeft, QString("%1 %2").arg(val).arg(units));
    if(fMax / val <= 3.0f) {
        QRect textRect2 = painter.boundingRect(QRect(XMARGIN, YMARGIN + h - (h * val2 / fMax) - yMarginText, 0, 0), Qt::AlignLeft, QString("%1 %2").arg(val2).arg(units));
        textRect2.translate(0, -textRect2.height());
        painter.fillRect(textRect2, GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BACKGROUND_WIDGET));
        painter.setPen(axisCol2);
        painter.drawText(textRect2, Qt::AlignLeft, QString("%1 %2").arg(val2).arg(units));
    }

    // Draw statistic rect on top of everything else
    const int nPadding = 5;
    const int nMarginStats = 20;
    const QString strTotal = tr("Total");
    const QString strReceived = tr("Received");
    const QString strSent = tr("Sent");
    // Get a bold font for the title and a normal one for the rest
    QFont fontTotal = GUIUtil::getFont(GUIUtil::FontWeight::Bold, false, 16);
    QFont fontInOut = GUIUtil::getFont(GUIUtil::FontWeight::Normal, false, 12);
    // Use font metrics to determine minimum rect sizes depending on the font scale
    QFontMetrics fmTotal(fontTotal);
    QFontMetrics fmInOut(fontInOut);
    const int nSizeMark = fmInOut.height() + 2 * nPadding;
    const int nWidthText = GUIUtil::TextWidth(fmInOut, strReceived) + 2 * nPadding;
    const int nWidthBytes = GUIUtil::TextWidth(fmInOut, "1000 GB") + 2 * nPadding;
    const int nHeightTotals = fmTotal.height() + 2 * nPadding;
    const int nHeightInOut = fmInOut.height() + 2 * nPadding;
    const int nWidthStats = nSizeMark + nWidthText + nWidthBytes + 2 * nPadding;
    const int nHeightStats = nHeightTotals + 2 * nHeightInOut + 2 * nPadding;
    auto addPadding = [&](QRect& rect, int nPadding) {
        rect.adjust(nPadding, nPadding, -nPadding, -nPadding);
    };
    // Create top-level rects
    QRect rectOuter = QRect(drawRect.width() - nWidthStats - nMarginStats, nMarginStats, nWidthStats, nHeightStats);
    QRect rectContent = rectOuter;
    QRect rectContentPadded = rectContent;
    addPadding(rectContentPadded, nPadding);
    QRect rectTotals(rectContentPadded.topLeft(), QSize(rectContentPadded.width(), nHeightTotals));
    QRect rectIn(rectTotals.bottomLeft(), QSize(rectContentPadded.width(), nHeightInOut));
    QRect rectOut(rectIn.bottomLeft(), QSize(rectContentPadded.width(), nHeightInOut));
    // Create subrects for received
    QRect rectInMark(rectIn.topLeft(), QSize(nSizeMark, nSizeMark));
    QRect rectInText(rectInMark.topRight(), QSize(nWidthText, nHeightInOut));
    QRect rectInBytes(rectInText.topRight(), QSize(nWidthBytes, nHeightInOut));
    // Create subrects for sent
    QRect rectOutMark(rectOut.topLeft(), QSize(nSizeMark, nSizeMark));
    QRect rectOutText(rectOutMark.topRight(), QSize(nWidthText, nHeightInOut));
    QRect rectOutBytes(rectOutText.topRight(), QSize(nWidthBytes, nHeightInOut));
    // Add padding where required
    addPadding(rectTotals, nPadding);
    addPadding(rectInMark, 1.6 * nPadding);
    addPadding(rectInText, nPadding);
    addPadding(rectInBytes, nPadding);
    addPadding(rectOutMark, 1.6 * nPadding);
    addPadding(rectOutText, nPadding);
    addPadding(rectOutBytes, nPadding);
    // Finally draw it all
    painter.setPen(QPen(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BORDER_NETSTATS), 1));
    painter.drawRect(rectOuter);
    painter.fillRect(rectContent, GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BACKGROUND_NETSTATS));
    painter.setPen(axisCol);
    painter.setFont(fontTotal);
    painter.drawText(rectTotals, Qt::AlignLeft, strTotal);
    painter.setFont(fontInOut);
    painter.drawText(rectInText, Qt::AlignLeft, strReceived);
    painter.drawText(rectInBytes, Qt::AlignRight, GUIUtil::formatBytes(trafficGraphData.getLastBytesIn()));
    painter.drawText(rectOutText, Qt::AlignLeft, strSent);
    painter.drawText(rectOutBytes, Qt::AlignRight, GUIUtil::formatBytes(trafficGraphData.getLastBytesOut()));
    painter.setPen(green);
    painter.setBrush(green);
    painter.drawEllipse(rectInMark);
    painter.setPen(red);
    painter.setBrush(red);
    painter.drawEllipse(rectOutMark);
}

void TrafficGraphWidget::updateRates()
{
    if(!clientModel) return;

    bool updated = trafficGraphData.update(clientModel->node().getTotalBytesRecv(),clientModel->node().getTotalBytesSent());

    if (updated){
        float tmax = DEFAULT_SAMPLE_HEIGHT;
        for (const TrafficSample& sample : trafficGraphData.getCurrentRangeQueueWithAverageBandwidth()) {
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
        trafficGraphData.setLastBytes(clientModel->node().getTotalBytesRecv(), clientModel->node().getTotalBytesSent());
    }
    update();
}
