// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/guiutil.h>

#include <qt/bitcoinaddressvalidator.h>
#include <qt/bitcoinunits.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/walletmodel.h>

#include <primitives/transaction.h>
#include <init.h>
#include <policy/policy.h>
#include <protocol.h>
#include <script/script.h>
#include <script/standard.h>
#include <ui_interface.h>
#include <util.h>

#ifdef WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501
#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#endif

#include <boost/scoped_array.hpp>

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QLineEdit>
#include <QSettings>
#include <QTextDocument> // for Qt::mightBeRichText
#include <QThread>
#include <QUrlQuery>
#include <QMouseEvent>

static fs::detail::utf8_codecvt_facet utf8;

#if defined(Q_OS_MAC)
extern double NSAppKitVersionNumber;
#if !defined(NSAppKitVersionNumber10_8)
#define NSAppKitVersionNumber10_8 1187
#endif
#if !defined(NSAppKitVersionNumber10_9)
#define NSAppKitVersionNumber10_9 1265
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <CoreServices/CoreServices.h>
#include <QProcess>

void ForceActivation();
#endif

namespace GUIUtil {

// The name of the traditional theme
static const QString traditionalTheme = "Traditional";
// The theme to set by default if settings are missing or incorrect
static const QString defaultTheme = "Light";
// The prefix a theme name should have if we want to apply dark colors and styles to it
static const QString darkThemePrefix = "Dark";

/** Font related default values. */
static const FontFamily defaultFontFamily = FontFamily::SystemDefault;
static const int defaultFontSize = 12;
static const double fontScaleSteps = 0.01;
#ifdef Q_OS_MAC
static const QFont::Weight defaultFontWeightNormal = QFont::ExtraLight;
static const QFont::Weight defaultFontWeightBold = QFont::Medium;
static const int defaultFontScale = 0;
#else
static const QFont::Weight defaultFontWeightNormal = QFont::Light;
static const QFont::Weight defaultFontWeightBold = QFont::Medium;
static const int defaultFontScale = 0;
#endif

/** Font related variables. */
// Application font family. May be overwritten by -font-family.
static FontFamily fontFamily = defaultFontFamily;
// Application font scale value. May be overwritten by -font-scale.
static int fontScale = defaultFontScale;
// Application font weight for normal text. May be overwritten by -font-weight-normal.
static QFont::Weight fontWeightNormal = defaultFontWeightNormal;
// Application font weight for bold text. May be overwritten by -font-weight-bold.
static QFont::Weight fontWeightBold = defaultFontWeightBold;

// Contains all widgets and its font attributes (weight, italic) with font changes due to GUIUtil::setFont
static std::map<QWidget*, std::pair<FontWeight, bool>> mapNormalFontUpdates;
// Contains all widgets where a fixed pitch font has been set with GUIUtil::setFixedPitchFont
static std::set<QWidget*> setFixedPitchFontUpdates;
// Contains all widgets where a non-default fontsize has been seet with GUIUtil::setFont
static std::map<QWidget*, int> mapFontSizeUpdates;

static const std::map<ThemedColor, QColor> themedColors = {
    { ThemedColor::DEFAULT, QColor(0, 0, 0) },
    { ThemedColor::UNCONFIRMED, QColor(128, 128, 128) },
    { ThemedColor::NEGATIVE, QColor(255, 0, 0) },
    { ThemedColor::BAREADDRESS, QColor(140, 140, 140) },
    { ThemedColor::TX_STATUS_OPENUNTILDATE, QColor(64, 64, 255) },
    { ThemedColor::TX_STATUS_OFFLINE, QColor(192, 192, 192) },
    { ThemedColor::TX_STATUS_DANGER, QColor(200, 100, 100) },
    { ThemedColor::TX_STATUS_LOCKED, QColor(0, 128, 255) },
};

static const std::map<ThemedColor, QColor> themedDarkColors = {
    { ThemedColor::DEFAULT, QColor(170, 170, 170) },
    { ThemedColor::UNCONFIRMED, QColor(204, 204, 204) },
    { ThemedColor::NEGATIVE, QColor(255, 69, 0) },
    { ThemedColor::BAREADDRESS, QColor(140, 140, 140) },
    { ThemedColor::TX_STATUS_OPENUNTILDATE, QColor(64, 64, 255) },
    { ThemedColor::TX_STATUS_OFFLINE, QColor(192, 192, 192) },
    { ThemedColor::TX_STATUS_DANGER, QColor(200, 100, 100) },
    { ThemedColor::TX_STATUS_LOCKED, QColor(0, 128, 255) },
};

static const std::map<ThemedStyle, QString> themedStyles = {
    { ThemedStyle::TS_INVALID, "background:#FF8080;" },
    { ThemedStyle::TS_ERROR, "color:red;" },
    { ThemedStyle::TS_SUCCESS, "color:green;" },
    { ThemedStyle::TS_COMMAND, "color:#006060;" },
    { ThemedStyle::TS_PRIMARY, "color:black;" },
    { ThemedStyle::TS_SECONDARY, "color:#808080;" },
};

static const std::map<ThemedStyle, QString> themedDarkStyles = {
    { ThemedStyle::TS_INVALID, "background:#ff4500;" },
    { ThemedStyle::TS_ERROR, "color:#ff4500;" },
    { ThemedStyle::TS_SUCCESS, "color:green;" },
    { ThemedStyle::TS_COMMAND, "color:#0cc;" },
    { ThemedStyle::TS_PRIMARY, "color:#ccc;" },
    { ThemedStyle::TS_SECONDARY, "color:#aaa;" },
};

QColor getThemedQColor(ThemedColor color)
{
    QString theme = QSettings().value("theme", "").toString();
    return theme.startsWith(darkThemePrefix) ? themedDarkColors.at(color) : themedColors.at(color);
}

QString getThemedStyleQString(ThemedStyle style)
{
    QString theme = QSettings().value("theme", "").toString();
    return theme.startsWith(darkThemePrefix) ? themedDarkStyles.at(style) : themedStyles.at(style);
}

QString dateTimeStr(const QDateTime &date)
{
    return date.date().toString(Qt::SystemLocaleShortDate) + QString(" ") + date.toString("hh:mm");
}

QString dateTimeStr(qint64 nTime)
{
    return dateTimeStr(QDateTime::fromTime_t((qint32)nTime));
}

QFont fixedPitchFont()
{
    if (dashThemeActive()) {
        return getFontNormal();
    } else {
        return QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }
}

// Just some dummy data to generate a convincing random-looking (but consistent) address
static const uint8_t dummydata[] = {0xeb,0x15,0x23,0x1d,0xfc,0xeb,0x60,0x92,0x58,0x86,0xb6,0x7d,0x06,0x52,0x99,0x92,0x59,0x15,0xae,0xb1,0x72,0xc0,0x66,0x47};

// Generate a dummy address with invalid CRC, starting with the network prefix.
static std::string DummyAddress(const CChainParams &params)
{
    std::vector<unsigned char> sourcedata = params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
    sourcedata.insert(sourcedata.end(), dummydata, dummydata + sizeof(dummydata));
    for(int i=0; i<256; ++i) { // Try every trailing byte
        std::string s = EncodeBase58(sourcedata.data(), sourcedata.data() + sourcedata.size());
        if (!IsValidDestinationString(s)) {
            return s;
        }
        sourcedata[sourcedata.size()-1] += 1;
    }
    return "";
}

void setupAddressWidget(QValidatedLineEdit *widget, QWidget *parent, bool fAllowURI)
{
    parent->setFocusProxy(widget);

    setFixedPitchFont({widget});
    // We don't want translators to use own addresses in translations
    // and this is the only place, where this address is supplied.
    widget->setPlaceholderText(QObject::tr("Enter a Dash address (e.g. %1)").arg(
        QString::fromStdString(DummyAddress(Params()))));
    widget->setValidator(new BitcoinAddressEntryValidator(parent, fAllowURI));
    widget->setCheckValidator(new BitcoinAddressCheckValidator(parent));
}

void setupAmountWidget(QLineEdit *widget, QWidget *parent)
{
    QDoubleValidator *amountValidator = new QDoubleValidator(parent);
    amountValidator->setDecimals(8);
    amountValidator->setBottom(0.0);
    widget->setValidator(amountValidator);
    widget->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
}

bool parseBitcoinURI(const QUrl &uri, SendCoinsRecipient *out)
{
    // return if URI is not valid or is no dash: URI
    if(!uri.isValid() || uri.scheme() != QString("dash"))
        return false;

    SendCoinsRecipient rv;
    rv.address = uri.path();
    // Trim any following forward slash which may have been added by the OS
    if (rv.address.endsWith("/")) {
        rv.address.truncate(rv.address.length() - 1);
    }
    rv.amount = 0;

    QUrlQuery uriQuery(uri);
    QList<QPair<QString, QString> > items = uriQuery.queryItems();

    for (QList<QPair<QString, QString> >::iterator i = items.begin(); i != items.end(); i++)
    {
        bool fShouldReturnFalse = false;
        if (i->first.startsWith("req-"))
        {
            i->first.remove(0, 4);
            fShouldReturnFalse = true;
        }

        if (i->first == "label")
        {
            rv.label = i->second;
            fShouldReturnFalse = false;
        }
        if (i->first == "IS")
        {
            // we simply ignore IS
            fShouldReturnFalse = false;
        }
        if (i->first == "message")
        {
            rv.message = i->second;
            fShouldReturnFalse = false;
        }
        else if (i->first == "amount")
        {
            if(!i->second.isEmpty())
            {
                if(!BitcoinUnits::parse(BitcoinUnits::DASH, i->second, &rv.amount))
                {
                    return false;
                }
            }
            fShouldReturnFalse = false;
        }

        if (fShouldReturnFalse)
            return false;
    }
    if(out)
    {
        *out = rv;
    }
    return true;
}

bool parseBitcoinURI(QString uri, SendCoinsRecipient *out)
{
    // Convert dash:// to dash:
    //
    //    Cannot handle this later, because dash:// will cause Qt to see the part after // as host,
    //    which will lower-case it (and thus invalidate the address).
    if(uri.startsWith("dash://", Qt::CaseInsensitive))
    {
        uri.replace(0, 7, "dash:");
    }
    QUrl uriInstance(uri);
    return parseBitcoinURI(uriInstance, out);
}

bool validateBitcoinURI(const QString& uri)
{
    SendCoinsRecipient rcp;
    return parseBitcoinURI(uri, &rcp);
}

QString formatBitcoinURI(const SendCoinsRecipient &info)
{
    QString ret = QString("dash:%1").arg(info.address);
    int paramCount = 0;

    if (info.amount)
    {
        ret += QString("?amount=%1").arg(BitcoinUnits::format(BitcoinUnits::DASH, info.amount, false, BitcoinUnits::separatorNever));
        paramCount++;
    }

    if (!info.label.isEmpty())
    {
        QString lbl(QUrl::toPercentEncoding(info.label));
        ret += QString("%1label=%2").arg(paramCount == 0 ? "?" : "&").arg(lbl);
        paramCount++;
    }

    if (!info.message.isEmpty())
    {
        QString msg(QUrl::toPercentEncoding(info.message));
        ret += QString("%1message=%2").arg(paramCount == 0 ? "?" : "&").arg(msg);
        paramCount++;
    }

    return ret;
}

bool isDust(const QString& address, const CAmount& amount)
{
    CTxDestination dest = DecodeDestination(address.toStdString());
    CScript script = GetScriptForDestination(dest);
    CTxOut txOut(amount, script);
    return IsDust(txOut, ::dustRelayFee);
}

QString HtmlEscape(const QString& str, bool fMultiLine)
{
    QString escaped = str.toHtmlEscaped();
    escaped = escaped.replace(" ", "&nbsp;");
    if(fMultiLine)
    {
        escaped = escaped.replace("\n", "<br>\n");
    }
    return escaped;
}

QString HtmlEscape(const std::string& str, bool fMultiLine)
{
    return HtmlEscape(QString::fromStdString(str), fMultiLine);
}

void copyEntryData(QAbstractItemView *view, int column, int role)
{
    if(!view || !view->selectionModel())
        return;
    QModelIndexList selection = view->selectionModel()->selectedRows(column);

    if(!selection.isEmpty())
    {
        // Copy first item
        setClipboard(selection.at(0).data(role).toString());
    }
}

QList<QModelIndex> getEntryData(QAbstractItemView *view, int column)
{
    if(!view || !view->selectionModel())
        return QList<QModelIndex>();
    return view->selectionModel()->selectedRows(column);
}

QString getSaveFileName(QWidget *parent, const QString &caption, const QString &dir,
    const QString &filter,
    QString *selectedSuffixOut)
{
    QString selectedFilter;
    QString myDir;
    if(dir.isEmpty()) // Default to user documents location
    {
        myDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    else
    {
        myDir = dir;
    }
    /* Directly convert path to native OS path separators */
    QString result = QDir::toNativeSeparators(QFileDialog::getSaveFileName(parent, caption, myDir, filter, &selectedFilter));

    /* Extract first suffix from filter pattern "Description (*.foo)" or "Description (*.foo *.bar ...) */
    QRegExp filter_re(".* \\(\\*\\.(.*)[ \\)]");
    QString selectedSuffix;
    if(filter_re.exactMatch(selectedFilter))
    {
        selectedSuffix = filter_re.cap(1);
    }

    /* Add suffix if needed */
    QFileInfo info(result);
    if(!result.isEmpty())
    {
        if(info.suffix().isEmpty() && !selectedSuffix.isEmpty())
        {
            /* No suffix specified, add selected suffix */
            if(!result.endsWith("."))
                result.append(".");
            result.append(selectedSuffix);
        }
    }

    /* Return selected suffix if asked to */
    if(selectedSuffixOut)
    {
        *selectedSuffixOut = selectedSuffix;
    }
    return result;
}

QString getOpenFileName(QWidget *parent, const QString &caption, const QString &dir,
    const QString &filter,
    QString *selectedSuffixOut)
{
    QString selectedFilter;
    QString myDir;
    if(dir.isEmpty()) // Default to user documents location
    {
        myDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    else
    {
        myDir = dir;
    }
    /* Directly convert path to native OS path separators */
    QString result = QDir::toNativeSeparators(QFileDialog::getOpenFileName(parent, caption, myDir, filter, &selectedFilter));

    if(selectedSuffixOut)
    {
        /* Extract first suffix from filter pattern "Description (*.foo)" or "Description (*.foo *.bar ...) */
        QRegExp filter_re(".* \\(\\*\\.(.*)[ \\)]");
        QString selectedSuffix;
        if(filter_re.exactMatch(selectedFilter))
        {
            selectedSuffix = filter_re.cap(1);
        }
        *selectedSuffixOut = selectedSuffix;
    }
    return result;
}

Qt::ConnectionType blockingGUIThreadConnection()
{
    if(QThread::currentThread() != qApp->thread())
    {
        return Qt::BlockingQueuedConnection;
    }
    else
    {
        return Qt::DirectConnection;
    }
}

bool checkPoint(const QPoint &p, const QWidget *w)
{
    QWidget *atW = QApplication::widgetAt(w->mapToGlobal(p));
    if (!atW) return false;
    return atW->topLevelWidget() == w;
}

bool isObscured(QWidget *w)
{
    return !(checkPoint(QPoint(0, 0), w)
        && checkPoint(QPoint(w->width() - 1, 0), w)
        && checkPoint(QPoint(0, w->height() - 1), w)
        && checkPoint(QPoint(w->width() - 1, w->height() - 1), w)
        && checkPoint(QPoint(w->width() / 2, w->height() / 2), w));
}

void bringToFront(QWidget* w)
{
#ifdef Q_OS_MAC
    ForceActivation();
#endif

    if (w) {
        // activateWindow() (sometimes) helps with keyboard focus on Windows
        if (w->isMinimized()) {
            w->showNormal();
        } else {
            w->show();
        }
        w->activateWindow();
        w->raise();
    }
}

void openDebugLogfile()
{
    fs::path pathDebug = GetDataDir() / "debug.log";

    /* Open debug.log with the associated application */
    if (fs::exists(pathDebug))
        QDesktopServices::openUrl(QUrl::fromLocalFile(boostPathToQString(pathDebug)));
}

void openConfigfile()
{
    fs::path pathConfig = GetConfigFile(gArgs.GetArg("-conf", BITCOIN_CONF_FILENAME));

    /* Open dash.conf with the associated application */
    if (fs::exists(pathConfig))
        QDesktopServices::openUrl(QUrl::fromLocalFile(boostPathToQString(pathConfig)));
}

void showBackups()
{
    fs::path backupsDir = GetBackupsDir();

    /* Open folder with default browser */
    if (fs::exists(backupsDir))
        QDesktopServices::openUrl(QUrl::fromLocalFile(boostPathToQString(backupsDir)));
}

void SubstituteFonts(const QString& language)
{
#if defined(Q_OS_MAC)
// Background:
// OSX's default font changed in 10.9 and Qt is unable to find it with its
// usual fallback methods when building against the 10.7 sdk or lower.
// The 10.8 SDK added a function to let it find the correct fallback font.
// If this fallback is not properly loaded, some characters may fail to
// render correctly.
//
// The same thing happened with 10.10. .Helvetica Neue DeskInterface is now default.
//
// Solution: If building with the 10.7 SDK or lower and the user's platform
// is 10.9 or higher at runtime, substitute the correct font. This needs to
// happen before the QApplication is created.
#if defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_8
    if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_8)
    {
        if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_9)
            /* On a 10.9 - 10.9.x system */
            QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
        else
        {
            /* 10.10 or later system */
            if (language == "zh_CN" || language == "zh_TW" || language == "zh_HK") // traditional or simplified Chinese
              QFont::insertSubstitution(".Helvetica Neue DeskInterface", "Heiti SC");
            else if (language == "ja") // Japanese
              QFont::insertSubstitution(".Helvetica Neue DeskInterface", "Songti SC");
            else
              QFont::insertSubstitution(".Helvetica Neue DeskInterface", "Lucida Grande");
        }
    }
#endif
#endif
}

ToolTipToRichTextFilter::ToolTipToRichTextFilter(int _size_threshold, QObject *parent) :
    QObject(parent),
    size_threshold(_size_threshold)
{

}

bool ToolTipToRichTextFilter::eventFilter(QObject *obj, QEvent *evt)
{
    if(evt->type() == QEvent::ToolTipChange)
    {
        QWidget *widget = static_cast<QWidget*>(obj);
        QString tooltip = widget->toolTip();
        if(tooltip.size() > size_threshold && !tooltip.startsWith("<qt"))
        {
            // Escape the current message as HTML and replace \n by <br> if it's not rich text
            if(!Qt::mightBeRichText(tooltip))
                tooltip = HtmlEscape(tooltip, true);
            // Envelop with <qt></qt> to make sure Qt detects every tooltip as rich text
            // and style='white-space:pre' to preserve line composition
            tooltip = "<qt style='white-space:pre'>" + tooltip + "</qt>";
            widget->setToolTip(tooltip);
            return true;
        }
    }
    return QObject::eventFilter(obj, evt);
}

void TableViewLastColumnResizingFixer::connectViewHeadersSignals()
{
    connect(tableView->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), this, SLOT(on_sectionResized(int,int,int)));
    connect(tableView->horizontalHeader(), SIGNAL(geometriesChanged()), this, SLOT(on_geometriesChanged()));
}

