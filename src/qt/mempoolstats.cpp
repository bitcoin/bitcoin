// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/mempoolstats.h>
#include <qt/forms/ui_mempoolstats.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <stats/stats.h>

#include <math.h>

static const char *LABEL_FONT = "Arial";
static int LABEL_TITLE_SIZE = 22;
static int LABEL_KV_SIZE = 12;

static const int TEN_MINS = 600;
static const int ONE_HOUR = 3600;
static const int ONE_DAY = ONE_HOUR*24;

static const int LABEL_LEFT_SIZE = 30;
static const int LABEL_RIGHT_SIZE = 30;
static const int GRAPH_PADDING_LEFT = 30+LABEL_LEFT_SIZE;
static const int GRAPH_PADDING_RIGHT = 30+LABEL_RIGHT_SIZE;
static const int GRAPH_PADDING_TOP = 10;
static const int GRAPH_PADDING_TOP_LABEL = 150;
static const int GRAPH_PADDING_BOTTOM = 50;
static const int LABEL_HEIGHT = 15;

void ClickableTextItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_EMIT objectClicked(this);
}

void ClickableTextItem::setEnabled(bool state)
{
    if (state)
        setDefaultTextColor(QColor(15,68,113, 250));
    else
        setDefaultTextColor(QColor(100,100,100, 200));
}

MempoolStats::MempoolStats(QWidget *parent) :
QWidget(parent, Qt::Window),
timeFilter(TEN_MINS),
ui(new Ui::MempoolStats)
{
    ui->setupUi(this);
    if (parent) {
        parent->installEventFilter(this);
        raise();
    }

    // autoadjust font size
    QGraphicsTextItem testText("jY"); //screendesign expected 27.5 pixel in width for this string
    testText.setFont(QFont(LABEL_FONT, LABEL_TITLE_SIZE, QFont::Light));
    LABEL_TITLE_SIZE *= 27.5/testText.boundingRect().width();
    LABEL_KV_SIZE *= 27.5/testText.boundingRect().width();

    scene = new QGraphicsScene();
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if (clientModel)
        drawChart();
}

void MempoolStats::setClientModel(ClientModel *model)
{
    clientModel = model;

    if (model)
        connect(model, SIGNAL(mempoolStatsDidUpdate()), this, SLOT(drawChart()));
}

