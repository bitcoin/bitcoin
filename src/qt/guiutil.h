// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GUIUTIL_H
#define BITCOIN_QT_GUIUTIL_H

#include <consensus/amount.h>
#include <fs.h>
#include <qt/guiconstants.h>
#include <net.h>
#include <netaddress.h>
#include <util/check.h>

#include <QApplication>
#include <QEvent>
#include <QItemDelegate>
#include <QLabel>
#include <QMetaObject>
#include <QObject>
#include <QProgressBar>
#include <QString>

#include <cassert>
#include <chrono>
#include <utility>

class QValidatedLineEdit;
class OptionsModel;
class SendCoinsRecipient;

namespace interfaces
{
class Node;
}

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QAbstractItemView;
class QAction;
class QButtonGroup;
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

/** Utility functions used by the Dash Qt UI.
 */
namespace GUIUtil
{
    /* Enumeration of possible "colors" */
    enum class ThemedColor {
        /* Transaction list -- TX status decoration - default color */
        DEFAULT,
        /* Transaction list -- unconfirmed transaction */
        UNCONFIRMED,
        /* Theme related blue color */
        BLUE,
        /* Eye-friendly orange color */
        ORANGE,
        /* Eye-friendly red color, e.g. Transaction list -- negative amount */
        RED,
        /* Eye-friendly green color */
        GREEN,
        /* Transaction list -- bare address (without label) */
        BAREADDRESS,
        /* Transaction list -- TX status decoration - open until date */
        TX_STATUS_OPENUNTILDATE,
        /* Background used for some widgets. Its slightly darker than the wallets frame background. */
        BACKGROUND_WIDGET,
        /* Border color used for some widgets. Its slightly brighter than BACKGROUND_WIDGET. */
        BORDER_WIDGET,
        /* Border color of network statistics overlay in debug window. */
        BORDER_NETSTATS,
        /* Background color of network statistics overlay in debug window. */
        BACKGROUND_NETSTATS,
        /* Pixel color of generated QR codes. */
        QR_PIXEL,
        /* Alternative color for black/white icons. White part will be filled with this color by default. */
        ICON_ALTERNATIVE_COLOR,
    };

    /* Enumeration of possible "styles" */
    enum class ThemedStyle {
        /* Invalid field background style */
        TS_INVALID,
        /* Warning text style */
        TS_WARNING,
        /* Failed operation text style */
        TS_ERROR,
        /* Successful operation text style */
        TS_SUCCESS,
        /* Command text style */
        TS_COMMAND,
        /* General text styles */
        TS_PRIMARY,
        TS_SECONDARY,
    };

    /** Helper to get colors for various themes which can't be applied via css for some reason */
    QColor getThemedQColor(ThemedColor color);

    /** Helper to get css style strings which are injected into rich text through qt */
    QString getThemedStyleQString(ThemedStyle style);

    /** Helper to get an icon colorized with the given color (replaces black) and colorAlternative (replaces white)  */
    QIcon getIcon(const QString& strIcon, ThemedColor color, ThemedColor colorAlternative, const QString& strIconPath = ICONS_PATH);
    QIcon getIcon(const QString& strIcon, ThemedColor color = ThemedColor::BLUE, const QString& strIconPath = ICONS_PATH);

    /** Helper to set an icon for a button with the given color (replaces black) and colorAlternative (replaces white). */
    void setIcon(QAbstractButton* button, const QString& strIcon, ThemedColor color, ThemedColor colorAlternative, const QSize& size);
    void setIcon(QAbstractButton* button, const QString& strIcon, ThemedColor color = ThemedColor::BLUE, const QSize& size = QSize(BUTTON_ICONSIZE, BUTTON_ICONSIZE));

    // Use this flags to prevent a "What's This" button in the title bar of the dialog on Windows.
    constexpr auto dialog_flags = Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;

    // Create human-readable string from date
    QString dateTimeStr(const QDateTime &datetime);
    QString dateTimeStr(qint64 nTime);

    // Return a monospace font
    QFont fixedPitchFont(bool use_embedded_font = false);

    // Set up widget for address
    void setupAddressWidget(QValidatedLineEdit *widget, QWidget *parent, bool fAllowURI = false);

    // Setup appearance settings if not done yet
    void setupAppearance(QWidget* parent, OptionsModel* model);

    /**
     * Connects an additional shortcut to a QAbstractButton. Works around the
     * one shortcut limitation of the button's shortcut property.
     * @param[in] button    QAbstractButton to assign shortcut to
     * @param[in] shortcut  QKeySequence to use as shortcut
     */
    void AddButtonShortcut(QAbstractButton* button, const QKeySequence& shortcut);

    // Parse "dash:" URI into recipient object, return true on successful parsing
    bool parseBitcoinURI(const QUrl &uri, SendCoinsRecipient *out);
    bool parseBitcoinURI(QString uri, SendCoinsRecipient *out);
    bool validateBitcoinURI(const QString& uri);
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

    // Open dash.conf
    void openConfigfile();

    // Browse backup folder
    void showBackups();

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

    /** Change the stylesheet directory. This is used by
        the parameter -custom-css-dir.*/
    void setStyleSheetDirectory(const QString& path);

