// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/splashscreen.h>


#include <chainparams.h>
#include <clientversion.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <qt/guiutil.h>
#include <qt/networkstyle.h>
#include <qt/walletmodel.h>
#include <util/system.h>
#include <util/translation.h>

#include <functional>

#include <QApplication>
#include <QCloseEvent>
#include <QPainter>
#include <QScreen>

SplashScreen::SplashScreen(const NetworkStyle *networkStyle) :
    QWidget(), curAlignment(0)
{

    // transparent background
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background:transparent;");

    // no window decorations
    setWindowFlags(Qt::FramelessWindowHint);

    // Geometries of splashscreen
    int width = 380;
    int height = 460;
    int logoWidth = 270;
    int logoHeight = 270;

    // set reference point, paddings
    int paddingTop = 10;
    int titleVersionVSpace = 25;

    float fontFactor            = 1.0;
    float scale = qApp->devicePixelRatio();

    // define text to place
    QString titleText       = PACKAGE_NAME;
    QString versionText = QString::fromStdString(FormatFullVersion()).remove(0, 1);
    QString titleAddText    = networkStyle->getTitleAddText();

    QFont fontNormal = GUIUtil::getFontNormal();
    QFont fontBold = GUIUtil::getFontBold();

    QPixmap pixmapLogo = networkStyle->getSplashImage();
    pixmapLogo.setDevicePixelRatio(scale);

    // Adjust logo color based on the current theme
    QImage imgLogo = pixmapLogo.toImage().convertToFormat(QImage::Format_ARGB32);
    QColor logoColor = GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BLUE);
    for (int x = 0; x < imgLogo.width(); ++x) {
        for (int y = 0; y < imgLogo.height(); ++y) {
            const QRgb rgb = imgLogo.pixel(x, y);
            imgLogo.setPixel(x, y, qRgba(logoColor.red(), logoColor.green(), logoColor.blue(), qAlpha(rgb)));
        }
    }
    pixmapLogo.convertFromImage(imgLogo);

    pixmap = QPixmap(width * scale, height * scale);
    pixmap.setDevicePixelRatio(scale);
    pixmap.fill(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BORDER_WIDGET));

    QPainter pixPaint(&pixmap);

    QRect rect = QRect(1, 1, width - 2, height - 2);
    pixPaint.fillRect(rect, GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BACKGROUND_WIDGET));

    pixPaint.drawPixmap((width / 2) - (logoWidth / 2), (height / 2) - (logoHeight / 2) + 20, pixmapLogo.scaled(logoWidth * scale, logoHeight * scale, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    pixPaint.setPen(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::DEFAULT));

    // check font size and drawing with
    fontBold.setPointSize(50 * fontFactor);
    pixPaint.setFont(fontBold);
    QFontMetrics fm = pixPaint.fontMetrics();
    int titleTextWidth = GUIUtil::TextWidth(fm, titleText);
    if (titleTextWidth > width * 0.8) {
        fontFactor = 0.75;
    }

    fontBold.setPointSize(50 * fontFactor);
    pixPaint.setFont(fontBold);
    fm = pixPaint.fontMetrics();
    titleTextWidth  = GUIUtil::TextWidth(fm, titleText);
    int titleTextHeight = fm.height();
    pixPaint.drawText((width / 2) - (titleTextWidth / 2), titleTextHeight + paddingTop, titleText);

    fontNormal.setPointSize(16 * fontFactor);
    pixPaint.setFont(fontNormal);
    fm = pixPaint.fontMetrics();
    int versionTextWidth = GUIUtil::TextWidth(fm, versionText);
    pixPaint.drawText((width / 2) - (versionTextWidth / 2), titleTextHeight + paddingTop + titleVersionVSpace, versionText);

    // draw additional text if special network
    if(!titleAddText.isEmpty()) {
        fontBold.setPointSize(10 * fontFactor);
        pixPaint.setFont(fontBold);
        fm = pixPaint.fontMetrics();
        int titleAddTextWidth = GUIUtil::TextWidth(fm, titleAddText);
        // Draw the badge background with the network-specific color
        QRect badgeRect = QRect(width - titleAddTextWidth - 20, 5, width, fm.height() + 10);
        QColor badgeColor = networkStyle->getBadgeColor();
        pixPaint.fillRect(badgeRect, badgeColor);
        // Draw the text itself using white color, regardless of the current theme
        pixPaint.setPen(QColor(255, 255, 255));
        pixPaint.drawText(width - titleAddTextWidth - 10, paddingTop + 10, titleAddText);
    }

    pixPaint.end();

    // Resize window and move to center of desktop, disallow resizing
    QRect r(QPoint(), QSize(width, height));
    resize(r.size());
    setFixedSize(r.size());
    move(QGuiApplication::primaryScreen()->geometry().center() - r.center());

    installEventFilter(this);

    GUIUtil::handleCloseWindowShortcut(this);
}