// We need to disconnect these while handling the resize events, otherwise we can enter infinite loops.
void TableViewLastColumnResizingFixer::disconnectViewHeadersSignals()
{
    disconnect(tableView->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), this, SLOT(on_sectionResized(int,int,int)));
    disconnect(tableView->horizontalHeader(), SIGNAL(geometriesChanged()), this, SLOT(on_geometriesChanged()));
}

// Setup the resize mode, handles compatibility for Qt5 and below as the method signatures changed.
// Refactored here for readability.
void TableViewLastColumnResizingFixer::setViewHeaderResizeMode(int logicalIndex, QHeaderView::ResizeMode resizeMode)
{
    tableView->horizontalHeader()->setSectionResizeMode(logicalIndex, resizeMode);
}

void TableViewLastColumnResizingFixer::resizeColumn(int nColumnIndex, int width)
{
    tableView->setColumnWidth(nColumnIndex, width);
    tableView->horizontalHeader()->resizeSection(nColumnIndex, width);
}

int TableViewLastColumnResizingFixer::getColumnsWidth()
{
    int nColumnsWidthSum = 0;
    for (int i = 0; i < columnCount; i++)
    {
        nColumnsWidthSum += tableView->horizontalHeader()->sectionSize(i);
    }
    return nColumnsWidthSum;
}

