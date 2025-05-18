// Copyright (c) 2016-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_QT_MODALOVERLAY_H
#define TORTOISECOIN_QT_MODALOVERLAY_H

#include <QDateTime>
#include <QPropertyAnimation>
#include <QWidget>

//! The required delta of headers to the estimated number of available headers until we show the IBD progress
static constexpr int HEADER_HEIGHT_DELTA_SYNC = 24;

namespace Ui {
    class ModalOverlay;
}

/** Modal overlay to display information about the chain-sync state */
class ModalOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit ModalOverlay(bool enable_wallet, QWidget *parent);
    ~ModalOverlay();

    void tipUpdate(int count, const QDateTime& blockDate, double nVerificationProgress);
    void setKnownBestHeight(int count, const QDateTime& blockDate, bool presync);

    // will show or hide the modal layer
    void showHide(bool hide = false, bool userRequested = false);
    bool isLayerVisible() const { return layerIsVisible; }

public Q_SLOTS:
    void toggleVisibility();
    void closeClicked();

Q_SIGNALS:
    void triggered(bool hidden);

protected:
    bool eventFilter(QObject * obj, QEvent * ev) override;
    bool event(QEvent* ev) override;

private:
    Ui::ModalOverlay *ui;
    int bestHeaderHeight{0}; // best known height (based on the headers)
    QDateTime bestHeaderDate;
    QVector<QPair<qint64, double> > blockProcessTime;
    bool layerIsVisible{false};
    bool userClosed{false};
    QPropertyAnimation m_animation;
    void UpdateHeaderSyncLabel();
    void UpdateHeaderPresyncLabel(int height, const QDateTime& blockDate);
};

#endif // TORTOISECOIN_QT_MODALOVERLAY_H