SplashScreen::~SplashScreen()
{
    if (m_node) unsubscribeFromCoreSignals();
}

void SplashScreen::setNode(interfaces::Node& node)
{
    assert(!m_node);
    m_node = &node;
    subscribeToCoreSignals();
    if (m_shutdown) m_node->startShutdown();
}

void SplashScreen::shutdown()
{
    m_shutdown = true;
    if (m_node) m_node->startShutdown();
}

bool SplashScreen::eventFilter(QObject * obj, QEvent * ev) {
    if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
        if (keyEvent->key() == Qt::Key_Q) {
            shutdown();
        }
    }
    return QObject::eventFilter(obj, ev);
}

static void InitMessage(SplashScreen *splash, const std::string &message)
{
    bool invoked = QMetaObject::invokeMethod(splash, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(int, Qt::AlignBottom | Qt::AlignHCenter),
        Q_ARG(QColor, GUIUtil::getThemedQColor(GUIUtil::ThemedColor::DEFAULT)));
    assert(invoked);
}

static void ShowProgress(SplashScreen *splash, const std::string &title, int nProgress, bool resume_possible)
{
    InitMessage(splash, title + std::string("\n") +
            (resume_possible ? SplashScreen::tr("(press q to shutdown and continue later)").toStdString()
                                : SplashScreen::tr("press q to shutdown").toStdString()) +
            strprintf("\n%d", nProgress) + "%");
}

void SplashScreen::subscribeToCoreSignals()
{
    // Connect signals to client
    m_handler_init_message = m_node->handleInitMessage(std::bind(InitMessage, this, std::placeholders::_1));
    m_handler_show_progress = m_node->handleShowProgress(std::bind(ShowProgress, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    m_handler_init_wallet = m_node->handleInitWallet([this]() { handleLoadWallet(); });
}

void SplashScreen::handleLoadWallet()
{
#ifdef ENABLE_WALLET
    if (!WalletModel::isWalletEnabled()) return;
    m_handler_load_wallet = m_node->walletLoader().handleLoadWallet([this](std::unique_ptr<interfaces::Wallet> wallet) {
        m_connected_wallet_handlers.emplace_back(wallet->handleShowProgress(std::bind(ShowProgress, this, std::placeholders::_1, std::placeholders::_2, false)));
        m_connected_wallets.emplace_back(std::move(wallet));
    });
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    m_handler_init_message->disconnect();
    m_handler_show_progress->disconnect();
#ifdef ENABLE_WALLET
    if (m_handler_load_wallet != nullptr) {
        m_handler_load_wallet->disconnect();
    }
#endif // ENABLE_WALLET
    for (const auto& handler : m_connected_wallet_handlers) {
        handler->disconnect();
    }
    m_connected_wallet_handlers.clear();
    m_connected_wallets.clear();
}

void SplashScreen::showMessage(const QString &message, int alignment, const QColor &color)
{
    curMessage = message;
    curAlignment = alignment;
    curColor = color;
    update();
}

void SplashScreen::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QFont messageFont = GUIUtil::getFontNormal();
    messageFont.setPointSize(14);
    painter.setFont(messageFont);
    painter.drawPixmap(0, 0, pixmap);
    QRect r = rect().adjusted(5, 5, -5, -15);
    painter.setPen(curColor);
    painter.drawText(r, curAlignment, curMessage);
}

void SplashScreen::closeEvent(QCloseEvent *event)
{
    shutdown(); // allows an "emergency" shutdown during startup
    event->ignore();
}