int TableViewLastColumnResizingFixer::getAvailableWidthForColumn(int column)
{
    int nResult = lastColumnMinimumWidth;
    int nTableWidth = tableView->horizontalHeader()->width();

    if (nTableWidth > 0)
    {
        int nOtherColsWidth = getColumnsWidth() - tableView->horizontalHeader()->sectionSize(column);
        nResult = std::max(nResult, nTableWidth - nOtherColsWidth);
    }

    return nResult;
}

// Make sure we don't make the columns wider than the table's viewport width.
void TableViewLastColumnResizingFixer::adjustTableColumnsWidth()
{
    disconnectViewHeadersSignals();
    resizeColumn(lastColumnIndex, getAvailableWidthForColumn(lastColumnIndex));
    connectViewHeadersSignals();

    int nTableWidth = tableView->horizontalHeader()->width();
    int nColsWidth = getColumnsWidth();
    if (nColsWidth > nTableWidth)
    {
        resizeColumn(secondToLastColumnIndex,getAvailableWidthForColumn(secondToLastColumnIndex));
    }
}

// Make column use all the space available, useful during window resizing.
void TableViewLastColumnResizingFixer::stretchColumnWidth(int column)
{
    disconnectViewHeadersSignals();
    resizeColumn(column, getAvailableWidthForColumn(column));
    connectViewHeadersSignals();
}

