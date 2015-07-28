// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pulsedialog.h"

#include "bitcoinunits.h"
#include "main.h"
#include "util.h"
#include "utiltime.h"

#include <QGraphicsView>
#include <QLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsTextItem>
#include <QFontDatabase>
#include <QFontDialog>
#include <QtAlgorithms>

PulseDialog::PulseDialog(QWidget *parent) :
    QDialog(parent)
{
    
    std::string bestBlockHeight;
    {
        LOCK(cs_main);
        bestBlockHeight = strprintf("%d", chainActive.Tip()->nHeight);
    }
    
    scene = new QGraphicsScene(this);
    gView = new QGraphicsView(scene);
    gView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
   
    scene->setSceneRect(0,0 , 780, 550);
    
    QVBoxLayout mainLayout;
    mainLayout.addWidget(gView);
    mainLayout.setSpacing(0);
    mainLayout.setMargin(0);
    
    setLayout(&mainLayout);
    LoadFonts();
    
    RegisterValidationInterface(this);
    DrawRecentBlocks();
    DrawRecentTx();
    DrawPeers();
    
    connect(this, SIGNAL(signalToUpdate()), SLOT(UpdateOnMainThread()));
}

PulseDialog::~PulseDialog()
{
    foreach (QGraphicsItem *item, blocksItems)
    {
        QGraphicsScene *aScene = item->scene();
        if(aScene)
            aScene->removeItem(item);
    }
    
    foreach (QGraphicsItem *item, txItems)
    {
        QGraphicsScene *aScene = item->scene();
        if(aScene)
            aScene->removeItem(item);
    }
    
    foreach (QGraphicsItem *item, nodeItems)
    {
        QGraphicsScene *aScene = item->scene();
        if(aScene)
            aScene->removeItem(item);
    }
    
    delete gView;
    delete scene;
    
}

void PulseDialog::SyncTransaction(const CTransaction& tx, const CBlock* pblock)
{
    emit signalToUpdate();
}

void PulseDialog::BlockChecked(const CBlock&, const CValidationState&) {
    emit signalToUpdate();
}

void PulseDialog::UpdateOnMainThread() {
    DrawRecentBlocks();
    DrawRecentTx();
    DrawPeers();
}

void PulseDialog::LoadFonts()
{
    QFontDatabase fDatabase;

    fDatabase.addApplicationFont(":/fonts/BebasNeue-Regular");
    fDatabase.addApplicationFont(":/fonts/BebasNeue-Thin");
    
        QStringList fams = fDatabase.families();
        foreach (const QString &family, fams) {
            std::string utf8_text = family.toUtf8().constData();
            LogPrintf("font: %s\n", utf8_text);
        }
    
    smallFont           = fDatabase.font("Bebas Neue", "Regular", 15);
    smallFontLight      = fDatabase.font("Bebas Neue Thin", "Thin", 15);
    bigFont             = fDatabase.font("Bebas Neue Thin", "Thin", 36);
    subtitleFont        = fDatabase.font("Bebas Neue Thin", "Thin", 48);
    
    tinyFont   = fDatabase.font("Bebas Neue", "Regular", 10);
    tinyFont8   = fDatabase.font("Bebas Neue", "Regular", 8);
}

