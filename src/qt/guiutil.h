// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GUIUTIL_H
#define BITCOIN_QT_GUIUTIL_H

#include <amount.h>
#include <fs.h>

#include <QEvent>
#include <QHeaderView>
#include <QMessageBox>
#include <QObject>
#include <QProgressBar>
#include <QString>
#include <QTableView>
#include <QLabel>

class QValidatedLineEdit;
class SendCoinsRecipient;

QT_BEGIN_NAMESPACE
class QAbstractItemView;
class QDateTime;
class QFont;
class QLineEdit;
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
        /* Transaction list -- negative amount */
        NEGATIVE,
        /* Transaction list -- bare address (without label) */
        BAREADDRESS,
        /* Transaction list -- TX status decoration - open until date */
        TX_STATUS_OPENUNTILDATE,
        /* Transaction list -- TX status decoration - offline */
        TX_STATUS_OFFLINE,
        /* Transaction list -- TX status decoration - danger, tx needs attention */
        TX_STATUS_DANGER,
        /* Transaction list -- TX status decoration - LockedByInstantSend color */
        TX_STATUS_LOCKED,
    };

    /* Enumeration of possible "styles" */
    enum class ThemedStyle {
        /* Invalid field background style */
        TS_INVALID,
        /* Failed operation text style */
        TS_ERROR,
        /* Failed operation text style */
        TS_SUCCESS,
        /* Comand text style */
        TS_COMMAND,
        /* General text styles */
        TS_PRIMARY,
        TS_SECONDARY,
    };

    /** Helper to get colors for various themes which can't be applied via css for some reason */
    QColor getThemedQColor(ThemedColor color);

    /** Helper to get css style strings which are injected into rich text through qt */
    QString getThemedStyleQString(ThemedStyle style);

    // Create human-readable string from date
    QString dateTimeStr(const QDateTime &datetime);
    QString dateTimeStr(qint64 nTime);

    // Return a monospace font
    QFont fixedPitchFont();

    // Set up widgets for address and amounts
    void setupAddressWidget(QValidatedLineEdit *widget, QWidget *parent, bool fAllowURI = false);
    void setupAmountWidget(QLineEdit *widget, QWidget *parent);

    // Parse "dash:" URI into recipient object, return true on successful parsing
    bool parseBitcoinURI(const QUrl &uri, SendCoinsRecipient *out);
    bool parseBitcoinURI(QString uri, SendCoinsRecipient *out);
    bool validateBitcoinURI(const QString& uri);
    QString formatBitcoinURI(const SendCoinsRecipient &info);

    // Returns true if given address+amount meets "dust" definition
    bool isDust(const QString& address, const CAmount& amount);

    // HTML escaping for rich text controls
    QString HtmlEscape(const QString& str, bool fMultiLine=false);
    QString HtmlEscape(const std::string& str, bool fMultiLine=false);

    /** Copy a field of the currently selected entry of a view to the clipboard. Does nothing if nothing
        is selected.
       @param[in] column  Data column to extract from the model
       @param[in] role    Data role to extract from the model
       @see  TransactionView::copyLabel, TransactionView::copyAmount, TransactionView::copyAddress
     */
    void copyEntryData(QAbstractItemView *view, int column, int role=Qt::EditRole);

    /** Return a field of the currently selected entry as a QString. Does nothing if nothing
        is selected.
       @param[in] column  Data column to extract from the model
       @see  TransactionView::copyLabel, TransactionView::copyAmount, TransactionView::copyAddress
     */
    QList<QModelIndex> getEntryData(QAbstractItemView *view, int column);

    void setClipboard(const QString& str);

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

    // Open debug.log
    void openDebugLogfile();
	
    // Open dash.conf
    void openConfigfile();	

    // Browse backup folder
    void showBackups();

    // Replace invalid default fonts with known good ones
    void SubstituteFonts(const QString& language);

    /** Qt event filter that intercepts ToolTipChange events, and replaces the tooltip with a rich text
      representation if needed. This assures that Qt can word-wrap long tooltip messages.
      Tooltips longer than the provided size threshold (in characters) are wrapped.
     */
    class ToolTipToRichTextFilter : public QObject
    {
        Q_OBJECT

    public:
        explicit ToolTipToRichTextFilter(int size_threshold, QObject *parent = 0);

    protected:
        bool eventFilter(QObject *obj, QEvent *evt);

    private:
        int size_threshold;
    };

    /**
     * Makes a QTableView last column feel as if it was being resized from its left border.
     * Also makes sure the column widths are never larger than the table's viewport.
     * In Qt, all columns are resizable from the right, but it's not intuitive resizing the last column from the right.
     * Usually our second to last columns behave as if stretched, and when on stretch mode, columns aren't resizable
     * interactively or programmatically.
     *
     * This helper object takes care of this issue.
     *
     */
    class TableViewLastColumnResizingFixer: public QObject
    {
        Q_OBJECT

        public:
            TableViewLastColumnResizingFixer(QTableView* table, int lastColMinimumWidth, int allColsMinimumWidth, QObject *parent);
            void stretchColumnWidth(int column);

        private:
            QTableView* tableView;
            int lastColumnMinimumWidth;
            int allColumnsMinimumWidth;
            int lastColumnIndex;
            int columnCount;
            int secondToLastColumnIndex;

            void adjustTableColumnsWidth();
            int getAvailableWidthForColumn(int column);
            int getColumnsWidth();
            void connectViewHeadersSignals();
            void disconnectViewHeadersSignals();
            void setViewHeaderResizeMode(int logicalIndex, QHeaderView::ResizeMode resizeMode);
            void resizeColumn(int nColumnIndex, int width);

        private Q_SLOTS:
            void on_sectionResized(int logicalIndex, int oldSize, int newSize);
            void on_geometriesChanged();
    };

    bool GetStartOnSystemStartup();
    bool SetStartOnSystemStartup(bool fAutoStart);

    /** Modify Qt network specific settings on migration */
    void migrateQtSettings();

    /** Change the stylesheet directory. This is used by
        the parameter -custom-css-dir.*/
    void setStyleSheetDirectory(const QString& path);

    /** Check if a custom css directory has been set with -custom-css-dir */
    bool isStyleSheetDirectoryCustom();

    /** Return a list of all required css files */
    const std::vector<QString> listStyleSheets();

    /** Return a list of all theme css files */
    const std::vector<QString> listThemes();

    /** Load global CSS theme */
    QString loadStyleSheet();

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

    /** Set an application wide default font, depends on the selected theme */
    void setApplicationFont();

    /** Workaround to set correct font styles in all themes since there is a bug in macOS which leads to
        issues loading variations of montserrat in css it also keeps track of the set fonts to update on
        theme changes. */
    void setFont(const std::vector<QWidget*>& vecWidgets, FontWeight weight, int nPointSize = -1, bool fItalic = false);

    /** Workaround to set a fixed pitch font in traditional theme while keeping track of font updates */
    void setFixedPitchFont(const std::vector<QWidget*>& vecWidgets);

    /** Update the font of all widgets where a custom font has been set with
        GUIUtil::setFont */
    void updateFonts();

    /** Get a properly weighted QFont object with the font Montserrat
        Use ExtraLight as default as this lines up with the default in css. */
    QFont getFont(FontWeight weight, bool fItalic = false, int nPointSize = -1);

    /** Get the default normal QFont */
    QFont getFontNormal();

    /** Get the default bold QFont */
    QFont getFontBold();

    /** Check if a dash specific theme is activated (light/dark) */
    bool dashThemeActive();

    /** Disable the OS default focus rect for macOS because we have custom focus rects
     * set in the css files */
    void disableMacFocusRect(const QWidget* w);

    /* Convert QString to OS specific boost path through UTF-8 */
    fs::path qstringToBoostPath(const QString &path);

    /* Convert OS specific boost path to QString through UTF-8 */
    QString boostPathToQString(const fs::path &path);

    /* Convert seconds into a QString with days, hours, mins, secs */
    QString formatDurationStr(int secs);

    /* Format CNodeStats.nServices bitmask into a user-readable string */
    QString formatServicesStr(quint64 mask);

    /* Format a CNodeCombinedStats.dPingTime into a user-readable string or display N/A, if 0*/
    QString formatPingTime(double dPingTime);

    /* Format a CNodeCombinedStats.nTimeOffset into a user-readable string. */
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
        void mouseReleaseEvent(QMouseEvent *event);
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
        void mouseReleaseEvent(QMouseEvent *event);
    };

#if defined(Q_OS_MAC)
    // workaround for Qt OSX Bug:
    // https://bugreports.qt-project.org/browse/QTBUG-15631
    // QProgressBar uses around 10% CPU even when app is in background
    class ProgressBar : public ClickableProgressBar
    {
        bool event(QEvent *e) {
            return (e->type() != QEvent::StyleAnimationUpdate) ? QProgressBar::event(e) : false;
        }
    };
#else
    typedef ClickableProgressBar ProgressBar;
#endif

} // namespace GUIUtil

#endif // BITCOIN_QT_GUIUTIL_H