// When a section is resized this is a slot-proxy for ajustAmountColumnWidth().
void TableViewLastColumnResizingFixer::on_sectionResized(int logicalIndex, int oldSize, int newSize)
{
    adjustTableColumnsWidth();
    int remainingWidth = getAvailableWidthForColumn(logicalIndex);
    if (newSize > remainingWidth)
    {
       resizeColumn(logicalIndex, remainingWidth);
    }
}

// When the table's geometry is ready, we manually perform the stretch of the "Message" column,
// as the "Stretch" resize mode does not allow for interactive resizing.
void TableViewLastColumnResizingFixer::on_geometriesChanged()
{
    if ((getColumnsWidth() - this->tableView->horizontalHeader()->width()) != 0)
    {
        disconnectViewHeadersSignals();
        resizeColumn(secondToLastColumnIndex, getAvailableWidthForColumn(secondToLastColumnIndex));
        connectViewHeadersSignals();
    }
}

/**
 * Initializes all internal variables and prepares the
 * the resize modes of the last 2 columns of the table and
 */
TableViewLastColumnResizingFixer::TableViewLastColumnResizingFixer(QTableView* table, int lastColMinimumWidth, int allColsMinimumWidth, QObject *parent) :
    QObject(parent),
    tableView(table),
    lastColumnMinimumWidth(lastColMinimumWidth),
    allColumnsMinimumWidth(allColsMinimumWidth)
{
    columnCount = tableView->horizontalHeader()->count();
    lastColumnIndex = columnCount - 1;
    secondToLastColumnIndex = columnCount - 2;
    tableView->horizontalHeader()->setMinimumSectionSize(allColumnsMinimumWidth);
    setViewHeaderResizeMode(secondToLastColumnIndex, QHeaderView::Interactive);
    setViewHeaderResizeMode(lastColumnIndex, QHeaderView::Interactive);
}

#ifdef WIN32
fs::path static StartupShortcutPath()
{
    std::string chain = gArgs.GetChainName();
    if (chain == CBaseChainParams::MAIN)
        return GetSpecialFolderPath(CSIDL_STARTUP) / "Dash Core.lnk";
    if (chain == CBaseChainParams::TESTNET) // Remove this special case when CBaseChainParams::TESTNET = "testnet4"
        return GetSpecialFolderPath(CSIDL_STARTUP) / "Dash Core (testnet).lnk";
    return GetSpecialFolderPath(CSIDL_STARTUP) / strprintf("Dash Core (%s).lnk", chain);
}

bool GetStartOnSystemStartup()
{
    // check for "Dash Core*.lnk"
    return fs::exists(StartupShortcutPath());
}

