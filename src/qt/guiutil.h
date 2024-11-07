// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GUIUTIL_H
#define BITCOIN_QT_GUIUTIL_H

#include <consensus/amount.h>
#include <net.h>
#include <netaddress.h>
#include <util/check.h>
#include <util/fs.h>

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLabel>
#include <QMessageBox>
#include <QMetaObject>
#include <QObject>
#include <QProgressBar>
#include <QString>
#include <QTableView>

#include <cassert>
#include <chrono>
#include <utility>

class PlatformStyle;
class QValidatedLineEdit;
class SendCoinsRecipient;

namespace interfaces
{
    class Node;
}

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QAbstractItemView;
class QAction;
class QDateTime;
class QDialog;
class QFont;
class QKeySequence;
class QLineEdit;
class QMenu;
class QPoint;
class QProgressDialog;
class QUrl;
class QWidget;
QT_END_NAMESPACE

/** Utility functions used by the Bitcoin Qt UI.
 */
namespace GUIUtil
{
    // Use this flags to prevent a "What's This" button in the title bar of the dialog on Windows.
    constexpr auto dialog_flags = Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;

    // Create human-readable string from date
    QString dateTimeStr(const QDateTime &datetime);
    QString dateTimeStr(qint64 nTime);

    // Return a monospace font
    QFont fixedPitchFont(bool use_embedded_font = false);

    // Set up widget for address
    void setupAddressWidget(QValidatedLineEdit *widget, QWidget *parent);

    /**
     * Connects an additional shortcut to a QAbstractButton. Works around the
     * one shortcut limitation of the button's shortcut property.
     * @param[in] button    QAbstractButton to assign shortcut to
     * @param[in] shortcut  QKeySequence to use as shortcut
     */
    void AddButtonShortcut(QAbstractButton* button, const QKeySequence& shortcut);

    // Parse "bitcoin:" URI into recipient object, return true on successful parsing
    bool parseBitcoinURI(const QUrl &uri, SendCoinsRecipient *out);
    bool parseBitcoinURI(QString uri, SendCoinsRecipient *out);
    QString formatBitcoinURI(const SendCoinsRecipient &info);

    // Returns true if given address+amount meets "dust" definition
    bool isDust(interfaces::Node& node, const QString& address, const CAmount& amount);

    // HTML escaping for rich text controls
    QString HtmlEscape(const QString& str, bool fMultiLine=false);
    QString HtmlEscape(const std::string& str, bool fMultiLine=false);

    /** Copy a field of the currently selected entry of a view to the clipboard. Does nothing if nothing
        is selected.
       @param[in] column  Data column to extract from the model
       @param[in] role    Data role to extract from the model
       @see  TransactionView::copyLabel, TransactionView::copyAmount, TransactionView::copyAddress
     */
    void copyEntryData(const QAbstractItemView *view, int column, int role=Qt::EditRole);

    /** Return a field of the currently selected entry as a QString. Does nothing if nothing
        is selected.
       @param[in] column  Data column to extract from the model
       @see  TransactionView::copyLabel, TransactionView::copyAmount, TransactionView::copyAddress
     */
    QList<QModelIndex> getEntryData(const QAbstractItemView *view, int column);

    /** Returns true if the specified field of the currently selected view entry is not empty.
       @param[in] column  Data column to extract from the model
       @param[in] role    Data role to extract from the model
       @see  TransactionView::contextualMenu
     */
    bool hasEntryData(const QAbstractItemView *view, int column, int role);

    void setClipboard(const QString& str);

    /**
     * Loads the font from the file specified by file_name, aborts if it fails.
     */
    void LoadFont(const QString& file_name);

    /**
     * Determine default data directory for operating system.
     */
    QString getDefaultDataDirectory();

    /**
     * Extract first suffix from filter pattern "Description (*.foo)" or "Description (*.foo *.bar ...).
     *
     * @param[in] filter Filter specification such as "Comma Separated Files (*.csv)"
     * @return QString
     */
    QString ExtractFirstSuffixFromFilter(const QString& filter);

    /** Get save filename, mimics QFileDialog::getSaveFileName, except that it appends a default suffix
        when no suffix is provided by the user.

      @param[in] parent  Parent window (or 0)
      @param[in] caption Window caption (or empty, for default)
      @param[in] dir     Starting directory (or empty, to default to documents directory)
      @param[in] filter  Filter specification such as "Comma Separated Files (*.csv)"
      @param[out] selectedSuffixOut  Pointer to return the suffix (file type) that was selected (or 0).
                  Can be useful when choosing the save file format based on suffix.
     */
    QString getSaveFileName(QWidget *parent, const QString &caption, const QString &dir,
        const QString &filter,
        QString *selectedSuffixOut);