void PulseDialog::DrawRecentTx()
{
    foreach (QGraphicsItem *item, txItems)
    {
        QGraphicsScene *aScene = item->scene();
        if(aScene)
            aScene->removeItem(item);
    }

    int desiredNumOfMempoolTx = 8;
    std::vector<CTxMemPoolEntry> latestMemPoolTx;
    
    unsigned long memPoolSize = 0;
    {
        LOCK(mempool.cs);
        memPoolSize = mempool.size();
        
        for(int i = desiredNumOfMempoolTx;i>0;i--)
        {
            CTxMemPoolEntry currentNewest;
            BOOST_FOREACH(const PAIRTYPE(uint256, CTxMemPoolEntry)& entry, mempool.mapTx)
            {
                const uint256& hash = entry.first;
                bool found = false;
                BOOST_FOREACH(CTxMemPoolEntry latestTxItem, latestMemPoolTx)
                {
                    if (latestTxItem.GetTx().GetHash() == hash)
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                    continue;
                
                const CTxMemPoolEntry& e = entry.second;
                
                if (e.GetTx().IsCoinBase())
                    continue;
                
                if (e.GetTime() > currentNewest.GetTime())
                    currentNewest = e;
            }
            
            if (currentNewest.GetTime() > 0)
            {
                latestMemPoolTx.push_back(currentNewest);
            }
        }
    }
    
    float currentY = 190.0;
    float deltaX = 0.0;
    
    std::string title = strprintf("Transactions (%d)",memPoolSize);
    QGraphicsTextItem *heightLabel = scene->addText(QString::fromStdString(title),subtitleFont);
    heightLabel->setDefaultTextColor(QColor(55,55,55));
    heightLabel->setPos(QPointF(deltaX+8,currentY));
    txItems.append(heightLabel);
    
    currentY+=55.0;
    BOOST_FOREACH(CTxMemPoolEntry latestTxItem, latestMemPoolTx)
    {
        std::string hexStr = latestTxItem.GetTx().GetHash().GetHex();
        std::string hexStrShort = hexStr.substr(0, 12)+"…";
        int64_t txAge = GetTime() - latestTxItem.GetTime();
        std::string ageStr = strprintf("%d", (int)round(txAge/60.0))+"min";
        if((int)round(txAge/60) <= 0)
            ageStr = "<1min";
        
        QGraphicsPixmapItem *blockIcon = scene->addPixmap(QPixmap(":/icons/transaction"));
        blockIcon->setPos(QPointF(deltaX+13,currentY+9));
        blockIcon->setScale(0.5);
        txItems.append(blockIcon);
        
        QGraphicsRectItem *rectItem = scene->addRect(deltaX+120, currentY+6, 105, 22, QPen(QColor(0, 0, 0)), QBrush(QColor(0,0,0)));
        txItems.append(rectItem);
        
        QGraphicsTextItem *hashText = scene->addText(QString::fromStdString(hexStrShort),smallFontLight);
        hashText->setDefaultTextColor(QColor(55,55,55));
        hashText->setPos(QPointF(deltaX+35,currentY+8));
        txItems.append(hashText);
        
        QGraphicsTextItem *valueText = scene->addText(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, latestTxItem.GetTx().GetValueOut()),smallFont);
        valueText->setDefaultTextColor(QColor(255,255,255));
        valueText->setPos(QPointF(deltaX+225-valueText->boundingRect().width(),currentY+8));
        txItems.append(valueText);
        
//        QGraphicsTextItem *ageText = scene->addText(QString::fromStdString(ageStr),smallFont);
//        ageText->setDefaultTextColor(QColor(55,55,55));
//        ageText->setPos(QPointF(deltaX+290-ageText->boundingRect().width(),currentY+8));
//        txItems.append(ageText);
        
        currentY+=35.0;
    }
}