bool SetStartOnSystemStartup(bool fAutoStart)
{
    // If the shortcut exists already, remove it for updating
    fs::remove(StartupShortcutPath());

    if (fAutoStart)
    {
        CoInitialize(nullptr);

        // Get a pointer to the IShellLink interface.
        IShellLink* psl = nullptr;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr,
            CLSCTX_INPROC_SERVER, IID_IShellLink,
            reinterpret_cast<void**>(&psl));

        if (SUCCEEDED(hres))
        {
            // Get the current executable path
            TCHAR pszExePath[MAX_PATH];
            GetModuleFileName(nullptr, pszExePath, sizeof(pszExePath));

            // Start client minimized
            QString strArgs = "-min";
            // Set -testnet /-regtest options
            strArgs += QString::fromStdString(strprintf(" -testnet=%d -regtest=%d", gArgs.GetBoolArg("-testnet", false), gArgs.GetBoolArg("-regtest", false)));

#ifdef UNICODE
            boost::scoped_array<TCHAR> args(new TCHAR[strArgs.length() + 1]);
            // Convert the QString to TCHAR*
            strArgs.toWCharArray(args.get());
            // Add missing '\0'-termination to string
            args[strArgs.length()] = '\0';
#endif

            // Set the path to the shortcut target
            psl->SetPath(pszExePath);
            PathRemoveFileSpec(pszExePath);
            psl->SetWorkingDirectory(pszExePath);
            psl->SetShowCmd(SW_SHOWMINNOACTIVE);
#ifndef UNICODE
            psl->SetArguments(strArgs.toStdString().c_str());
#else
            psl->SetArguments(args.get());
#endif

            // Query IShellLink for the IPersistFile interface for
            // saving the shortcut in persistent storage.
            IPersistFile* ppf = nullptr;
            hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
            if (SUCCEEDED(hres))
            {
                WCHAR pwsz[MAX_PATH];
                // Ensure that the string is ANSI.
                MultiByteToWideChar(CP_ACP, 0, StartupShortcutPath().string().c_str(), -1, pwsz, MAX_PATH);
                // Save the link by calling IPersistFile::Save.
                hres = ppf->Save(pwsz, TRUE);
                ppf->Release();
                psl->Release();
                CoUninitialize();
                return true;
            }
            psl->Release();
        }
        CoUninitialize();
        return false;
    }
    return true;
}
#elif defined(Q_OS_LINUX)

// Follow the Desktop Application Autostart Spec:
// http://standards.freedesktop.org/autostart-spec/autostart-spec-latest.html

fs::path static GetAutostartDir()
{
    char* pszConfigHome = getenv("XDG_CONFIG_HOME");
    if (pszConfigHome) return fs::path(pszConfigHome) / "autostart";
    char* pszHome = getenv("HOME");
    if (pszHome) return fs::path(pszHome) / ".config" / "autostart";
    return fs::path();
}

fs::path static GetAutostartFilePath()
{
    std::string chain = gArgs.GetChainName();
    if (chain == CBaseChainParams::MAIN)
        return GetAutostartDir() / "dashcore.desktop";
    return GetAutostartDir() / strprintf("dashcore-%s.lnk", chain);
}

bool GetStartOnSystemStartup()
{
    fs::ifstream optionFile(GetAutostartFilePath());
    if (!optionFile.good())
        return false;
    // Scan through file for "Hidden=true":
    std::string line;
    while (!optionFile.eof())
    {
        getline(optionFile, line);
        if (line.find("Hidden") != std::string::npos &&
            line.find("true") != std::string::npos)
            return false;
    }
    optionFile.close();

    return true;
}

bool SetStartOnSystemStartup(bool fAutoStart)
{
    if (!fAutoStart)
        fs::remove(GetAutostartFilePath());
    else
    {
        char pszExePath[MAX_PATH+1];
        ssize_t r = readlink("/proc/self/exe", pszExePath, sizeof(pszExePath) - 1);
        if (r == -1)
            return false;
        pszExePath[r] = '\0';

        fs::create_directories(GetAutostartDir());

        fs::ofstream optionFile(GetAutostartFilePath(), std::ios_base::out|std::ios_base::trunc);
        if (!optionFile.good())
            return false;
        std::string chain = gArgs.GetChainName();
        // Write a dashcore.desktop file to the autostart directory:
        optionFile << "[Desktop Entry]\n";
        optionFile << "Type=Application\n";
        if (chain == CBaseChainParams::MAIN)
            optionFile << "Name=Dash Core\n";
        else
            optionFile << strprintf("Name=Dash Core (%s)\n", chain);
        optionFile << "Exec=" << pszExePath << strprintf(" -min -testnet=%d -regtest=%d\n", gArgs.GetBoolArg("-testnet", false), gArgs.GetBoolArg("-regtest", false));
        optionFile << "Terminal=false\n";
        optionFile << "Hidden=false\n";
        optionFile.close();
    }
    return true;
}


#elif defined(Q_OS_MAC)
// based on: https://github.com/Mozketo/LaunchAtLoginController/blob/master/LaunchAtLoginController.m

LSSharedFileListItemRef findStartupItemInList(LSSharedFileListRef list, CFURLRef findUrl);
LSSharedFileListItemRef findStartupItemInList(LSSharedFileListRef list, CFURLRef findUrl)
{
    CFArrayRef listSnapshot = LSSharedFileListCopySnapshot(list, nullptr);
    if (listSnapshot == nullptr) {
        return nullptr;
    }

    // loop through the list of startup items and try to find the Dash Core app
    for(int i = 0; i < CFArrayGetCount(listSnapshot); i++) {
        LSSharedFileListItemRef item = (LSSharedFileListItemRef)CFArrayGetValueAtIndex(listSnapshot, i);
        UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;
        CFURLRef currentItemURL = nullptr;

#if defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= 10100
        if(&LSSharedFileListItemCopyResolvedURL)
            currentItemURL = LSSharedFileListItemCopyResolvedURL(item, resolutionFlags, nullptr);
#if defined(MAC_OS_X_VERSION_MIN_REQUIRED) && MAC_OS_X_VERSION_MIN_REQUIRED < 10100
        else
            LSSharedFileListItemResolve(item, resolutionFlags, &currentItemURL, nullptr);
#endif
#else
        LSSharedFileListItemResolve(item, resolutionFlags, &currentItemURL, nullptr);
#endif

        if(currentItemURL) {
            if (CFEqual(currentItemURL, findUrl)) {
                // found
                CFRelease(listSnapshot);
                CFRelease(currentItemURL);
                return item;
            }
            CFRelease(currentItemURL);
        }
    }

    CFRelease(listSnapshot);
    return nullptr;
}

bool GetStartOnSystemStartup()
{
    CFURLRef bitcoinAppUrl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    if (bitcoinAppUrl == nullptr) {
        return false;
    }

    LSSharedFileListRef loginItems = LSSharedFileListCreate(nullptr, kLSSharedFileListSessionLoginItems, nullptr);
    LSSharedFileListItemRef foundItem = findStartupItemInList(loginItems, bitcoinAppUrl);

    CFRelease(bitcoinAppUrl);
    return !!foundItem; // return boolified object
}