    /** Get open filename, convenience wrapper for QFileDialog::getOpenFileName.

      @param[in] parent  Parent window (or 0)
      @param[in] caption Window caption (or empty, for default)
      @param[in] dir     Starting directory (or empty, to default to documents directory)
      @param[in] filter  Filter specification such as "Comma Separated Files (*.csv)"
      @param[out] selectedSuffixOut  Pointer to return the suffix (file type) that was selected (or 0).
                  Can be useful when choosing the save file format based on suffix.
     */
    QString getOpenFileName(QWidget *parent, const QString &caption, const QString &dir,
        const QString &filter,
        QString *selectedSuffixOut);

    /** Get connection type to call object slot in GUI thread with invokeMethod. The call will be blocking.

       @returns If called from the GUI thread, return a Qt::DirectConnection.
                If called from another thread, return a Qt::BlockingQueuedConnection.
    */
    Qt::ConnectionType blockingGUIThreadConnection();

    // Determine whether a widget is hidden behind other windows
    bool isObscured(QWidget *w);

    // Activate, show and raise the widget
    void bringToFront(QWidget* w);

    // Set shortcut to close window
    void handleCloseWindowShortcut(QWidget* w);

    // Open debug.log
    void openDebugLogfile();

    // Open the config file
    bool openBitcoinConf();

    /** Qt event filter that intercepts ToolTipChange events, and replaces the tooltip with a rich text
      representation if needed. This assures that Qt can word-wrap long tooltip messages.
      Tooltips longer than the provided size threshold (in characters) are wrapped.
     */
    class ToolTipToRichTextFilter : public QObject
    {
        Q_OBJECT

    public:
        explicit ToolTipToRichTextFilter(int size_threshold, QObject *parent = nullptr);

    protected:
        bool eventFilter(QObject *obj, QEvent *evt) override;

    private:
        int size_threshold;
    };

    /**
     * Qt event filter that intercepts QEvent::FocusOut events for QLabel objects, and
     * resets their `textInteractionFlags' property to get rid of the visible cursor.
     *
     * This is a temporary fix of QTBUG-59514.
     */
    class LabelOutOfFocusEventFilter : public QObject
    {
        Q_OBJECT

    public:
        explicit LabelOutOfFocusEventFilter(QObject* parent);
        bool eventFilter(QObject* watched, QEvent* event) override;
    };

    bool GetStartOnSystemStartup();
    bool SetStartOnSystemStartup(bool fAutoStart);

    /** Convert QString to OS specific boost path through UTF-8 */
    fs::path QStringToPath(const QString &path);

    /** Convert OS specific boost path to QString through UTF-8 */
    QString PathToQString(const fs::path &path);

    /** Convert enum Network to QString */
    QString NetworkToQString(Network net);

    /** Convert enum ConnectionType to QString */
    QString ConnectionTypeToQString(ConnectionType conn_type, bool prepend_direction);

    /** Convert seconds into a QString with days, hours, mins, secs */
    QString formatDurationStr(std::chrono::seconds dur);

    /** Convert peer connection time to a QString denominated in the most relevant unit. */
    QString FormatPeerAge(std::chrono::seconds time_connected);

    /** Format CNodeStats.nServices bitmask into a user-readable string */
    QString formatServicesStr(quint64 mask);

    /** Format a CNodeStats.m_last_ping_time into a user-readable string or display N/A, if 0 */
    QString formatPingTime(std::chrono::microseconds ping_time);

    /** Format a CNodeStateStats.time_offset into a user-readable string */
    QString formatTimeOffset(int64_t time_offset);

    QString formatNiceTimeOffset(qint64 secs);

    QString formatBytes(uint64_t bytes);

    qreal calculateIdealFontSize(int width, const QString& text, QFont font, qreal minPointSize = 4, qreal startPointSize = 14);

    class ThemedLabel : public QLabel
    {
        Q_OBJECT

    public:
        explicit ThemedLabel(const PlatformStyle* platform_style, QWidget* parent = nullptr);
        void setThemedPixmap(const QString& image_filename, int width, int height);

    protected:
        void changeEvent(QEvent* e) override;

    private:
        const PlatformStyle* m_platform_style;
        QString m_image_filename;
        int m_pixmap_width;
        int m_pixmap_height;
        void updateThemedPixmap();
    };

    class ClickableLabel : public ThemedLabel
    {
        Q_OBJECT

    public:
        explicit ClickableLabel(const PlatformStyle* platform_style, QWidget* parent = nullptr);

    Q_SIGNALS:
        /** Emitted when the label is clicked. The relative mouse coordinates of the click are
         * passed to the signal.
         */
        void clicked(const QPoint& point);
    protected:
        void mouseReleaseEvent(QMouseEvent *event) override;
    };

    class ClickableProgressBar : public QProgressBar
    {
        Q_OBJECT

    Q_SIGNALS:
        /** Emitted when the progressbar is clicked. The relative mouse coordinates of the click are
         * passed to the signal.
         */
        void clicked(const QPoint& point);
    protected:
        void mouseReleaseEvent(QMouseEvent *event) override;
    };