void PulseDialog::DrawPeers()
{
//    QList<CNodeCombinedStats> cachedNodeStats;
//    /** Column to sort nodes by */
//    int sortColumn;
//    /** Order (ascending or descending) to sort nodes by */
//    Qt::SortOrder sortOrder;
//    /** Index of rows by node ID */
//    std::map<NodeId, int> mapNodeRows;
//    
//    /** Pull a full list of peers from vNodes into our cache */
//    void refreshPeers()
//    {
//        {
//            TRY_LOCK(cs_vNodes, lockNodes);
//            if (!lockNodes)
//            {
//                // skip the refresh if we can't immediately get the lock
//                return;
//            }
//            cachedNodeStats.clear();
//#if QT_VERSION >= 0x040700
//            cachedNodeStats.reserve(vNodes.size());
//#endif
//            BOOST_FOREACH(CNode* pnode, vNodes)
//            {
//                CNodeCombinedStats stats;
//                stats.nodeStateStats.nMisbehavior = 0;
//                stats.nodeStateStats.nSyncHeight = -1;
//                stats.fNodeStateStatsAvailable = false;
//                pnode->copyStats(stats.nodeStats);
//                cachedNodeStats.append(stats);
//            }
//        }
//        
//        // Try to retrieve the CNodeStateStats for each node.
//        {
//            TRY_LOCK(cs_main, lockMain);
//            if (lockMain)
//            {
//                BOOST_FOREACH(CNodeCombinedStats &stats, cachedNodeStats)
//                stats.fNodeStateStatsAvailable = GetNodeStateStats(stats.nodeStats.nodeid, stats.nodeStateStats);
//            }
//        }
//        
//        if (sortColumn >= 0)
//            // sort cacheNodeStats (use stable sort to prevent rows jumping around unneceesarily)
//            qStableSort(cachedNodeStats.begin(), cachedNodeStats.end(), NodeLessThan(sortColumn, sortOrder));
//        
//        // build index map
//        mapNodeRows.clear();
//        int row = 0;
//        BOOST_FOREACH(CNodeCombinedStats &stats, cachedNodeStats)
//        mapNodeRows.insert(std::pair<NodeId, int>(stats.nodeStats.nodeid, row++));
//    }
//
    
    foreach (QGraphicsItem *item, nodeItems)
    {
        QGraphicsScene *aScene = item->scene();
        if(aScene)
            aScene->removeItem(item);
    }
    
    

    
    float currentY = 190.0;
    float deltaX = 420.0;
    
    LOCK(cs_vNodes);
    
    std::string title = strprintf("Nodes (%d)",vNodes.size());
    QGraphicsTextItem *heightLabel = scene->addText(QString::fromStdString(title),subtitleFont);
    heightLabel->setDefaultTextColor(QColor(55,55,55));
    heightLabel->setPos(QPointF(deltaX,currentY));
    nodeItems.append(heightLabel);
    currentY+=55.0;
    
    uint64_t maxSend = 0;
    uint64_t maxRecv = 0;
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(pnode->nSendBytes > maxSend)
            maxSend = pnode->nSendBytes;
        
        if(pnode->nRecvBytes > maxRecv)
            maxRecv = pnode->nRecvBytes;
    }

    float rectHeight = 18;
    float maxBytesSize = 100;
    
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        // every node has a rect for the ip
        QGraphicsRectItem *rectItem = scene->addRect(deltaX+120, currentY, 80, rectHeight, QPen(QColor(0, 0, 0)), QBrush(QColor(0,0,0)));
        txItems.append(rectItem);
        
        QGraphicsTextItem *ipText = scene->addText(QString::fromStdString(pnode->addrName),tinyFont);
        ipText->setDefaultTextColor(QColor(255,255,255));
        ipText->setPos(QPointF(deltaX+122,currentY+2));
        txItems.append(ipText);
        
        int widthRecv = maxBytesSize/maxRecv*pnode->nRecvBytes;
        QGraphicsRectItem *recvItem = scene->addRect(deltaX+120-widthRecv, currentY+5, widthRecv, 2, QPen(QColor(0, 0, 0, 0)), QBrush(QColor(120,20,20)));
        txItems.append(recvItem);
        
        int widthSend = maxBytesSize/maxSend*pnode->nSendBytes;
        QGraphicsRectItem *sentItem = scene->addRect(deltaX+120-widthSend, currentY+12, widthSend, 2, QPen(QColor(0, 0, 0, 0)), QBrush(QColor(20,120,20)));
        txItems.append(sentItem);
        
        QGraphicsTextItem *bytesRecvText = scene->addText(FormatBytes(pnode->nRecvBytes),tinyFont8);
        bytesRecvText->setDefaultTextColor(QColor(120,20,20));
        bytesRecvText->setPos(QPointF(deltaX+120-widthRecv-bytesRecvText->boundingRect().width()-1,currentY-1));
        txItems.append(bytesRecvText);
        
        QGraphicsTextItem *bytesSendText = scene->addText(FormatBytes(pnode->nSendBytes),tinyFont8);
        bytesSendText->setDefaultTextColor(QColor(20,120,20));
        bytesSendText->setPos(QPointF(deltaX+120-widthSend-bytesSendText->boundingRect().width()-1,currentY+7-1));
        txItems.append(bytesSendText);
        
        currentY+=30;
    }
}

QString PulseDialog::FormatBytes(quint64 bytes)
{
    if(bytes < 1024)
        return QString(tr("%1 B")).arg(bytes);
    if(bytes < 1024 * 1024)
        return QString(tr("%1 KB")).arg(bytes / 1024);
    if(bytes < 1024 * 1024 * 1024)
        return QString(tr("%1 MB")).arg(bytes / 1024 / 1024);
    
    return QString(tr("%1 GB")).arg(bytes / 1024 / 1024 / 1024);
}