bool SetStartOnSystemStartup(bool fAutoStart)
{
    CFURLRef bitcoinAppUrl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    if (bitcoinAppUrl == nullptr) {
        return false;
    }

    LSSharedFileListRef loginItems = LSSharedFileListCreate(nullptr, kLSSharedFileListSessionLoginItems, nullptr);
    LSSharedFileListItemRef foundItem = findStartupItemInList(loginItems, bitcoinAppUrl);

    if(fAutoStart && !foundItem) {
        // add Dash Core app to startup item list
        LSSharedFileListInsertItemURL(loginItems, kLSSharedFileListItemBeforeFirst, nullptr, nullptr, bitcoinAppUrl, nullptr, nullptr);
    }
    else if(!fAutoStart && foundItem) {
        // remove item
        LSSharedFileListItemRemove(loginItems, foundItem);
    }

    CFRelease(bitcoinAppUrl);
    return true;
}
#pragma GCC diagnostic pop
#else

bool GetStartOnSystemStartup() { return false; }
bool SetStartOnSystemStartup(bool fAutoStart) { return false; }

#endif

void migrateQtSettings()
{
    // Migration (12.1)
    QSettings settings;
    if(!settings.value("fMigrationDone121", false).toBool()) {
        settings.remove("theme");
        settings.remove("nWindowPos");
        settings.remove("nWindowSize");
        settings.setValue("fMigrationDone121", true);
    }
}

// Open CSS when configured
QString loadStyleSheet()
{
    static std::unique_ptr<QString> stylesheet;

    if (stylesheet.get() == nullptr) {

        stylesheet = std::make_unique<QString>();

        QSettings settings;
        QDir themes(":themes");
        QString theme = settings.value("theme", "").toString();

        // Make sure settings are pointing to an existent theme
        if (theme.isEmpty() || !themes.exists(theme)) {
            theme = defaultTheme;
            settings.setValue("theme", theme);
        }

        // If light/dark theme is used load general styles first
        if (dashThemeActive()) {
            QFile qFileGeneral(":css/general");
            if (qFileGeneral.open(QFile::ReadOnly)) {
                stylesheet.get()->append(QLatin1String(qFileGeneral.readAll()));
            }

#ifndef Q_OS_MAC
            // Apply some styling to scrollbars
            QFile qFileScrollbars(QString(":/css/scrollbars"));
            if (qFileScrollbars.open(QFile::ReadOnly)) {
                stylesheet.get()->append(QLatin1String(qFileScrollbars.readAll()));
            }
#endif
        }

        QFile qFileTheme(":themes/" + theme);
        if (qFileTheme.open(QFile::ReadOnly)) {
            stylesheet.get()->append(QLatin1String(qFileTheme.readAll()));
        }
    }

    return *stylesheet.get();
}

FontFamily fontFamilyFromString(const QString& strFamily)
{
    if (strFamily == "SystemDefault") {
        return FontFamily::SystemDefault;
    }
    if (strFamily == "Montserrat") {
        return FontFamily::Montserrat;
    }
    throw std::invalid_argument(strprintf("Invalid font-family: %s", strFamily.toStdString()));
}

QString fontFamilyToString(FontFamily family)
{
    switch (family) {
    case FontFamily::SystemDefault:
        return "SystemDefault";
    case FontFamily::Montserrat:
        return "Montserrat";
    default:
        assert(false);
    }
}

void setFontFamily(FontFamily family)
{
    fontFamily = family;
    updateFonts();
}

FontFamily getFontFamilyDefault()
{
    return defaultFontFamily;
}

FontFamily getFontFamily()
{
    return fontFamily;
}

bool weightFromArg(int nArg, QFont::Weight& weight)
{
    const std::map<int, QFont::Weight> mapWeight{
        {0, QFont::Thin},
        {1, QFont::ExtraLight},
        {2, QFont::Light},
        {3, QFont::Normal},
        {4, QFont::Medium},
        {5, QFont::DemiBold},
        {6, QFont::Bold},
        {7, QFont::ExtraBold},
        {8, QFont::Black}
    };
    auto it = mapWeight.find(nArg);
    if (it == mapWeight.end()) {
        return false;
    }
    weight = it->second;
    return true;
}

int weightToArg(const QFont::Weight weight)
{
    const std::map<QFont::Weight, int> mapWeight{
        {QFont::Thin, 0},
        {QFont::ExtraLight, 1},
        {QFont::Light, 2},
        {QFont::Normal, 3},
        {QFont::Medium, 4},
        {QFont::DemiBold, 5},
        {QFont::Bold, 6},
        {QFont::ExtraBold, 7},
        {QFont::Black, 8}
    };
    assert(mapWeight.count(weight));
    return mapWeight.find(weight)->second;
}

QFont::Weight getFontWeightNormalDefault()
{
    return defaultFontWeightNormal;
}

QFont::Weight toQFontWeight(FontWeight weight)
{
    return weight == FontWeight::Bold ? getFontWeightBold() : getFontWeightNormal();
}

QFont::Weight getFontWeightNormal()
{
    return fontWeightNormal;
}

void setFontWeightNormal(QFont::Weight weight)
{
    fontWeightNormal = weight;
    updateFonts();
}

QFont::Weight getFontWeightBoldDefault()
{
    return defaultFontWeightBold;
}

QFont::Weight getFontWeightBold()
{
    return fontWeightBold;
}

void setFontWeightBold(QFont::Weight weight)
{
    fontWeightBold = weight;
    updateFonts();
}

int getFontScaleDefault()
{
    return defaultFontScale;
}

int getFontScale()
{
    return fontScale;
}

void setFontScale(int nScale)
{
    fontScale = nScale;
    updateFonts();
}

double getScaledFontSize(int nSize)
{
    return (nSize * (1 + (fontScale * fontScaleSteps)));
}

