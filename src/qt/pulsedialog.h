// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PULSEDIALOG_H
#define BITCOIN_QT_PULSEDIALOG_H

#include "main.h"
#include "validationinterface.h"

#include <QDialog>
#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTimer>

#include <vector>

namespace Ui {
    class PulseDialog;
}

/** Preferences dialog. */
class PulseDialog : public QDialog, public CValidationInterface
{
    Q_OBJECT
    
public:
    PulseDialog(QWidget *parent = 0);
    ~PulseDialog();
    
    void BlockChecked(const CBlock&, const CValidationState&);
    void SyncTransaction(const CTransaction& tx, const CBlock* pblock);
    void DrawRecentBlocks();

signals:
    void signalToUpdate();
public slots:
    void UpdateOnMainThread();
    
private:
    void LoadFonts();
    void DrawRecentTx();
    void DrawPeers();
    static QString FormatBytes(quint64 bytes);
    
    QGraphicsScene *scene;
    QGraphicsView *gView;
    QTimer *timer;
    
    std::vector<std::map<std::string, QGraphicsTextItem *> > blockViews;
    
    QList<QGraphicsItem *> blocksItems;
    QList<QGraphicsItem *> txItems;
    QList<QGraphicsItem *> nodeItems;
    
    QFont smallFont;
    QFont smallFontLight;
    QFont tinyFont;
    QFont tinyFont8;
    QFont bigFont;
    QFont subtitleFont;
};

#endif // BITCOIN_QT_PULSEDIALOG_H
