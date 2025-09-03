// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <qt/modaloverlay.h>
#include <qt/forms/ui_modaloverlay.h>

#include <chainparams.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>

#include <QColor>
#include <QEasingCurve>
#include <QPalette>
#include <QPropertyAnimation>
#include <QResizeEvent>

ModalOverlay::ModalOverlay(bool enable_wallet, const PlatformStyle& platform_style, QWidget* parent)
    : QWidget(parent),
      ui(new Ui::ModalOverlay),
      bestHeaderDate(QDateTime()),
      m_platform_style(platform_style)
{
    ui->setupUi(this);

    // Add ThemedLabel for warning icon
    GUIUtil::ThemedLabel* warningIcon = new GUIUtil::ThemedLabel(&m_platform_style, this);
    warningIcon->setThemedPixmap(QStringLiteral(":/icons/warning"), 48, 48);
    ui->verticalLayoutIcon->insertWidget(0, warningIcon);

    connect(ui->closeButton, &QPushButton::clicked, this, &ModalOverlay::closeClicked);
    if (parent) {
        parent->installEventFilter(this);
        raise();
    }
    ui->closeButton->installEventFilter(this);

    blockProcessTime.clear();
    setVisible(false);
    if (!enable_wallet) {
        ui->infoText->setVisible(false);
        ui->infoTextStrong->setText(tr("%1 is currently syncing.  It will download headers and blocks from peers and validate them until reaching the tip of the block chain.").arg(CLIENT_NAME));
    }

    m_animation.setTargetObject(this);
    m_animation.setPropertyName("pos");
    m_animation.setDuration(300 /* ms */);
    m_animation.setEasingCurve(QEasingCurve::OutQuad);

    updateThemeStyles();
}

ModalOverlay::~ModalOverlay()
{
    delete ui;
}

bool ModalOverlay::eventFilter(QObject * obj, QEvent * ev) {
    if (obj == parent()) {
        if (ev->type() == QEvent::Resize) {
            QResizeEvent * rev = static_cast<QResizeEvent*>(ev);
            resize(rev->size());
            if (!layerIsVisible)
                setGeometry(0, height(), width(), height());

            if (m_animation.endValue().toPoint().y() > 0) {
                m_animation.setEndValue(QPoint(0, height()));
            }
        }
        else if (ev->type() == QEvent::ChildAdded) {
            raise();
        }
    }

    if (obj == ui->closeButton && ev->type() == QEvent::FocusOut && layerIsVisible) {
        ui->closeButton->setFocus(Qt::OtherFocusReason);
    }

    return QWidget::eventFilter(obj, ev);
}

//! Tracks parent widget changes
bool ModalOverlay::event(QEvent* ev) {
    if (ev->type() == QEvent::ParentAboutToChange) {
        if (parent()) parent()->removeEventFilter(this);
    }
    else if (ev->type() == QEvent::ParentChange) {
        if (parent()) {
            parent()->installEventFilter(this);
            raise();
        }
    }
    return QWidget::event(ev);
}

void ModalOverlay::setKnownBestHeight(int count, const QDateTime& blockDate, bool presync)
{
    if (!presync && count > bestHeaderHeight) {
        bestHeaderHeight = count;
        bestHeaderDate = blockDate;
        UpdateHeaderSyncLabel();
    }
    if (presync) {
        UpdateHeaderPresyncLabel(count, blockDate);
    }
}