bool loadFonts()
{
    QString family = fontFamilyToString(FontFamily::Montserrat);
    QString italic = "Italic";

    std::map<QString, bool> mapStyles{
        {"Thin", true},
        {"ExtraLight", true},
        {"Light", true},
        {"Italic", false},
        {"Regular", false},
        {"Medium", true},
        {"SemiBold", true},
        {"Bold", true},
        {"ExtraBold", true},
        {"Black", true},
    };

    QFontDatabase database;
    std::vector<int> vecFontIds;

    for (const auto& it : mapStyles) {
        QString font = ":fonts/" + family + "-" + it.first;
        vecFontIds.push_back(QFontDatabase::addApplicationFont(font));
        qDebug() << __func__ << ": " << font << " loaded with id " << vecFontIds.back();
        if (it.second) {
            vecFontIds.push_back(QFontDatabase::addApplicationFont(font + italic));
            qDebug() << __func__ << ": " << font + italic << " loaded with id " << vecFontIds.back();
        }
    }

    // Fail if an added id is -1 which means QFontDatabase::addApplicationFont failed.
    if (std::find(vecFontIds.begin(), vecFontIds.end(), -1) != vecFontIds.end()) {
        return InitError("Font loading failed.");
    }

    // Print debug logs for added fonts fetched by the added ids
    for (const auto& i : vecFontIds) {
        auto families = QFontDatabase::applicationFontFamilies(i);
        for (const QString& f : families) {
            qDebug() << __func__ << ": - Font id " << i << " is family: " << f;
            const QStringList fontStyles = database.styles(f);
            for (const QString& style : fontStyles) {
                qDebug() << __func__ << ": Style for family " << f << " with id: " << i << ": " << style;
            }
        }
    }
    // Print debug logs for added fonts fetched by the family name
    const QStringList fontFamilies = database.families();
    for (const QString& f : fontFamilies) {
        if (f.contains(family)) {
            const QStringList fontStyles = database.styles(f);
            for (const QString& style : fontStyles) {
                qDebug() << __func__ << ": Family: " << f << ", Style: " << style;
            }
        }
    }

    setApplicationFont();

    return true;
}

void setApplicationFont()
{
    static QFont osDefaultFont = QApplication::font();

    std::unique_ptr<QFont> font;

    if (fontFamily == FontFamily::Montserrat) {
        QString family = fontFamilyToString(FontFamily::Montserrat);
#ifdef Q_OS_MAC
        if (getFontWeightNormal() != getFontWeightNormalDefault()) {
            font = std::make_unique<QFont>(getFontNormal());
        } else {
            font = std::make_unique<QFont>(family);
            font->setWeight(getFontWeightNormalDefault());
        }
#else
        font = std::make_unique<QFont>(family);
        font->setWeight(getFontWeightNormal());
#endif
    } else {
        font = std::make_unique<QFont>(osDefaultFont);
    }

    font->setPointSizeF(defaultFontSize);
    qApp->setFont(*font);

    qDebug() << __func__ << ": " << qApp->font().toString() <<
                " family: " << qApp->font().family() <<
                ", style: " << qApp->font().styleName() <<
                " match: " << qApp->font().exactMatch();
}

void setFont(const std::vector<QWidget*>& vecWidgets, FontWeight weight, int nPointSize, bool fItalic)
{
    QFont font = getFont(weight, fItalic, nPointSize);

    for (auto it : vecWidgets) {
        auto fontAttributes = std::make_pair(weight, fItalic);
        auto itw = mapNormalFontUpdates.emplace(std::make_pair(it, fontAttributes));
        if (!itw.second) itw.first->second = fontAttributes;

        if (nPointSize != -1) {
            auto its = mapFontSizeUpdates.emplace(std::make_pair(it, nPointSize));
            if (!its.second) its.first->second = nPointSize;
        }

        it->setFont(font);
    }
}

void setFixedPitchFont(const std::vector<QWidget*>& vecWidgets)
{
    for (auto it : vecWidgets) {
        setFixedPitchFontUpdates.emplace(it);
        it->setFont(fixedPitchFont());
    }
}

void updateFonts()
{
    setApplicationFont();

    auto getKey = [](QWidget* w) -> QString {
        return w->parent() ? w->parent()->objectName() + w->objectName() : w->objectName();
    };

    static std::map<QString, int> mapDefaultFontSizes;
    std::map<QWidget*, QFont> mapWidgetFonts;

    for (QWidget* w : qApp->allWidgets()) {
        QFont font = w->font();
        font.setFamily(qApp->font().family());
        font.setWeight(getFontWeightNormal());
        font.setStyleName(qApp->font().styleName());
        font.setStyle(qApp->font().style());
        // Set the font size based on the widgets default font size + the font scale
        QString key = getKey(w);
        if (!mapDefaultFontSizes.count(key)) {
            mapDefaultFontSizes.emplace(std::make_pair(key, font.pointSize() > 0 ? font.pointSize() : defaultFontSize));
        }
        font.setPointSizeF(getScaledFontSize(mapDefaultFontSizes[key]));
        mapWidgetFonts.emplace(w, font);
    }

    auto itn = mapNormalFontUpdates.begin();
    while (itn != mapNormalFontUpdates.end()) {
        if (mapWidgetFonts.count(itn->first)) {
            mapWidgetFonts[itn->first] = getFont(itn->second.first, itn->second.second, mapDefaultFontSizes[getKey(itn->first)]);
            ++itn;
        } else {
            itn = mapNormalFontUpdates.erase(itn);
        }
    }
    auto its = mapFontSizeUpdates.begin();
    while (its != mapFontSizeUpdates.end()) {
        if (mapWidgetFonts.count(its->first)) {
            QFont font = mapWidgetFonts[its->first];
            font.setPointSizeF(getScaledFontSize(its->second));
            mapWidgetFonts[its->first] = font;
            ++its;
        } else {
            its = mapFontSizeUpdates.erase(its);
        }
    }
    auto itf = setFixedPitchFontUpdates.begin();
    while (itf != setFixedPitchFontUpdates.end()) {
        if (mapWidgetFonts.count(*itf)) {
            QFont font = fixedPitchFont();
            font.setPointSizeF(getScaledFontSize(mapDefaultFontSizes[getKey(*itf)]));
            mapWidgetFonts[*itf] = font;
            ++itf;
        } else {
            itf = setFixedPitchFontUpdates.erase(itf);
        }
    }

    for (auto it : mapWidgetFonts) {
        it.first->setFont(it.second);
    }
}

QFont getFont(FontWeight weight, bool fItalic, int nPointSize)
{
    QFont font;
    QFont::Weight qWeight = toQFontWeight(weight);

    if (fontFamily == FontFamily::Montserrat) {

        static std::map<QFont::Weight, QString> mapMontserratMapping{
            {QFont::Thin, "Thin"},
            {QFont::ExtraLight, "ExtraLight"},
            {QFont::Light, "Light"},
            {QFont::Medium, "Medium"},
            {QFont::DemiBold, "SemiBold"},
            {QFont::ExtraBold, "ExtraBold"},
            {QFont::Black, "Black"},
#ifdef Q_OS_MAC
            {QFont::Normal, "Regular"},
            {QFont::Bold, "Bold"},
#else
            {QFont::Normal, ""},
            {QFont::Bold, ""},
#endif
        };

        assert(mapMontserratMapping.count(qWeight));

#ifdef Q_OS_MAC

        QString styleName = mapMontserratMapping[qWeight];

        if (fItalic) {
            if (styleName == "Regular") {
                styleName = "Italic";
            } else {
                styleName += " Italic";
            }
        }

        font.setFamily(fontFamilyToString(FontFamily::Montserrat));
        font.setStyleName(styleName);
#else
        font.setFamily(fontFamilyToString(FontFamily::Montserrat) + " " + mapMontserratMapping[qWeight]);
        font.setWeight(qWeight);
        font.setStyle(fItalic ? QFont::StyleItalic : QFont::StyleNormal);
#endif
    } else {
        font.setFamily(QApplication::font().family());
        font.setWeight(qWeight);
        font.setStyle(fItalic ? QFont::StyleItalic : QFont::StyleNormal);
    }
    qDebug() << "GUIUtil::getFont() - " << font.toString() << " family: " << font.family() << ", style: " << font.styleName() << " match: " << font.exactMatch();

    if (nPointSize != -1) {
        font.setPointSizeF(getScaledFontSize(nPointSize));
    }

    return font;
}

