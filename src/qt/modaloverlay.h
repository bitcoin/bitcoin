// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MODALOVERLAY_H
#define BITCOIN_QT_MODALOVERLAY_H

#include <QDateTime>
#include <QWidget>

namespace Ui {
    class ModalOverlay;
}

/** Modal overlay to display information about the chain-sync state */
class ModalOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit ModalOverlay(QWidget *parent);
    ~ModalOverlay();

public Q_SLOTS:
    void tipUpdate(int count, const QDateTime& blockDate, double nVerificationProgress);
    void setKnownBestHeight(int count, const QDateTime& blockDate);

    // will show or hide the modal layer
    void showHide(bool hide = false, bool userRequested = false);
    void closeClicked();

protected:
    bool eventFilter(QObject * obj, QEvent * ev);
    bool event(QEvent* ev);

private:
    Ui::ModalOverlay *ui;
    int bestBlockHeight; //best known height (based on the headers)
    QVector<QPair<qint64, double> > blockProcessTime;
    bool layerIsVisible;
    bool userClosed;
};

#endif // BITCOIN_QT_MODALOVERLAY_H