    typedef ClickableProgressBar ProgressBar;

    class ItemDelegate : public QItemDelegate
    {
        Q_OBJECT
    public:
        ItemDelegate(QObject* parent) : QItemDelegate(parent) {}

    Q_SIGNALS:
        void keyEscapePressed();

    private:
        bool eventFilter(QObject *object, QEvent *event) override;
    };

    // Fix known bugs in QProgressDialog class.
    void PolishProgressDialog(QProgressDialog* dialog);

    /**
     * Returns the distance in pixels appropriate for drawing a subsequent character after text.
     *
     * In Qt 5.12 and before the QFontMetrics::width() is used and it is deprecated since Qt 5.13.
     * In Qt 5.11 the QFontMetrics::horizontalAdvance() was introduced.
     */
    int TextWidth(const QFontMetrics& fm, const QString& text);

    /**
     * Writes to debug.log short info about the used Qt and the host system.
     */
    void LogQtInfo();

    /**
     * Call QMenu::popup() only on supported QT_QPA_PLATFORM.
     */
    void PopupMenu(QMenu* menu, const QPoint& point, QAction* at_action = nullptr);

    /**
     * Returns the start-moment of the day in local time.
     *
     * QDateTime::QDateTime(const QDate& date) is deprecated since Qt 5.15.
     * QDate::startOfDay() was introduced in Qt 5.14.
     */
    QDateTime StartOfDay(const QDate& date);

    /**
     * Returns true if pixmap has been set.
     *
     * QPixmap* QLabel::pixmap() is deprecated since Qt 5.15.
     */
    bool HasPixmap(const QLabel* label);
    QImage GetImage(const QLabel* label);

    /**
     * Splits the string into substrings wherever separator occurs, and returns
     * the list of those strings. Empty strings do not appear in the result.
     *
     * QString::split() signature differs in different Qt versions:
     *  - QString::SplitBehavior is deprecated since Qt 5.15
     *  - Qt::SplitBehavior was introduced in Qt 5.14
     * If {QString|Qt}::SkipEmptyParts behavior is required, use this
     * function instead of QString::split().
     */
    template <typename SeparatorType>
    QStringList SplitSkipEmptyParts(const QString& string, const SeparatorType& separator)
    {
    #if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        return string.split(separator, Qt::SkipEmptyParts);
    #else
        return string.split(separator, QString::SkipEmptyParts);
    #endif
    }


    /**
     * Replaces a plain text link with an HTML tagged one.
     */
    QString MakeHtmlLink(const QString& source, const QString& link);

    void PrintSlotException(
        const std::exception* exception,
        const QObject* sender,
        const QObject* receiver);

    /**
     * A drop-in replacement of QObject::connect function
     * (see: https://doc.qt.io/qt-5/qobject.html#connect-3), that
     * guaranties that all exceptions are handled within the slot.
     *
     * NOTE: This function is incompatible with Qt private signals.
     */
    template <typename Sender, typename Signal, typename Receiver, typename Slot>
    auto ExceptionSafeConnect(
        Sender sender, Signal signal, Receiver receiver, Slot method,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        return QObject::connect(
            sender, signal, receiver,
            [sender, receiver, method](auto&&... args) {
                bool ok{true};
                try {
                    (receiver->*method)(std::forward<decltype(args)>(args)...);
                } catch (const NonFatalCheckError& e) {
                    PrintSlotException(&e, sender, receiver);
                    ok = QMetaObject::invokeMethod(
                        qApp, "handleNonFatalException",
                        blockingGUIThreadConnection(),
                        Q_ARG(QString, QString::fromStdString(e.what())));
                } catch (const std::exception& e) {
                    PrintSlotException(&e, sender, receiver);
                    ok = QMetaObject::invokeMethod(
                        qApp, "handleRunawayException",
                        blockingGUIThreadConnection(),
                        Q_ARG(QString, QString::fromStdString(e.what())));
                } catch (...) {
                    PrintSlotException(nullptr, sender, receiver);
                    ok = QMetaObject::invokeMethod(
                        qApp, "handleRunawayException",
                        blockingGUIThreadConnection(),
                        Q_ARG(QString, "Unknown failure occurred."));
                }
                assert(ok);
            },
            type);
    }

    /**
     * Shows a QDialog instance asynchronously, and deletes it on close.
     */
    void ShowModalDialogAsynchronously(QDialog* dialog);

    inline bool IsEscapeOrBack(int key)
    {
        if (key == Qt::Key_Escape) return true;
#ifdef Q_OS_ANDROID
        if (key == Qt::Key_Back) return true;
#endif // Q_OS_ANDROID
        return false;
    }

    QString WalletDisplayName(const std::string& name);
    QString WalletDisplayName(const QString& name);

} // namespace GUIUtil

#endif // BITCOIN_QT_GUIUTIL_H