QFont getFontNormal()
{
    return getFont(FontWeight::Normal);
}

QFont getFontBold()
{
    return getFont(FontWeight::Bold);
}

bool dashThemeActive()
{
    QSettings settings;
    QString theme = settings.value("theme", "").toString();
    return theme != traditionalTheme;
}

void disableMacFocusRect(const QWidget* w)
{
#ifdef Q_OS_MAC
    for (const auto& c : w->findChildren<QWidget*>()) {
        if (c->testAttribute(Qt::WA_MacShowFocusRect)) {
            c->setAttribute(Qt::WA_MacShowFocusRect, !dashThemeActive());
        }
    }
#endif
}

void setClipboard(const QString& str)
{
    QApplication::clipboard()->setText(str, QClipboard::Clipboard);
    QApplication::clipboard()->setText(str, QClipboard::Selection);
}

fs::path qstringToBoostPath(const QString &path)
{
    return fs::path(path.toStdString(), utf8);
}

QString boostPathToQString(const fs::path &path)
{
    return QString::fromStdString(path.string(utf8));
}

QString formatDurationStr(int secs)
{
    QStringList strList;
    int days = secs / 86400;
    int hours = (secs % 86400) / 3600;
    int mins = (secs % 3600) / 60;
    int seconds = secs % 60;

    if (days)
        strList.append(QString(QObject::tr("%1 d")).arg(days));
    if (hours)
        strList.append(QString(QObject::tr("%1 h")).arg(hours));
    if (mins)
        strList.append(QString(QObject::tr("%1 m")).arg(mins));
    if (seconds || (!days && !hours && !mins))
        strList.append(QString(QObject::tr("%1 s")).arg(seconds));

    return strList.join(" ");
}

QString formatServicesStr(quint64 mask)
{
    QStringList strList;

    // Just scan the last 8 bits for now.
    for (int i = 0; i < 8; i++) {
        uint64_t check = 1 << i;
        if (mask & check)
        {
            switch (check)
            {
            case NODE_NETWORK:
                strList.append("NETWORK");
                break;
            case NODE_GETUTXO:
                strList.append("GETUTXO");
                break;
            case NODE_BLOOM:
                strList.append("BLOOM");
                break;
            case NODE_XTHIN:
                strList.append("XTHIN");
                break;
            default:
                strList.append(QString("%1[%2]").arg("UNKNOWN").arg(check));
            }
        }
    }

    if (strList.size())
        return strList.join(" & ");
    else
        return QObject::tr("None");
}

QString formatPingTime(double dPingTime)
{
    return (dPingTime == std::numeric_limits<int64_t>::max()/1e6 || dPingTime == 0) ? QObject::tr("N/A") : QString(QObject::tr("%1 ms")).arg(QString::number((int)(dPingTime * 1000), 10));
}

QString formatTimeOffset(int64_t nTimeOffset)
{
  return QString(QObject::tr("%1 s")).arg(QString::number((int)nTimeOffset, 10));
}

QString formatNiceTimeOffset(qint64 secs)
{
    // Represent time from last generated block in human readable text
    QString timeBehindText;
    const int HOUR_IN_SECONDS = 60*60;
    const int DAY_IN_SECONDS = 24*60*60;
    const int WEEK_IN_SECONDS = 7*24*60*60;
    const int YEAR_IN_SECONDS = 31556952; // Average length of year in Gregorian calendar
    if(secs < 60)
    {
        timeBehindText = QObject::tr("%n second(s)","",secs);
    }
    else if(secs < 2*HOUR_IN_SECONDS)
    {
        timeBehindText = QObject::tr("%n minute(s)","",secs/60);
    }
    else if(secs < 2*DAY_IN_SECONDS)
    {
        timeBehindText = QObject::tr("%n hour(s)","",secs/HOUR_IN_SECONDS);
    }
    else if(secs < 2*WEEK_IN_SECONDS)
    {
        timeBehindText = QObject::tr("%n day(s)","",secs/DAY_IN_SECONDS);
    }
    else if(secs < YEAR_IN_SECONDS)
    {
        timeBehindText = QObject::tr("%n week(s)","",secs/WEEK_IN_SECONDS);
    }
    else
    {
        qint64 years = secs / YEAR_IN_SECONDS;
        qint64 remainder = secs % YEAR_IN_SECONDS;
        timeBehindText = QObject::tr("%1 and %2").arg(QObject::tr("%n year(s)", "", years)).arg(QObject::tr("%n week(s)","", remainder/WEEK_IN_SECONDS));
    }
    return timeBehindText;
}

QString formatBytes(uint64_t bytes)
{
    if(bytes < 1024)
        return QString(QObject::tr("%1 B")).arg(bytes);
    if(bytes < 1024 * 1024)
        return QString(QObject::tr("%1 KB")).arg(bytes / 1024);
    if(bytes < 1024 * 1024 * 1024)
        return QString(QObject::tr("%1 MB")).arg(bytes / 1024 / 1024);

    return QString(QObject::tr("%1 GB")).arg(bytes / 1024 / 1024 / 1024);
}

qreal calculateIdealFontSize(int width, const QString& text, QFont font, qreal minPointSize, qreal font_size) {
    while(font_size >= minPointSize) {
        font.setPointSizeF(font_size);
        QFontMetrics fm(font);
        if (fm.width(text) < width) {
            break;
        }
        font_size -= 0.5;
    }
    return font_size;
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_EMIT clicked(event->pos());
}

void ClickableProgressBar::mouseReleaseEvent(QMouseEvent *event)
{
    Q_EMIT clicked(event->pos());
}

} // namespace GUIUtil