void PulseDialog::DrawRecentBlocks()
{
    foreach (QGraphicsItem *item, blocksItems)
    {
        QGraphicsScene *aScene = item->scene();
        if(aScene)
            aScene->removeItem(item);
    }
    
    float deltaX = 0.0;
    float smallFontYDelta = 15.0;
    
    float currentY = 20;
    int maxBlocks = 3;
    CBlockIndex *blkIdx = chainActive.Tip();
    do
    {
        std::map<std::string,QGraphicsTextItem> views;
        
        CBlock block;
        if(!ReadBlockFromDisk(block, blkIdx))
        {
            
        }
        
        std::string heightString = strprintf("%d", blkIdx->nHeight);
        std::string nrTxString = strprintf("%ud", (blkIdx->nTx));
        std::string hexStr = block.GetHash().GetHex();
        std::string hexStrShort = "…"+hexStr.substr(hexStr.length()-12);
        
        int64_t ageInSecs = GetTime() - (int64_t)block.nTime;
        std::string timeString = strprintf("%ld", (ageInSecs))+"s";
        
        if(ageInSecs > 3600)
            timeString = "~"+strprintf("%d", (int)round(ageInSecs/3600)+1)+"h";
        else if(ageInSecs > 60)
            timeString = "~"+strprintf("%d", (int)round(ageInSecs/60)+1)+"m";
        else
            timeString = "<"+strprintf("%d", (int)floor(ageInSecs/60)+1)+"m";
        
//        QWidget *newWidget = new QWidget;
//        QGridLayout *layout = new QGridLayout(this);
//        layout->setSpacing(0);
//        layout->setMargin(0);
//        newWidget->setLayout(layout);
        
        QGraphicsTextItem *timeValue = scene->addText(QString::fromStdString(timeString),smallFont);
        timeValue->setDefaultTextColor(QColor(55,55,55));
        timeValue->setPos(QPointF(deltaX+4,currentY+smallFontYDelta));
        blocksItems.append(timeValue);
        
        QGraphicsPixmapItem *blockIcon = scene->addPixmap(QPixmap(":/icons/block"));
        blockIcon->setPos(QPointF(deltaX+44,currentY+9));
        blockIcon->setScale(0.5);
        blocksItems.append(blockIcon);

        QGraphicsTextItem *heightLabel = scene->addText(QString("height"),smallFont);
        heightLabel->setDefaultTextColor(QColor(55,55,55));
        heightLabel->setPos(QPointF(deltaX+85,currentY+smallFontYDelta));
        blocksItems.append(heightLabel);
        
        QGraphicsTextItem *heightValue = scene->addText(QString::fromStdString(heightString),bigFont);
        heightValue->setDefaultTextColor(QColor(55,55,55));
        heightValue->setPos(QPointF(deltaX+210-heightValue->boundingRect().width(),currentY));
        blocksItems.append(heightValue);

        QGraphicsTextItem *sizeLabel = scene->addText(QString("size"),smallFont);
        sizeLabel->setDefaultTextColor(QColor(55,55,55));
        sizeLabel->setPos(QPointF(deltaX+237,currentY+smallFontYDelta));
        blocksItems.append(sizeLabel);
    
        QGraphicsTextItem *sizeValue = scene->addText(FormatBytes(::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)),bigFont);
        sizeValue->setDefaultTextColor(QColor(55,55,55));
        sizeValue->setPos(QPointF(deltaX+347-sizeValue->boundingRect().width(),currentY));
        blocksItems.append(sizeValue);
        
        QGraphicsTextItem *nrTxLabel = scene->addText(QString("transactions"),smallFont);
        nrTxLabel->setDefaultTextColor(QColor(55,55,55));
        nrTxLabel->setPos(QPointF(deltaX+367,currentY+smallFontYDelta));
        blocksItems.append(nrTxLabel);
        
        QGraphicsTextItem *nrTxValue = scene->addText(QString::fromStdString(nrTxString),bigFont);
        nrTxValue->setDefaultTextColor(QColor(55,55,55));
        nrTxValue->setPos(QPointF(deltaX+510-nrTxValue->boundingRect().width(),currentY));
        blocksItems.append(nrTxValue);
        
        QGraphicsTextItem *hashLabel = scene->addText(QString("hash"),smallFont);
        hashLabel->setDefaultTextColor(QColor(55,55,55));
        hashLabel->setPos(QPointF(deltaX+523,currentY+smallFontYDelta));
        blocksItems.append(hashLabel);
        
        QGraphicsTextItem *hashValue = scene->addText(QString::fromStdString(hexStrShort),bigFont);
        hashValue->setDefaultTextColor(QColor(55,55,55));
        hashValue->setPos(QPointF(deltaX+740-hashValue->boundingRect().width(),currentY));
        blocksItems.append(hashValue);
        
        
        currentY+=50.0;
        
        if (maxBlocks > 1)
        {
            QPen myPen = QPen(QColor(200, 200, 200, 255));
            blocksItems.append(scene->addLine(deltaX+25, currentY-7, deltaX+740, currentY-7, myPen));
        }
        
        maxBlocks--;
        if (maxBlocks <= 0)
            break;
        
    } while ((blkIdx = blkIdx->pprev));
}