    /** Check if a custom css directory has been set with -custom-css-dir */
    bool isStyleSheetDirectoryCustom();

    /** Return a list of all required css files */
    std::vector<QString> listStyleSheets();

    /** Return a list of all theme css files */
    std::vector<QString> listThemes();

    /** Return the name of the default theme `*/
    QString getDefaultTheme();

    /** Check if the given theme name is valid or not */
    bool isValidTheme(const QString& strTheme);

    /** Sets the stylesheet of the whole app and updates it if the
    related css files has been changed and -debug-ui mode is active. */
    void loadStyleSheet(bool fForceUpdate = false);

    enum class FontFamily {
        SystemDefault,
        Montserrat,
    };

    FontFamily fontFamilyFromString(const QString& strFamily);
    QString fontFamilyToString(FontFamily family);

    /** set/get font family: GUIUtil::fontFamily */
    FontFamily getFontFamilyDefault();
    FontFamily getFontFamily();
    void setFontFamily(FontFamily family);

    enum class FontWeight {
        Normal, // Font weight for normal text
        Bold,   // Font weight for bold text
    };

    /** Convert weight value from args (0-8) to QFont::Weight */
    bool weightFromArg(int nArg, QFont::Weight& weight);
    /** Convert QFont::Weight to an arg value (0-8) */
    int weightToArg(const QFont::Weight weight);
    /** Convert GUIUtil::FontWeight to QFont::Weight */
    QFont::Weight toQFontWeight(FontWeight weight);

    /** set/get normal font weight: GUIUtil::fontWeightNormal */
    QFont::Weight getFontWeightNormalDefault();
    QFont::Weight getFontWeightNormal();
    void setFontWeightNormal(QFont::Weight weight);

    /** set/get bold font weight: GUIUtil::fontWeightBold */
    QFont::Weight getFontWeightBoldDefault();
    QFont::Weight getFontWeightBold();
    void setFontWeightBold(QFont::Weight weight);

    /** set/get font scale: GUIUtil::fontScale */
    int getFontScaleDefault();
    int getFontScale();
    void setFontScale(int nScale);

    /** get font size with GUIUtil::fontScale applied */
    double getScaledFontSize(int nSize);

    /** Load dash specific appliciation fonts */
    bool loadFonts();
    /** Check if the fonts have been loaded successfully */
    bool fontsLoaded();

    /** Set an application wide default font, depends on the selected theme */
    void setApplicationFont();

    /** Workaround to set correct font styles in all themes since there is a bug in macOS which leads to
        issues loading variations of montserrat in css it also keeps track of the set fonts to update on
        theme changes. */
    void setFont(const std::vector<QWidget*>& vecWidgets, FontWeight weight, int nPointSize = -1, bool fItalic = false);

    /** Update the font of all widgets where a custom font has been set with
        GUIUtil::setFont */
    void updateFonts();

    /** Get a properly weighted QFont object with the selected font. */
    QFont getFont(FontFamily family, QFont::Weight qWeight, bool fItalic = false, int nPointSize = -1);
    QFont getFont(QFont::Weight qWeight, bool fItalic = false, int nPointSize = -1);
    QFont getFont(FontWeight weight, bool fItalic = false, int nPointSize = -1);

    /** Get the default normal QFont */
    QFont getFontNormal();

    /** Get the default bold QFont */
    QFont getFontBold();

    /** Return supported normal default for the current font family */
    QFont::Weight getSupportedFontWeightNormalDefault();
    /** Return supported bold default for the current font family */
    QFont::Weight getSupportedFontWeightBoldDefault();
    /** Return supported weights for the current font family */
    std::vector<QFont::Weight> getSupportedWeights();
    /** Convert an index to a weight in the supported weights vector */
    QFont::Weight supportedWeightFromIndex(int nIndex);
    /** Convert a weight to an index in the supported weights vector */
    int supportedWeightToIndex(QFont::Weight weight);
    /** Check if a weight is supported by the current font family */
    bool isSupportedWeight(QFont::Weight weight);

    /** Return the name of the currently active theme.*/
    QString getActiveTheme();

    /** Check if a dash specific theme is activated (light/dark).*/
    bool dashThemeActive();

    /** Load the theme and update all UI elements according to the appearance settings. */
    void loadTheme(bool fForce = false);

    /** Disable the OS default focus rect for macOS because we have custom focus rects
     * set in the css files */
    void disableMacFocusRect(const QWidget* w);

    /** Enable/Disable the macOS focus rects depending on the current theme. */
    void updateMacFocusRects();

    /** Update shortcuts for individual buttons in QButtonGroup based on their visibility. */
    void updateButtonGroupShortcuts(QButtonGroup* buttonGroup);

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

    /** Format a CNodeCombinedStats.nTimeOffset into a user-readable string */
    QString formatTimeOffset(int64_t nTimeOffset);

    QString formatNiceTimeOffset(qint64 secs);

    QString formatBytes(uint64_t bytes);

    qreal calculateIdealFontSize(int width, const QString& text, QFont font, qreal minPointSize = 4, qreal startPointSize = 14);

    class ClickableLabel : public QLabel
    {
        Q_OBJECT

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

} // namespace GUIUtil

#endif // BITCOIN_QT_GUIUTIL_H