void MempoolStats::drawChart()
{
    if (!(isVisible() && clientModel))
        return;

    if (!titleItem)
    {
        // create labels (only once)
        titleItem = scene->addText(tr("Mempool Statistics"));
        titleItem->setFont(QFont(LABEL_FONT, LABEL_TITLE_SIZE, QFont::Light));
        titleLine = scene->addLine(0,0,100,100);
        titleLine->setPen(QPen(QColor(100,100,100, 200), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        cbShowMemUsage = new QCheckBox("Dynamic Memory Usage");
        cbShowMemUsage->setChecked(true);
        dynMemUsageSwitch = scene->addWidget(cbShowMemUsage);
        connect(cbShowMemUsage, SIGNAL(stateChanged(int)), this, SLOT(drawChart()));
        cbShowMemUsage->setFont(QFont(LABEL_FONT, LABEL_KV_SIZE, QFont::Light));
        dynMemUsageValueItem = scene->addText("N/A");
        dynMemUsageValueItem->setFont(QFont(LABEL_FONT, LABEL_KV_SIZE, QFont::Bold));

        cbShowNumTxns = new QCheckBox("Amount of Transactions");
        cbShowNumTxns->setChecked(true);
        txCountSwitch = scene->addWidget(cbShowNumTxns);
        scene->addItem(txCountSwitch);
        connect(cbShowNumTxns, SIGNAL(stateChanged(int)), this, SLOT(drawChart()));
        cbShowNumTxns->setFont(QFont(LABEL_FONT, LABEL_KV_SIZE, QFont::Light));
        txCountValueItem = scene->addText("N/A");
        txCountValueItem->setFont(QFont(LABEL_FONT, LABEL_KV_SIZE, QFont::Bold));

        cbShowMinFeerate = new QCheckBox("Min relay fee per kB");
        cbShowMinFeerate->setChecked(false);
        minFeeSwitch = scene->addWidget(cbShowMinFeerate);
        scene->addItem(minFeeSwitch);
        connect(cbShowMinFeerate, SIGNAL(stateChanged(int)), this, SLOT(drawChart()));
        cbShowMinFeerate->setFont(QFont(LABEL_FONT, LABEL_KV_SIZE, QFont::Light));
        minFeeValueItem = scene->addText(tr("N/A"));
        minFeeValueItem->setFont(QFont(LABEL_FONT, LABEL_KV_SIZE, QFont::Bold));

        noDataItem = scene->addText(tr("No Data available"));
        noDataItem->setFont(QFont(LABEL_FONT, LABEL_TITLE_SIZE, QFont::Light));
        noDataItem->setDefaultTextColor(QColor(100,100,100, 200));

        last10MinLabel = new ClickableTextItem(); last10MinLabel->setPlainText(tr("Last 10 min"));
        scene->addItem(last10MinLabel);
        connect(last10MinLabel, SIGNAL(objectClicked(QGraphicsItem*)), this, SLOT(objectClicked(QGraphicsItem*)));
        last10MinLabel->setFont(QFont(LABEL_FONT, LABEL_KV_SIZE, QFont::Light));
        lastHourLabel = new ClickableTextItem(); lastHourLabel->setPlainText(tr("Last Hour"));
        scene->addItem(lastHourLabel);
        connect(lastHourLabel, SIGNAL(objectClicked(QGraphicsItem*)), this, SLOT(objectClicked(QGraphicsItem*)));
        lastHourLabel->setFont(QFont(LABEL_FONT, LABEL_KV_SIZE, QFont::Light));
        lastDayLabel = new ClickableTextItem(); lastDayLabel->setPlainText(tr("Last Day"));
        scene->addItem(lastDayLabel);
        connect(lastDayLabel, SIGNAL(objectClicked(QGraphicsItem*)), this, SLOT(objectClicked(QGraphicsItem*)));
        lastDayLabel->setFont(QFont(LABEL_FONT, LABEL_KV_SIZE, QFont::Light));
        allDataLabel = new ClickableTextItem(); allDataLabel->setPlainText(tr("All Data"));
        scene->addItem(allDataLabel);
        connect(allDataLabel, SIGNAL(objectClicked(QGraphicsItem*)), this, SLOT(objectClicked(QGraphicsItem*)));
        allDataLabel->setFont(QFont(LABEL_FONT, LABEL_KV_SIZE, QFont::Light));
    }

    static const QString checkbox_style_base = QStringLiteral("background-color:rgb(255,255,255);");
    cbShowNumTxns->setStyleSheet(cbShowNumTxns->isChecked() ? (checkbox_style_base + "color:rgb(188,49,62);") : checkbox_style_base);
    cbShowMinFeerate->setStyleSheet(cbShowMinFeerate->isChecked() ? (checkbox_style_base + "color:rgb(49,113,62);") : checkbox_style_base);
    cbShowMemUsage->setStyleSheet(cbShowMemUsage->isChecked() ? (checkbox_style_base + "color:rgb(15,68,113);") : checkbox_style_base);

    last10MinLabel->setEnabled((timeFilter == TEN_MINS));
    lastHourLabel->setEnabled((timeFilter == ONE_HOUR));
    lastDayLabel->setEnabled((timeFilter == ONE_DAY));
    allDataLabel->setEnabled((timeFilter == 0));

    // remove the items which needs to be redrawn
    for (QGraphicsItem * item : redrawItems)
    {
        scene->removeItem(item);
        delete item;
    }
    redrawItems.clear();

    // get the samples
    QDateTime toDateTime = QDateTime::currentDateTime();
    QDateTime fromDateTime = toDateTime.addSecs(-timeFilter); //-1h
    if (timeFilter == 0)
    {
        // disable filter if timeFilter == 0
        toDateTime.setSecsSinceEpoch(0);
        fromDateTime.setSecsSinceEpoch(0);
    }

    mempoolSamples_t vSamples = clientModel->getMempoolStatsInRange(fromDateTime, toDateTime);

    // set the values into the overview labels
    if (vSamples.size())
    {
        dynMemUsageValueItem->setPlainText(GUIUtil::formatBytes((uint64_t)vSamples.back().m_dyn_mem_usage));
        txCountValueItem->setPlainText(QString::number(vSamples.back().m_tx_count));
        minFeeValueItem->setPlainText(QString::number(vSamples.back().m_min_fee_per_k));
    }

    // set dynamic label positions
    int maxValueSize = std::max(std::max(txCountValueItem->boundingRect().width(), dynMemUsageValueItem->boundingRect().width()), minFeeValueItem->boundingRect().width());
    maxValueSize = ceil(maxValueSize*0.11)*10; //use size steps of 10dip

    int rightPaddingLabels = std::max(std::max(dynMemUsageSwitch->boundingRect().width(), txCountSwitch->boundingRect().width()), minFeeSwitch->boundingRect().width())+maxValueSize;
    int rightPadding = 10;
    dynMemUsageSwitch->setPos(width()-rightPaddingLabels-rightPadding, 5);

    txCountSwitch->setPos(width()-rightPaddingLabels-rightPadding, dynMemUsageSwitch->pos().y()+dynMemUsageSwitch->boundingRect().height());

    minFeeSwitch->setPos(width()-rightPaddingLabels-rightPadding, txCountSwitch->pos().y()+txCountSwitch->boundingRect().height());

    dynMemUsageValueItem->setPos(width()-dynMemUsageValueItem->boundingRect().width()-rightPadding, dynMemUsageSwitch->pos().y());
    txCountValueItem->setPos(width()-txCountValueItem->boundingRect().width()-rightPadding, txCountSwitch->pos().y());
    minFeeValueItem->setPos(width()-minFeeValueItem->boundingRect().width()-rightPadding, minFeeSwitch->pos().y());

    titleItem->setPos(5,minFeeSwitch->pos().y()+minFeeSwitch->boundingRect().height()-titleItem->boundingRect().height()+10);
    titleLine->setLine(10, titleItem->pos().y()+titleItem->boundingRect().height(), width()-10, titleItem->pos().y()+titleItem->boundingRect().height());

    // center the optional "no data" label
    noDataItem->setPos(width()/2.0-noDataItem->boundingRect().width()/2.0, height()/2.0);

    // set the position of the filter icons
    static const int filterBottomPadding = 30;
    int totalWidth = last10MinLabel->boundingRect().width()+lastHourLabel->boundingRect().width()+lastDayLabel->boundingRect().width()+allDataLabel->boundingRect().width()+30;
    last10MinLabel->setPos((width()-totalWidth)/2.0,height()-filterBottomPadding);
    lastHourLabel->setPos((width()-totalWidth)/2.0+last10MinLabel->boundingRect().width()+10,height()-filterBottomPadding);
    lastDayLabel->setPos((width()-totalWidth)/2.0+last10MinLabel->boundingRect().width()+lastHourLabel->boundingRect().width()+20,height()-filterBottomPadding);
    allDataLabel->setPos((width()-totalWidth)/2.0+last10MinLabel->boundingRect().width()+lastHourLabel->boundingRect().width()+lastDayLabel->boundingRect().width()+30,height()-filterBottomPadding);

    // don't paint the grind/graph if there are no or only a single sample
    if (vSamples.size() < 2)
    {
        noDataItem->setVisible(true);
        return;
    }
    noDataItem->setVisible(false);

    int bottom = ui->graphicsView->size().height()-GRAPH_PADDING_BOTTOM;
    qreal maxwidth = ui->graphicsView->size().width()-GRAPH_PADDING_LEFT-GRAPH_PADDING_RIGHT;
    qreal maxheightG = ui->graphicsView->size().height()-GRAPH_PADDING_TOP-GRAPH_PADDING_TOP_LABEL-LABEL_HEIGHT;
    float paddingTopSizeFactor = 1.2;
    qreal step = maxwidth/(double)vSamples.size();

    // make sure we skip samples that would be drawn narrower then 1px
    // larger window can result in drawing more samples
    int samplesStep = 1;
    if (step < 1)
        samplesStep = ceil(1/samplesStep);

    // find maximum values
    int64_t maxDynMemUsage = 0;
    int64_t minDynMemUsage = std::numeric_limits<int64_t>::max();
    int64_t maxTxCount = 0;
    int64_t minTxCount = std::numeric_limits<int64_t>::max();
    int64_t maxMinFee = 0;
    uint32_t maxTimeDetla = vSamples.back().m_time_delta-vSamples.front().m_time_delta;
    for(const struct CStatsMempoolSample &sample : vSamples)
    {
        if (sample.m_dyn_mem_usage > maxDynMemUsage)
            maxDynMemUsage = sample.m_dyn_mem_usage;

        if (sample.m_dyn_mem_usage < minDynMemUsage)
            minDynMemUsage = sample.m_dyn_mem_usage;

        if (sample.m_tx_count > maxTxCount)
            maxTxCount = sample.m_tx_count;

        if (sample.m_tx_count < minTxCount)
            minTxCount = sample.m_tx_count;

        if (sample.m_min_fee_per_k > maxMinFee)
            maxMinFee = sample.m_min_fee_per_k;
    }

    int64_t dynMemUsagelog10Val = pow(10.0, floor(log10(maxDynMemUsage*paddingTopSizeFactor-minDynMemUsage)));
    int64_t topDynMemUsage = ceil((double)maxDynMemUsage*paddingTopSizeFactor/dynMemUsagelog10Val)*dynMemUsagelog10Val;
    int64_t bottomDynMemUsage = floor((double)minDynMemUsage/dynMemUsagelog10Val)*dynMemUsagelog10Val;

    int64_t txCountLog10Val = pow(10.0, floor(log10(maxTxCount*paddingTopSizeFactor-minTxCount)));
    int64_t topTxCount = ceil((double)maxTxCount*paddingTopSizeFactor/txCountLog10Val)*txCountLog10Val;
    int64_t bottomTxCount = floor((double)minTxCount/txCountLog10Val)*txCountLog10Val;

    qreal currentX = GRAPH_PADDING_LEFT;
    QPainterPath dynMemUsagePath(QPointF(currentX, bottom));
    QPainterPath txCountPath(QPointF(currentX, bottom));
    QPainterPath minFeePath(QPointF(currentX, bottom));

    // draw the three possible paths
    for (mempoolSamples_t::iterator it = vSamples.begin(); it != vSamples.end(); it+=samplesStep)
    {
        const struct CStatsMempoolSample &sample = (*it);
        qreal xPos = maxTimeDetla > 0 ? maxwidth/maxTimeDetla*(sample.m_time_delta-vSamples.front().m_time_delta) : maxwidth/(double)vSamples.size();
        if (sample.m_time_delta == vSamples.front().m_time_delta)
        {
            dynMemUsagePath.moveTo(GRAPH_PADDING_LEFT+xPos, bottom-maxheightG/(topDynMemUsage-bottomDynMemUsage)*(sample.m_dyn_mem_usage-bottomDynMemUsage));
            txCountPath.moveTo(GRAPH_PADDING_LEFT+xPos, bottom-maxheightG/(topTxCount-bottomTxCount)*(sample.m_tx_count-bottomTxCount));
            minFeePath.moveTo(GRAPH_PADDING_LEFT+xPos, bottom-maxheightG/maxMinFee*sample.m_min_fee_per_k);
        }
        else
        {
            dynMemUsagePath.lineTo(GRAPH_PADDING_LEFT+xPos, bottom-maxheightG/(topDynMemUsage-bottomDynMemUsage)*(sample.m_dyn_mem_usage-bottomDynMemUsage));
            txCountPath.lineTo(GRAPH_PADDING_LEFT+xPos, bottom-maxheightG/(topTxCount-bottomTxCount)*(sample.m_tx_count-bottomTxCount));
            minFeePath.lineTo(GRAPH_PADDING_LEFT+xPos, bottom-maxheightG/maxMinFee*sample.m_min_fee_per_k);
        }
    }

    // copy the path for the fill
    QPainterPath dynMemUsagePathFill(dynMemUsagePath);

    // close the path for the fill
    dynMemUsagePathFill.lineTo(GRAPH_PADDING_LEFT+maxwidth, bottom);
    dynMemUsagePathFill.lineTo(GRAPH_PADDING_LEFT, bottom);

    QPainterPath dynMemUsageGridPath(QPointF(currentX, bottom));

    // draw horizontal grid
    int amountOfLinesH = 5;
    QFont gridFont;
    gridFont.setPointSize(8);
    for (int i=0; i < amountOfLinesH; i++)
    {
        qreal lY = bottom-i*(maxheightG/(amountOfLinesH-1));
        dynMemUsageGridPath.moveTo(GRAPH_PADDING_LEFT, lY);
        dynMemUsageGridPath.lineTo(GRAPH_PADDING_LEFT+maxwidth, lY);

        size_t gridDynSize = (float)i*(topDynMemUsage-bottomDynMemUsage)/(amountOfLinesH-1) + bottomDynMemUsage;
        size_t gridTxCount = (float)i*(topTxCount-bottomTxCount)/(amountOfLinesH-1) + bottomTxCount;

        QGraphicsTextItem *itemDynSize = scene->addText(GUIUtil::formatBytes(gridDynSize), gridFont);
        QGraphicsTextItem *itemTxCount = scene->addText(QString::number(gridTxCount), gridFont);

        itemDynSize->setPos(GRAPH_PADDING_LEFT-itemDynSize->boundingRect().width(), lY-(itemDynSize->boundingRect().height()/2));
        itemTxCount->setPos(GRAPH_PADDING_LEFT+maxwidth, lY-(itemDynSize->boundingRect().height()/2));
        redrawItems.append(itemDynSize);
        redrawItems.append(itemTxCount);
    }

    // draw vertical grid
    int amountOfLinesV = 4;
    QDateTime drawTime(fromDateTime);
    std::string fromS = fromDateTime.toString().toStdString();
    std::string toS = toDateTime.toString().toStdString();
    qint64 secsTotal = fromDateTime.secsTo(toDateTime);
    for (int i=0; i <= amountOfLinesV; i++)
    {
        qreal lX = i*(maxwidth/(amountOfLinesV));
        dynMemUsageGridPath.moveTo(GRAPH_PADDING_LEFT+lX, bottom);
        dynMemUsageGridPath.lineTo(GRAPH_PADDING_LEFT+lX, bottom-maxheightG);

        QGraphicsTextItem *item = scene->addText(drawTime.toString("HH:mm"), gridFont);
        item->setPos(GRAPH_PADDING_LEFT+lX-(item->boundingRect().width()/2), bottom);
        redrawItems.append(item);
        qint64 step = secsTotal/amountOfLinesV;
        drawTime = drawTime.addSecs(step);
    }

    // materialize path
    QPen gridPen(QColor(100,100,100, 200), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    redrawItems.append(scene->addPath(dynMemUsageGridPath, gridPen));

    // draw semi-transparent gradient for the dynamic memory size fill
    QLinearGradient gradient(currentX, bottom, currentX, 0);
    gradient.setColorAt(1.0, QColor(15,68,113, 250));
    gradient.setColorAt(0, QColor(255,255,255,0));
    QBrush graBru(gradient);

    QPen linePenBlue(QColor(15,68,113, 250), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen linePenRed(QColor(188,49,62, 250), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen linePenGreen(QColor(49,188,62, 250), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    if (cbShowNumTxns->isChecked())
        redrawItems.append(scene->addPath(txCountPath, linePenRed));
    if (cbShowMinFeerate->isChecked())
        redrawItems.append(scene->addPath(minFeePath, linePenGreen));
    if (cbShowMemUsage->isChecked())
    {
        redrawItems.append(scene->addPath(dynMemUsagePath, linePenBlue));
        redrawItems.append(scene->addPath(dynMemUsagePathFill, QPen(Qt::NoPen), graBru));
    }
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void MempoolStats::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    ui->graphicsView->resize(size());
    ui->graphicsView->scene()->setSceneRect(rect());
    drawChart();
}

void MempoolStats::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (clientModel)
        drawChart();
}

void MempoolStats::objectClicked(QGraphicsItem *item)
{
    if (item == last10MinLabel)
        timeFilter = 600;

    if (item == lastHourLabel)
        timeFilter = 3600;

    if (item == lastDayLabel)
        timeFilter = 24*3600;

    if (item == allDataLabel)
        timeFilter = 0;

    drawChart();
}

MempoolStats::~MempoolStats()
{
    if (titleItem)
    {
        for (QGraphicsItem * item : redrawItems)
        {
            scene->removeItem(item);
            delete item;
        }
        redrawItems.clear();

        delete titleItem;
        delete titleLine;
        delete noDataItem;
        delete dynMemUsageValueItem;
        delete txCountValueItem;
        delete minFeeValueItem;
        delete last10MinLabel;
        delete lastHourLabel;
        delete lastDayLabel;
        delete allDataLabel;
        delete txCountSwitch;
        delete minFeeSwitch;
        delete dynMemUsageSwitch;
        delete scene;
    }
}