void ModalOverlay::tipUpdate(int count, const QDateTime& blockDate, double nVerificationProgress)
{
    QDateTime currentDate = QDateTime::currentDateTime();

    // keep a vector of samples of verification progress at height
    blockProcessTime.push_front(qMakePair(currentDate.toMSecsSinceEpoch(), nVerificationProgress));

    // show progress speed if we have more than one sample
    if (blockProcessTime.size() >= 2) {
        double progressDelta = 0;
        double progressPerHour = 0;
        qint64 timeDelta = 0;
        qint64 remainingMSecs = 0;
        double remainingProgress = 1.0 - nVerificationProgress;
        for (int i = 1; i < blockProcessTime.size(); i++) {
            QPair<qint64, double> sample = blockProcessTime[i];

            // take first sample after 500 seconds or last available one
            if (sample.first < (currentDate.toMSecsSinceEpoch() - 500 * 1000) || i == blockProcessTime.size() - 1) {
                progressDelta = blockProcessTime[0].second - sample.second;
                timeDelta = blockProcessTime[0].first - sample.first;
                progressPerHour = (progressDelta > 0) ? progressDelta / (double)timeDelta * 1000 * 3600 : 0;
                remainingMSecs = (progressDelta > 0) ? remainingProgress / progressDelta * timeDelta : -1;
                break;
            }
        }
        // show progress increase per hour
        ui->progressIncreasePerH->setText(QString::number(progressPerHour * 100, 'f', 2)+"%");

        // show expected remaining time
        if(remainingMSecs >= 0) {
            ui->expectedTimeLeft->setText(GUIUtil::formatNiceTimeOffset(remainingMSecs / 1000.0));
        } else {
            ui->expectedTimeLeft->setText(QObject::tr("unknown"));
        }

        static const int MAX_SAMPLES = 5000;
        if (blockProcessTime.count() > MAX_SAMPLES) {
            blockProcessTime.remove(MAX_SAMPLES, blockProcessTime.count() - MAX_SAMPLES);
        }
    }

    // show the last block date
    ui->newestBlockDate->setText(blockDate.toString());

    // show the percentage done according to nVerificationProgress
    ui->percentageProgress->setText(QString::number(nVerificationProgress*100, 'f', 2)+"%");

    if (!bestHeaderDate.isValid())
        // not syncing
        return;

    // estimate the number of headers left based on nPowTargetSpacing
    // and check if the gui is not aware of the best header (happens rarely)
    int estimateNumHeadersLeft = bestHeaderDate.secsTo(currentDate) / Params().GetConsensus().nPowTargetSpacing;
    bool hasBestHeader = bestHeaderHeight >= count;

    // show remaining number of blocks
    if (estimateNumHeadersLeft < HEADER_HEIGHT_DELTA_SYNC && hasBestHeader) {
        ui->numberOfBlocksLeft->setText(QString::number(bestHeaderHeight - count));
    } else {
        UpdateHeaderSyncLabel();
        ui->expectedTimeLeft->setText(tr("Unknown…"));
    }
}

void ModalOverlay::UpdateHeaderSyncLabel() {
    int est_headers_left = bestHeaderDate.secsTo(QDateTime::currentDateTime()) / Params().GetConsensus().nPowTargetSpacing;
    ui->numberOfBlocksLeft->setText(tr("Unknown. Syncing Headers (%1, %2%)…").arg(bestHeaderHeight).arg(QString::number(100.0 / (bestHeaderHeight + est_headers_left) * bestHeaderHeight, 'f', 1)));
}

void ModalOverlay::UpdateHeaderPresyncLabel(int height, const QDateTime& blockDate) {
    int est_headers_left = blockDate.secsTo(QDateTime::currentDateTime()) / Params().GetConsensus().nPowTargetSpacing;
    ui->numberOfBlocksLeft->setText(tr("Unknown. Pre-syncing Headers (%1, %2%)…").arg(height).arg(QString::number(100.0 / (height + est_headers_left) * height, 'f', 1)));
}

void ModalOverlay::toggleVisibility()
{
    showHide(layerIsVisible, true);
    if (!layerIsVisible)
        userClosed = true;
}

void ModalOverlay::showHide(bool hide, bool userRequested)
{
    if ( (layerIsVisible && !hide) || (!layerIsVisible && hide) || (!hide && userClosed && !userRequested))
        return;

    Q_EMIT triggered(hide);

    if (!isVisible() && !hide)
        setVisible(true);

    m_animation.setStartValue(QPoint(0, hide ? 0 : height()));
    // The eventFilter() updates the endValue if it is required for QEvent::Resize.
    m_animation.setEndValue(QPoint(0, hide ? height() : 0));
    m_animation.start(QAbstractAnimation::KeepWhenStopped);
    layerIsVisible = !hide;

    if (layerIsVisible) {
        ui->closeButton->setFocus(Qt::OtherFocusReason);
    }
}

void ModalOverlay::closeClicked()
{
    showHide(true);
    userClosed = true;
}

void ModalOverlay::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::PaletteChange) {
        updateThemeStyles();
    }
    QWidget::changeEvent(e);
}

void ModalOverlay::updateThemeStyles()
{
    const QColor bg_colour = palette().color(backgroundRole());
    const bool dark_mode = GUIUtil::isDarkMode(bg_colour);

    // Overlay background - both light and dark mode use black but with different alpha
    QColor overlayBg = dark_mode ? QColor(0, 0, 0, 240) : QColor(0, 0, 0, 220);
    const QString bgStyle = QStringLiteral("#bgWidget { background: ") + overlayBg.name(QColor::HexArgb) + QStringLiteral("; }");
    ui->bgWidget->setStyleSheet(bgStyle);

    // Content widget with system window background, slightly transparent
    QColor contentBg = QColor(bg_colour.red(), bg_colour.green(), bg_colour.blue(), 240);
    const QString contentStyle = QStringLiteral("#contentWidget { background: ") + contentBg.name(QColor::HexArgb) + QStringLiteral("; border-radius: 6px; }");
    ui->contentWidget->setStyleSheet(contentStyle);
}
