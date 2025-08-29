// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/guiutil.h>

#include <qt/appearancewidget.h>
#include <qt/bitcoinaddressvalidator.h>
#include <qt/bitcoingui.h>
#include <qt/bitcoinunits.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/sendcoinsrecipient.h>

#include <base58.h>
#include <chainparams.h>
#include <fs.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <protocol.h>
#include <script/script.h>
#include <script/standard.h>
#include <util/system.h>
#include <util/time.h>

#include <cmath>

#ifdef WIN32
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#endif

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QJsonObject>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLatin1String>
#include <QLineEdit>
#include <QList>
#include <QLocale>
#include <QMenu>
#include <QMouseEvent>
#include <QPluginLoader>
#include <QPointer>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QSize>
#include <QString>
#include <QTextDocument> // for Qt::mightBeRichText
#include <QThread>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QtGlobal>

#include <chrono>
#include <exception>
#include <fstream>
#include <string>
#include <vector>

#if defined(Q_OS_MACOS)

#include <QProcess>

void ForceActivation();
#endif

using namespace std::chrono_literals;

namespace GUIUtil {

static RecursiveMutex cs_css;
// The default stylesheet directory
static const QString defaultStylesheetDirectory = ":css";
// The actual stylesheet directory
static QString stylesheetDirectory = defaultStylesheetDirectory;
// The name of the traditional theme
static const QString traditionalTheme = "Traditional";
// The theme to set by default if settings are missing or incorrect
static const QString defaultTheme = "Light";
// The prefix a theme name should have if we want to apply dark colors and styles to it
static const QString darkThemePrefix = "Dark";
// The theme to set as a base one for non-traditional themes
static const QString generalTheme = "general";
// Mapping theme => css file
static const std::map<QString, QString> mapThemeToStyle{
    {generalTheme, "general.css"},
    {"Dark", "dark.css"},
    {"Light", "light.css"},
    {"Traditional", "traditional.css"},
};

/** loadFonts stores the SystemDefault font in osDefaultFont to be able to reference it later again */
static std::unique_ptr<QFont> osDefaultFont;
/** Font related default values. */
static const FontFamily defaultFontFamily = FontFamily::SystemDefault;
static const int defaultFontSize = 12;
static const double fontScaleSteps = 0.01;
#ifdef Q_OS_MACOS
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
// Contains the weight settings separated for all available fonts
static std::map<FontFamily, std::pair<QFont::Weight, QFont::Weight>> mapDefaultWeights;
static std::map<FontFamily, std::pair<QFont::Weight, QFont::Weight>> mapWeights;
// Contains all widgets and its font attributes (weight, italic, size) with font changes due to GUIUtil::setFont
static std::map<QPointer<QWidget>, std::tuple<FontWeight, bool, int>> mapFontUpdates;
// Contains a list of supported font weights for all members of GUIUtil::FontFamily
static std::map<FontFamily, std::vector<QFont::Weight>> mapSupportedWeights;

#ifdef Q_OS_MACOS
// Contains all widgets where the macOS focus rect has been disabled.
static std::set<QWidget*> setRectsDisabled;
#endif

static const std::map<ThemedColor, QColor> themedColors = {
    { ThemedColor::DEFAULT, QColor(85, 85, 85) },
    { ThemedColor::UNCONFIRMED, QColor(128, 128, 128) },
    { ThemedColor::BLUE, QColor(0, 141, 228) },
    { ThemedColor::ORANGE, QColor(199, 147, 4) },
    { ThemedColor::RED, QColor(168, 72, 50) },
    { ThemedColor::GREEN, QColor(94, 140, 65) },
    { ThemedColor::BAREADDRESS, QColor(140, 140, 140) },
    { ThemedColor::TX_STATUS_OPENUNTILDATE, QColor(64, 64, 255) },
    { ThemedColor::BACKGROUND_WIDGET, QColor(234, 234, 236) },
    { ThemedColor::BORDER_WIDGET, QColor(220, 220, 220) },
    { ThemedColor::BACKGROUND_NETSTATS, QColor(210, 210, 210, 230) },
    { ThemedColor::BORDER_NETSTATS, QColor(180, 180, 180) },
    { ThemedColor::QR_PIXEL, QColor(85, 85, 85) },
    { ThemedColor::ICON_ALTERNATIVE_COLOR, QColor(167, 167, 167) },
};

static const std::map<ThemedColor, QColor> themedDarkColors = {
    { ThemedColor::DEFAULT, QColor(199, 199, 199) },
    { ThemedColor::UNCONFIRMED, QColor(170, 170, 170) },
    { ThemedColor::BLUE, QColor(0, 89, 154) },
    { ThemedColor::ORANGE, QColor(199, 147, 4) },
    { ThemedColor::RED, QColor(168, 72, 50) },
    { ThemedColor::GREEN, QColor(94, 140, 65) },
    { ThemedColor::BAREADDRESS, QColor(140, 140, 140) },
    { ThemedColor::TX_STATUS_OPENUNTILDATE, QColor(64, 64, 255) },
    { ThemedColor::BACKGROUND_WIDGET, QColor(45, 45, 46) },
    { ThemedColor::BORDER_WIDGET, QColor(74, 74, 75) },
    { ThemedColor::BACKGROUND_NETSTATS, QColor(45, 45, 46, 230) },
    { ThemedColor::BORDER_NETSTATS, QColor(74, 74, 75) },
    { ThemedColor::QR_PIXEL, QColor(199, 199, 199) },
    { ThemedColor::ICON_ALTERNATIVE_COLOR, QColor(74, 74, 75) },
};

static const std::map<ThemedStyle, QString> themedStyles = {
    { ThemedStyle::TS_INVALID, "background:#a84832;" },
    { ThemedStyle::TS_ERROR, "color:#a84832;" },
    { ThemedStyle::TS_WARNING, "color:#999900;" },
    { ThemedStyle::TS_SUCCESS, "color:#5e8c41;" },
    { ThemedStyle::TS_COMMAND, "color:#008de4;" },
    { ThemedStyle::TS_PRIMARY, "color:#333;" },
    { ThemedStyle::TS_SECONDARY, "color:#444;" },
};

static const std::map<ThemedStyle, QString> themedDarkStyles = {
    { ThemedStyle::TS_INVALID, "background:#a84832;" },
    { ThemedStyle::TS_ERROR, "color:#a84832;" },
    { ThemedStyle::TS_WARNING, "color:#999900;" },
    { ThemedStyle::TS_SUCCESS, "color:#5e8c41;" },
    { ThemedStyle::TS_COMMAND, "color:#00599a;" },
    { ThemedStyle::TS_PRIMARY, "color:#c7c7c7;" },
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

QIcon getIcon(const QString& strIcon, const ThemedColor color, const ThemedColor colorAlternative, const QString& strIconPath)
{
    QColor qcolor = getThemedQColor(color);
    QColor qcolorAlternative = getThemedQColor(colorAlternative);
    QIcon icon(strIconPath + strIcon);
    QIcon themedIcon;
    for (const QSize& size : icon.availableSizes()) {
        QImage image(icon.pixmap(size).toImage());
        image = image.convertToFormat(QImage::Format_ARGB32);
        for (int x = 0; x < image.width(); ++x) {
            for (int y = 0; y < image.height(); ++y) {
                const QRgb rgb = image.pixel(x, y);
                QColor* pColor;
                if ((rgb & RGB_MASK) < RGB_HALF) {
                    pColor = &qcolor;
                } else {
                    pColor = &qcolorAlternative;
                }
                image.setPixel(x, y, qRgba(pColor->red(), pColor->green(), pColor->blue(), qAlpha(rgb)));
            }
        }
        themedIcon.addPixmap(QPixmap::fromImage(image));
    }
    return themedIcon;
}

QIcon getIcon(const QString& strIcon, const ThemedColor color, const QString& strIconPath)
{
    return getIcon(strIcon, color, ThemedColor::ICON_ALTERNATIVE_COLOR, strIconPath);
}

void setIcon(QAbstractButton* button, const QString& strIcon, const ThemedColor color, const ThemedColor colorAlternative, const QSize& size)
{
    button->setIcon(getIcon(strIcon, color, colorAlternative));
    button->setIconSize(size);
}

void setIcon(QAbstractButton* button, const QString& strIcon, const ThemedColor color, const QSize& size)
{
    setIcon(button, strIcon, color, ThemedColor::ICON_ALTERNATIVE_COLOR, size);
}

QString dateTimeStr(const QDateTime &date)
{
    return QLocale::system().toString(date.date(), QLocale::ShortFormat) + QString(" ") + date.toString("hh:mm");
}

QString dateTimeStr(qint64 nTime)
{
    return dateTimeStr(QDateTime::fromSecsSinceEpoch((qint32)nTime));
}

QFont fixedPitchFont(bool use_embedded_font)
{
    if (use_embedded_font) {
        return {"Roboto Mono"};
    }
    return QFontDatabase::systemFont(QFontDatabase::FixedFont);
}

// Just some dummy data to generate a convincing random-looking (but consistent) address
static const uint8_t dummydata[] = {0xeb,0x15,0x23,0x1d,0xfc,0xeb,0x60,0x92,0x58,0x86,0xb6,0x7d,0x06,0x52,0x99,0x92,0x59,0x15,0xae,0xb1,0x72,0xc0,0x66,0x47};

// Generate a dummy address with invalid CRC, starting with the network prefix.
static std::string DummyAddress(const CChainParams &params)
{
    std::vector<unsigned char> sourcedata = params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
    sourcedata.insert(sourcedata.end(), dummydata, dummydata + sizeof(dummydata));
    for(int i=0; i<256; ++i) { // Try every trailing byte
        std::string s = EncodeBase58(sourcedata);
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

    // We don't want translators to use own addresses in translations
    // and this is the only place, where this address is supplied.
    widget->setPlaceholderText(QObject::tr("Enter a Dash address (e.g. %1)").arg(
        QString::fromStdString(DummyAddress(Params()))));
    widget->setValidator(new BitcoinAddressEntryValidator(parent, fAllowURI));
    widget->setCheckValidator(new BitcoinAddressCheckValidator(parent));
}

void setupAppearance(QWidget* parent, OptionsModel* model)
{
    if (!QSettings().value("fAppearanceSetupDone", false).toBool()) {
        // Create the dialog
        QDialog dlg(parent);
        dlg.setObjectName("AppearanceSetup");
        dlg.setWindowTitle(QObject::tr("Appearance Setup"));
        dlg.setWindowIcon(QIcon(":icons/dash"));
        // And the widgets we add to it
        QLabel lblHeading(QObject::tr("Please choose your preferred settings for the appearance of %1").arg(PACKAGE_NAME), &dlg);
        lblHeading.setObjectName("lblHeading");
        lblHeading.setWordWrap(true);
        QLabel lblSubHeading(QObject::tr("This can also be adjusted later in the \"Appearance\" tab of the preferences."), &dlg);
        lblSubHeading.setObjectName("lblSubHeading");
        lblSubHeading.setWordWrap(true);
        AppearanceWidget appearance(&dlg);
        appearance.setModel(model);
        QFrame line(&dlg);
        line.setFrameShape(QFrame::HLine);
        QDialogButtonBox buttonBox(QDialogButtonBox::Save);
        // Put them into a vbox and add the vbox to the dialog
        QVBoxLayout layout;
        layout.addWidget(&lblHeading);
        layout.addWidget(&lblSubHeading);
        layout.addWidget(&line);
        layout.addWidget(&appearance);
        layout.addWidget(&buttonBox);
        dlg.setLayout(&layout);
        // Adjust the headings
        setFont({&lblHeading}, FontWeight::Bold, 16);
        setFont({&lblSubHeading}, FontWeight::Normal, 14, true);
        // Make sure the dialog closes and accepts the settings if save has been pressed
        QObject::connect(&buttonBox, &QDialogButtonBox::accepted, [&]() {
            QSettings().setValue("fAppearanceSetupDone", true);
            appearance.accept();
            dlg.accept();
        });
        // And fire it!
        dlg.exec();
    }
}

void AddButtonShortcut(QAbstractButton* button, const QKeySequence& shortcut)
{
    QObject::connect(new QShortcut(shortcut, button), &QShortcut::activated, [button]() { button->animateClick(); });
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
                if (!BitcoinUnits::parse(BitcoinUnit::DASH, i->second, &rv.amount)) {
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
        ret += QString("?amount=%1").arg(BitcoinUnits::format(BitcoinUnit::DASH, info.amount, false, BitcoinUnits::SeparatorStyle::NEVER));
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

bool isDust(interfaces::Node& node, const QString& address, const CAmount& amount)
{
    CTxDestination dest = DecodeDestination(address.toStdString());
    CScript script = GetScriptForDestination(dest);
    CTxOut txOut(amount, script);
    return IsDust(txOut, node.getDustRelayFee());
}

QString HtmlEscape(const QString& str, bool fMultiLine)
{
    QString escaped = str.toHtmlEscaped();
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

void copyEntryData(const QAbstractItemView *view, int column, int role)
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

QList<QModelIndex> getEntryData(const QAbstractItemView *view, int column)
{
    if(!view || !view->selectionModel())
        return QList<QModelIndex>();
    return view->selectionModel()->selectedRows(column);
}

bool hasEntryData(const QAbstractItemView *view, int column, int role)
{
    QModelIndexList selection = getEntryData(view, column);
    if (selection.isEmpty()) return false;
    return !selection.at(0).data(role).toString().isEmpty();
}

void LoadFont(const QString& file_name)
{
    const int id = QFontDatabase::addApplicationFont(file_name);
    assert(id != -1);
}

QString getDefaultDataDirectory()
{
    return PathToQString(GetDefaultDataDir());
}

QString ExtractFirstSuffixFromFilter(const QString& filter)
{
    QRegularExpression filter_re(QStringLiteral(".* \\(\\*\\.(.*)[ \\)]"), QRegularExpression::InvertedGreedinessOption);
    QString suffix;
    QRegularExpressionMatch m = filter_re.match(filter);
    if (m.hasMatch()) {
        suffix = m.captured(1);
    }
    return suffix;
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

    QString selectedSuffix = ExtractFirstSuffixFromFilter(selectedFilter);

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
        *selectedSuffixOut = ExtractFirstSuffixFromFilter(selectedFilter);
        ;
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
    return atW->window() == w;
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
#ifdef Q_OS_MACOS
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

void handleCloseWindowShortcut(QWidget* w)
{
    QObject::connect(new QShortcut(QKeySequence(QObject::tr("Ctrl+W")), w), &QShortcut::activated, w, &QWidget::close);
}

void openDebugLogfile()
{
    fs::path pathDebug = gArgs.GetDataDirNet() / "debug.log";

    /* Open debug.log with the associated application */
    if (fs::exists(pathDebug))
        QDesktopServices::openUrl(QUrl::fromLocalFile(PathToQString(pathDebug)));
}

void openConfigfile()
{
    fs::path pathConfig = GetConfigFile(gArgs.GetPathArg("-conf", BITCOIN_CONF_FILENAME));

    /* Open dash.conf with the associated application */
    if (fs::exists(pathConfig)) {
        // Workaround for macOS-specific behavior; see #15409.
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(PathToQString(pathConfig)))) {
#ifdef Q_OS_MACOS
            QProcess::startDetached("/usr/bin/open", QStringList{"-t", PathToQString(pathConfig)});
#endif
            return;
        }
    }
}

void showBackups()
{
    fs::path backupsDir = gArgs.GetBackupsDirPath();

    /* Open folder with default browser */
    if (fs::exists(backupsDir))
        QDesktopServices::openUrl(QUrl::fromLocalFile(PathToQString(backupsDir)));
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

LabelOutOfFocusEventFilter::LabelOutOfFocusEventFilter(QObject* parent)
    : QObject(parent)
{
}

bool LabelOutOfFocusEventFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::FocusOut) {
        auto focus_out = static_cast<QFocusEvent*>(event);
        if (focus_out->reason() != Qt::PopupFocusReason) {
            auto label = qobject_cast<QLabel*>(watched);
            if (label) {
                auto flags = label->textInteractionFlags();
                label->setTextInteractionFlags(Qt::NoTextInteraction);
                label->setTextInteractionFlags(flags);
            }
        }
    }

    return QObject::eventFilter(watched, event);
}

#ifdef WIN32
fs::path static StartupShortcutPath()
{
    std::string chain = gArgs.GetChainName();
    if (chain == CBaseChainParams::MAIN)
        return GetSpecialFolderPath(CSIDL_STARTUP) / "Dash Core.lnk";
    if (chain == CBaseChainParams::TESTNET) // Remove this special case when CBaseChainParams::TESTNET = "testnet4"
        return GetSpecialFolderPath(CSIDL_STARTUP) / "Dash Core (testnet).lnk";
    return GetSpecialFolderPath(CSIDL_STARTUP) / fs::u8path(strprintf("Dash Core (%s).lnk", chain));
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
        IShellLinkW* psl = nullptr;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr,
            CLSCTX_INPROC_SERVER, IID_IShellLinkW,
            reinterpret_cast<void**>(&psl));

        if (SUCCEEDED(hres))
        {
            // Get the current executable path
            WCHAR pszExePath[MAX_PATH];
            GetModuleFileNameW(nullptr, pszExePath, ARRAYSIZE(pszExePath));

            // Start client minimized
            QString strArgs = "-min";
            // Set -testnet /-regtest options
            strArgs += QString::fromStdString(strprintf(" -chain=%s", gArgs.GetChainName()));

            // Set the path to the shortcut target
            psl->SetPath(pszExePath);
            PathRemoveFileSpecW(pszExePath);
            psl->SetWorkingDirectory(pszExePath);
            psl->SetShowCmd(SW_SHOWMINNOACTIVE);
            psl->SetArguments(strArgs.toStdWString().c_str());

            // Query IShellLink for the IPersistFile interface for
            // saving the shortcut in persistent storage.
            IPersistFile* ppf = nullptr;
            hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
            if (SUCCEEDED(hres))
            {
                // Save the link by calling IPersistFile::Save.
                hres = ppf->Save(StartupShortcutPath().wstring().c_str(), TRUE);
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
// https://specifications.freedesktop.org/autostart-spec/autostart-spec-latest.html

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
    return GetAutostartDir() / fs::u8path(strprintf("dashcore-%s.desktop", chain));
}

bool GetStartOnSystemStartup()
{
    std::ifstream optionFile{GetAutostartFilePath()};
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
        ssize_t r = readlink("/proc/self/exe", pszExePath, sizeof(pszExePath));
        if (r == -1 || r > MAX_PATH) {
            return false;
        }
        pszExePath[r] = '\0';

        fs::create_directories(GetAutostartDir());

        std::ofstream optionFile{GetAutostartFilePath(), std::ios_base::out | std::ios_base::trunc};
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
        optionFile << "Exec=" << pszExePath << strprintf(" -min -chain=%s\n", chain);
        optionFile << "Terminal=false\n";
        optionFile << "Hidden=false\n";
        optionFile.close();
    }
    return true;
}

#else

bool GetStartOnSystemStartup() { return false; }
bool SetStartOnSystemStartup(bool fAutoStart) { return false; }

#endif

void setStyleSheetDirectory(const QString& path)
{
    stylesheetDirectory = path;
}

bool isStyleSheetDirectoryCustom()
{
    return stylesheetDirectory != defaultStylesheetDirectory;
}

const std::vector<QString> listStyleSheets()
{
    std::vector<QString> vecStylesheets;
    for (const auto& it : mapThemeToStyle) {
        vecStylesheets.push_back(it.second);
    }
    return vecStylesheets;
}

const std::vector<QString> listThemes()
{
    std::vector<QString> vecThemes;
    for (const auto& it : mapThemeToStyle) {
        if (it.first == generalTheme) continue;
        vecThemes.push_back(it.first);
    }
    return vecThemes;
}

const QString getDefaultTheme()
{
    return defaultTheme;
}

bool isValidTheme(const QString& strTheme)
{
    return strTheme == defaultTheme || strTheme == darkThemePrefix || strTheme == traditionalTheme;
}

void loadStyleSheet(bool fForceUpdate)
{
    AssertLockNotHeld(cs_css);
    LOCK(cs_css);

    static std::unique_ptr<QString> stylesheet;

    bool fDebugCustomStyleSheets = gArgs.GetBoolArg("-debug-ui", false) && isStyleSheetDirectoryCustom();
    bool fStyleSheetChanged = false;

    if (stylesheet == nullptr || fForceUpdate || fDebugCustomStyleSheets) {
        auto hasModified = [](const std::vector<QString>& vecFiles) -> bool {
            static std::map<const QString, QDateTime> mapLastModified;

            bool fModified = false;
            for (auto file = vecFiles.begin(); file != vecFiles.end() && !fModified; ++file) {
                QFileInfo info(*file);
                QDateTime lastModified = info.lastModified(), prevLastModified;
                auto it = mapLastModified.emplace(std::make_pair(*file, lastModified));
                prevLastModified = it.second ? QDateTime() : it.first->second;
                it.first->second = lastModified;
                fModified = prevLastModified != lastModified;
            }
            return fModified;
        };

        auto loadFiles = [&](const std::vector<QString>& vecFiles) -> bool {
            if (!fForceUpdate && fDebugCustomStyleSheets && !hasModified(vecFiles)) {
                return false;
            }

            std::string platformName = gArgs.GetArg("-uiplatform", BitcoinGUI::DEFAULT_UIPLATFORM);
            stylesheet = std::make_unique<QString>();

            for (const auto& file : vecFiles) {
                QFile qFile(file);
                if (!qFile.open(QFile::ReadOnly)) {
                    throw std::runtime_error(strprintf("%s: Failed to open file: %s", __func__, file.toStdString()));
                }

                QString strStyle = QLatin1String(qFile.readAll());
                // Process all <os=...></os> groups in the stylesheet first
                QRegularExpressionMatch osStyleMatch;
                QRegularExpression osStyleExp(
                        "^"
                        "(<os=(?:'|\").+(?:'|\")>)" // group 1
                        "((?:.|\n)+?)"              // group 2
                        "(</os>?)"                  // group 3
                        "$");
                osStyleExp.setPatternOptions(QRegularExpression::MultilineOption);
                QRegularExpressionMatchIterator it = osStyleExp.globalMatch(strStyle);

                // For all <os=...></os> sections
                while (it.hasNext() && (osStyleMatch = it.next()).isValid()) {
                    QStringList listMatches = osStyleMatch.capturedTexts();

                    // Full match + 3 group matches
                    if (listMatches.size() % 4) {
                        throw std::runtime_error(strprintf("%s: Invalid <os=...></os> section in file %s", __func__, file.toStdString()));
                    }

                    for (int i = 0; i < listMatches.size(); i += 4) {
                        if (!listMatches[i + 1].contains(QString::fromStdString(platformName))) {
                            // If os is not supported for this styles
                            // just remove the full match
                            strStyle.replace(listMatches[i], "");
                        } else {
                            // If its supported remove the <os=...></os> tags
                            strStyle.replace(listMatches[i + 1], "");
                            strStyle.replace(listMatches[i + 3], "");
                        }
                    }
                }
                stylesheet->append(strStyle);
            }
            return true;
        };

        auto pathToFile = [&](const QString& theme) -> QString {
            return stylesheetDirectory + "/" + (isStyleSheetDirectoryCustom() ? mapThemeToStyle.at(theme) : theme);
        };

        std::vector<QString> vecFiles;
        // If light/dark theme is used load general styles first
        if (dashThemeActive()) {
            vecFiles.push_back(pathToFile(generalTheme));
        }
        vecFiles.push_back(pathToFile(getActiveTheme()));

        fStyleSheetChanged = loadFiles(vecFiles);
    }

    bool fUpdateStyleSheet = fForceUpdate || (fDebugCustomStyleSheets && fStyleSheetChanged);

    if (fUpdateStyleSheet && stylesheet != nullptr) {
        qApp->setStyleSheet(*stylesheet);
    }
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
    setApplicationFont();
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
    if (!mapWeights.count(fontFamily)) {
        return defaultFontWeightNormal;
    }
    return mapWeights[fontFamily].first;
}

void setFontWeightNormal(QFont::Weight weight)
{
    if (!mapWeights.count(fontFamily)) {
        throw std::runtime_error(strprintf("%s: Font family not loaded: %s", __func__, fontFamilyToString(fontFamily).toStdString()));
    }
    mapWeights[fontFamily].first = weight;
    updateFonts();
}

QFont::Weight getFontWeightBoldDefault()
{
    return defaultFontWeightBold;
}

QFont::Weight getFontWeightBold()
{
    if (!mapWeights.count(fontFamily)) {
        return defaultFontWeightBold;
    }
    return mapWeights[fontFamily].second;
}

void setFontWeightBold(QFont::Weight weight)
{
    if (!mapWeights.count(fontFamily)) {
        throw std::runtime_error(strprintf("%s: Font family not loaded: %s", __func__, fontFamilyToString(fontFamily).toStdString()));
    }
    mapWeights[fontFamily].second = weight;
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
    return std::round(nSize * (1 + (fontScale * fontScaleSteps)) * 4) / 4.0;
}

bool loadFonts()
{
    // Before any font changes store the applications default font to use it as SystemDefault.
    osDefaultFont = std::make_unique<QFont>(QApplication::font());

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
        osDefaultFont = nullptr;
        return false;
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

    // Initialize supported font weights for all available fonts
    // Generate a vector with supported font weights by comparing the width of a certain test text for all font weights
    auto supportedWeights = [](FontFamily family) -> std::vector<QFont::Weight> {
        auto getTestWidth = [&](QFont::Weight weight) -> int {
            QFont font = getFont(family, weight, false, defaultFontSize);
            return TextWidth(QFontMetrics(font), ("Check the width of this text to see if the weight change has an impact!"));
        };
        std::vector<QFont::Weight> vecWeights{QFont::Thin, QFont::ExtraLight, QFont::Light,
                                              QFont::Normal, QFont::Medium, QFont::DemiBold,
                                              QFont::Bold, QFont::ExtraBold, QFont::Black};
        std::vector<QFont::Weight> vecSupported;
        QFont::Weight prevWeight = vecWeights.front();
        for (auto weight = vecWeights.begin() + 1; weight != vecWeights.end(); ++weight) {
            if (getTestWidth(prevWeight) != getTestWidth(*weight)) {
                if (vecSupported.empty()) {
                    vecSupported.push_back(prevWeight);
                }
                vecSupported.push_back(*weight);
            }
            prevWeight = *weight;
        }
        if (vecSupported.empty()) {
            vecSupported.push_back(QFont::Normal);
        }
        return vecSupported;
    };

    mapSupportedWeights.insert(std::make_pair(FontFamily::SystemDefault, supportedWeights(FontFamily::SystemDefault)));
    mapSupportedWeights.insert(std::make_pair(FontFamily::Montserrat, supportedWeights(FontFamily::Montserrat)));

    auto getBestMatch = [&](FontFamily fontFamily, QFont::Weight targetWeight) {
        auto& vecSupported = mapSupportedWeights[fontFamily];
        auto it = vecSupported.begin();
        QFont::Weight bestWeight = *it;
        int nBestDiff = abs(*it - targetWeight);
        while (++it != vecSupported.end()) {
            int nDiff = abs(*it - targetWeight);
            if (nDiff < nBestDiff) {
                bestWeight = *it;
                nBestDiff = nDiff;
            }
        }
        return bestWeight;
    };

    auto addBestDefaults = [&](FontFamily family) -> auto {
        QFont::Weight normalWeight = getBestMatch(family, defaultFontWeightNormal);
        QFont::Weight boldWeight = getBestMatch(family, defaultFontWeightBold);
        if (normalWeight == boldWeight) {
            // If the results are the same use the next possible weight for bold font
            auto& vecSupported = mapSupportedWeights[fontFamily];
            auto it = std::find(vecSupported.begin(), vecSupported.end(),normalWeight);
            if (++it != vecSupported.end()) {
                boldWeight = *it;
            }
        }
        mapDefaultWeights.emplace(family, std::make_pair(normalWeight, boldWeight));
    };

    addBestDefaults(FontFamily::SystemDefault);
    addBestDefaults(FontFamily::Montserrat);

    // Load supported defaults. May become overwritten later.
    mapWeights = mapDefaultWeights;

    return true;
}

bool fontsLoaded()
{
    return osDefaultFont != nullptr;
}

void setApplicationFont()
{
    if (!fontsLoaded()) {
        return;
    }

    std::unique_ptr<QFont> font;

    if (fontFamily == FontFamily::Montserrat) {
        QString family = fontFamilyToString(FontFamily::Montserrat);
#ifdef Q_OS_MACOS
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
        font = std::make_unique<QFont>(*osDefaultFont);
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
    for (auto it : vecWidgets) {
        auto fontAttributes = std::make_tuple(weight, fItalic, nPointSize);
        auto itFontUpdate = mapFontUpdates.emplace(std::make_pair(it, fontAttributes));
        if (!itFontUpdate.second) {
            itFontUpdate.first->second = fontAttributes;
        }
    }
}

void updateFonts()
{
    // Fonts need to be loaded by GUIIUtil::loadFonts(), if not just return.
    if (!osDefaultFont) {
        return;
    }

    static std::map<QPointer<QWidget>, int> mapWidgetDefaultFontSizes;

    // QPointer becomes nullptr for objects that were deleted.
    // Remove them from mapDefaultFontSize and mapFontUpdates
    // before proceeding any further.
    size_t nRemovedDefaultFonts{0};
    auto itd = mapWidgetDefaultFontSizes.begin();
    while (itd != mapWidgetDefaultFontSizes.end()) {
        if (itd->first.isNull()) {
            itd = mapWidgetDefaultFontSizes.erase(itd);
            ++nRemovedDefaultFonts;
        } else {
            ++itd;
        }
    }

    size_t nRemovedFontUpdates{0};
    auto itn = mapFontUpdates.begin();
    while (itn != mapFontUpdates.end()) {
        if (itn->first.isNull()) {
            itn = mapFontUpdates.erase(itn);
            ++nRemovedFontUpdates;
        } else {
            ++itn;
        }
    }

    size_t nUpdatable{0}, nUpdated{0};
    std::map<QWidget*, QFont> mapWidgetFonts;
    // Loop through all widgets
    for (QWidget* w : qApp->allWidgets()) {
        std::vector<QString> vecIgnoreClasses{
            "QWidget", "QDialog", "QFrame", "QStackedWidget", "QDesktopWidget", "QDesktopScreenWidget",
            "QTipLabel", "QMessageBox", "QMenu", "QComboBoxPrivateScroller", "QComboBoxPrivateContainer",
            "QScrollBar", "QListView", "BitcoinGUI", "WalletView", "WalletFrame", "QVBoxLayout", "QGroupBox"
        };
        std::vector<QString> vecIgnoreObjects{
            "messagesWidget"
        };
        if (std::find(vecIgnoreClasses.begin(), vecIgnoreClasses.end(), w->metaObject()->className()) != vecIgnoreClasses.end() ||
            std::find(vecIgnoreObjects.begin(), vecIgnoreObjects.end(), w->objectName()) != vecIgnoreObjects.end()) {
            continue;
        }
        ++nUpdatable;

        QFont font = w->font();
        assert(font.pointSize() > 0);
        font.setFamily(qApp->font().family());
        font.setWeight(getFontWeightNormal());
        font.setStyleName(qApp->font().styleName());
        font.setStyle(qApp->font().style());

        // Insert/Get the default font size of the widget
        auto itDefault = mapWidgetDefaultFontSizes.emplace(w, font.pointSize());

        auto it = mapFontUpdates.find(w);
        if (it != mapFontUpdates.end()) {
            int nSize = std::get<2>(it->second);
            if (nSize == -1) {
                nSize = itDefault.first->second;
            }
            font = getFont(std::get<0>(it->second), std::get<1>(it->second), nSize);
        } else {
            font.setPointSizeF(getScaledFontSize(itDefault.first->second));
        }

        if (w->font() != font) {
            auto itWidgetFont = mapWidgetFonts.emplace(w, font);
            assert(itWidgetFont.second);
            ++nUpdated;
        }
    }
    qDebug().nospace() << __func__ << " - widget counts: updated/updatable/total(" << nUpdated << "/" << nUpdatable << "/" << qApp->allWidgets().size() << ")"
             << ", removed items: mapWidgetDefaultFontSizes/mapFontUpdates(" << nRemovedDefaultFonts << "/" << nRemovedFontUpdates << ")";

    // Perform the required font updates
    // NOTE: This is done as separate step to avoid scaling issues due to font inheritance
    //       hence all fonts are calculated and stored in mapWidgetFonts above.
    for (auto it : mapWidgetFonts) {
        it.first->setFont(it.second);
    }

    // Scale the global font size for the classes in the map below
    static std::map<std::string, int> mapClassFontUpdates{
        {"QTipLabel", -1}, {"QMenu", -1}, {"QMessageBox", -1}
    };
    for (auto& it : mapClassFontUpdates) {
        QFont fontClass = qApp->font(it.first.c_str());
        if (it.second == -1) {
            it.second = fontClass.pointSize();
        }
        double dSize = getScaledFontSize(it.second);
        if (fontClass.pointSizeF() != dSize) {
            fontClass.setPointSizeF(dSize);
            qApp->setFont(fontClass, it.first.c_str());
        }
    }
}

QFont getFont(FontFamily family, QFont::Weight qWeight, bool fItalic, int nPointSize)
{
    QFont font;
    if (!fontsLoaded()) {
        return font;
    }

    if (family == FontFamily::Montserrat) {
        static std::map<QFont::Weight, QString> mapMontserratMapping{
            {QFont::Thin, "Thin"},
            {QFont::ExtraLight, "ExtraLight"},
            {QFont::Light, "Light"},
            {QFont::Medium, "Medium"},
            {QFont::DemiBold, "SemiBold"},
            {QFont::ExtraBold, "ExtraBold"},
            {QFont::Black, "Black"},
#ifdef Q_OS_MACOS
            {QFont::Normal, "Regular"},
            {QFont::Bold, "Bold"},
#else
            {QFont::Normal, ""},
            {QFont::Bold, ""},
#endif
        };

        assert(mapMontserratMapping.count(qWeight));

#ifdef Q_OS_MACOS

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
        font.setFamily(osDefaultFont->family());
        font.setWeight(qWeight);
        font.setStyle(fItalic ? QFont::StyleItalic : QFont::StyleNormal);
    }

    if (nPointSize != -1) {
        font.setPointSizeF(getScaledFontSize(nPointSize));
    }

    if (gArgs.GetBoolArg("-debug-ui", false)) {
        qDebug() << __func__ << ": font size: " << font.pointSizeF() << " family: " << font.family() << ", style: " << font.styleName() << ", weight:" << font.weight() << " match: " << font.exactMatch();
    }

    return font;
}

QFont getFont(QFont::Weight qWeight, bool fItalic, int nPointSize)
{
    return getFont(fontFamily, qWeight, fItalic, nPointSize);
}
QFont getFont(FontWeight weight, bool fItalic, int nPointSize)
{
    return getFont(toQFontWeight(weight), fItalic, nPointSize);
}

QFont getFontNormal()
{
    return getFont(FontWeight::Normal);
}

QFont getFontBold()
{
    return getFont(FontWeight::Bold);
}

QFont::Weight getSupportedFontWeightNormalDefault()
{
    if (!mapDefaultWeights.count(fontFamily)) {
        throw std::runtime_error(strprintf("%s: Font family not loaded: %s", __func__, fontFamilyToString(fontFamily).toStdString()));
    }
    return mapDefaultWeights[fontFamily].first;
}

QFont::Weight getSupportedFontWeightBoldDefault()
{
    if (!mapDefaultWeights.count(fontFamily)) {
        throw std::runtime_error(strprintf("%s: Font family not loaded: %s", __func__, fontFamilyToString(fontFamily).toStdString()));
    }
    return mapDefaultWeights[fontFamily].second;
}

std::vector<QFont::Weight> getSupportedWeights()
{
    assert(mapSupportedWeights.count(fontFamily));
    return mapSupportedWeights[fontFamily];
}

QFont::Weight supportedWeightFromIndex(int nIndex)
{
    auto vecWeights = getSupportedWeights();
    assert(vecWeights.size() > uint64_t(nIndex));
    return vecWeights[nIndex];
}

int supportedWeightToIndex(QFont::Weight weight)
{
    auto vecWeights = getSupportedWeights();
    for (uint64_t index = 0; index < vecWeights.size(); ++index) {
        if (weight == vecWeights[index]) {
            return index;
        }
    }
    return -1;
}

bool isSupportedWeight(const QFont::Weight weight)
{
    return supportedWeightToIndex(weight) != -1;
}

QString getActiveTheme()
{
    QSettings settings;
    QString theme = settings.value("theme", defaultTheme).toString();
    if (!isValidTheme(theme)) {
        return defaultTheme;
    }
    return theme;
}

bool dashThemeActive()
{
    QSettings settings;
    QString theme = settings.value("theme", defaultTheme).toString();
    return theme != traditionalTheme;
}

void loadTheme(bool fForce)
{
    loadStyleSheet(fForce);
    updateFonts();
    updateMacFocusRects();
}

void disableMacFocusRect(const QWidget* w)
{
#ifdef Q_OS_MACOS
    for (const auto& c : w->findChildren<QWidget*>()) {
        if (c->testAttribute(Qt::WA_MacShowFocusRect)) {
            c->setAttribute(Qt::WA_MacShowFocusRect, !dashThemeActive());
            setRectsDisabled.emplace(c);
        }
    }
#endif
}

void updateMacFocusRects()
{
#ifdef Q_OS_MACOS
    QWidgetList allWidgets = QApplication::allWidgets();
    auto it = setRectsDisabled.begin();
    while (it != setRectsDisabled.end()) {
        if (allWidgets.contains(*it)) {
            (*it)->setAttribute(Qt::WA_MacShowFocusRect, !dashThemeActive());
            ++it;
        } else {
            it = setRectsDisabled.erase(it);
        }
    }
#endif
}

void updateButtonGroupShortcuts(QButtonGroup* buttonGroup)
{
    if (buttonGroup == nullptr) {
        return;
    }
#ifdef Q_OS_MACOS
    auto modifier = "Ctrl";
#else
    auto modifier = "Alt";
#endif
    int nKey = 1;
    for (auto button : buttonGroup->buttons()) {
        if (button->isVisible()) {
            button->setShortcut(QKeySequence(QString("%1+%2").arg(modifier).arg(nKey++)));
        } else {
            button->setShortcut(QKeySequence());
        }
    }
}

void setClipboard(const QString& str)
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Clipboard);
    if (clipboard->supportsSelection()) {
        clipboard->setText(str, QClipboard::Selection);
    }
}

fs::path QStringToPath(const QString &path)
{
    return fs::u8path(path.toStdString());
}

QString PathToQString(const fs::path &path)
{
    return QString::fromStdString(path.utf8string());
}

QString NetworkToQString(Network net)
{
    switch (net) {
    case NET_UNROUTABLE: return QObject::tr("Unroutable");
    case NET_IPV4: return "IPv4";
    case NET_IPV6: return "IPv6";
    case NET_ONION: return "Onion";
    case NET_I2P: return "I2P";
    case NET_CJDNS: return "CJDNS";
    case NET_INTERNAL: return QObject::tr("Internal");
    case NET_MAX: assert(false);
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

QString ConnectionTypeToQString(ConnectionType conn_type, bool prepend_direction)
{
    QString prefix;
    if (prepend_direction) {
        prefix = (conn_type == ConnectionType::INBOUND) ?
                     /*: An inbound connection from a peer. An inbound connection
                         is a connection initiated by a peer. */
                     QObject::tr("Inbound") :
                     /*: An outbound connection to a peer. An outbound connection
                         is a connection initiated by us. */
                     QObject::tr("Outbound") + " ";
    }
    switch (conn_type) {
    case ConnectionType::INBOUND: return prefix;
    //: Peer connection type that relays all network information.
    case ConnectionType::OUTBOUND_FULL_RELAY: return prefix + QObject::tr("Full Relay");
    /*: Peer connection type that relays network information about
        blocks and not transactions or addresses. */
    case ConnectionType::BLOCK_RELAY: return prefix + QObject::tr("Block Relay");
    //: Peer connection type established manually through one of several methods.
    case ConnectionType::MANUAL: return prefix + QObject::tr("Manual");
    //: Short-lived peer connection type that tests the aliveness of known addresses.
    case ConnectionType::FEELER: return prefix + QObject::tr("Feeler");
    //: Short-lived peer connection type that solicits known addresses from a peer.
    case ConnectionType::ADDR_FETCH: return prefix + QObject::tr("Address Fetch");
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

QString formatDurationStr(std::chrono::seconds dur)
{
    const auto d{std::chrono::duration_cast<std::chrono::days>(dur)};
    const auto h{std::chrono::duration_cast<std::chrono::hours>(dur - d)};
    const auto m{std::chrono::duration_cast<std::chrono::minutes>(dur - d - h)};
    const auto s{std::chrono::duration_cast<std::chrono::seconds>(dur - d - h - m)};
    QStringList str_list;
    if (auto d2{d.count()}) str_list.append(QObject::tr("%1 d").arg(d2));
    if (auto h2{h.count()}) str_list.append(QObject::tr("%1 h").arg(h2));
    if (auto m2{m.count()}) str_list.append(QObject::tr("%1 m").arg(m2));
    const auto s2{s.count()};
    if (s2 || str_list.empty()) str_list.append(QObject::tr("%1 s").arg(s2));
    return str_list.join(" ");
}

QString FormatPeerAge(std::chrono::seconds time_connected)
{
    const auto time_now{GetTime<std::chrono::seconds>()};
    const auto age{time_now - time_connected};
    if (age >= 24h) return QObject::tr("%1 d").arg(age / 24h);
    if (age >= 1h) return QObject::tr("%1 h").arg(age / 1h);
    if (age >= 1min) return QObject::tr("%1 m").arg(age / 1min);
    return QObject::tr("%1 s").arg(age / 1s);
}

QString formatServicesStr(quint64 mask)
{
    QStringList strList;

    for (const auto& flag : serviceFlagsToStr(mask)) {
        strList.append(QString::fromStdString(flag));
    }

    if (strList.size())
        return strList.join(", ");
    else
        return QObject::tr("None");
}

QString formatPingTime(std::chrono::microseconds ping_time)
{
    return (ping_time == std::chrono::microseconds::max() || ping_time == 0us) ?
        QObject::tr("N/A") :
        QObject::tr("%1 ms").arg(QString::number((int)(count_microseconds(ping_time) / 1000), 10));
}

QString formatTimeOffset(int64_t nTimeOffset)
{
  return QObject::tr("%1 s").arg(QString::number((int)nTimeOffset, 10));
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
    if (bytes < 1'000)
        return QObject::tr("%1 B").arg(bytes);
    if (bytes < 1'000'000)
        return QObject::tr("%1 kB").arg(bytes / 1'000);
    if (bytes < 1'000'000'000)
        return QObject::tr("%1 MB").arg(bytes / 1'000'000);

    return QObject::tr("%1 GB").arg(bytes / 1'000'000'000);
}

qreal calculateIdealFontSize(int width, const QString& text, QFont font, qreal minPointSize, qreal font_size) {
    while(font_size >= minPointSize) {
        font.setPointSizeF(font_size);
        QFontMetrics fm(font);
        if (TextWidth(fm, text) < width) {
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

bool ItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
            Q_EMIT keyEscapePressed();
        }
    }
    return QItemDelegate::eventFilter(object, event);
}

void PolishProgressDialog(QProgressDialog* dialog)
{
#ifdef Q_OS_MACOS
    // Workaround for macOS-only Qt bug; see: QTBUG-65750, QTBUG-70357.
    const int margin = TextWidth(dialog->fontMetrics(), ("X"));
    dialog->resize(dialog->width() + 2 * margin, dialog->height());
#endif
    // QProgressDialog estimates the time the operation will take (based on time
    // for steps), and only shows itself if that estimate is beyond minimumDuration.
    // The default minimumDuration value is 4 seconds, and it could make users
    // think that the GUI is frozen.
    dialog->setMinimumDuration(0);
}

int TextWidth(const QFontMetrics& fm, const QString& text)
{
    return fm.horizontalAdvance(text);
}

void LogQtInfo()
{
#ifdef QT_STATIC
    const std::string qt_link{"static"};
#else
    const std::string qt_link{"dynamic"};
#endif
    // TODO replace instances of LogPrintf with LogInfo once 28318 is merged
    LogPrintf("Qt %s (%s), plugin=%s\n", qVersion(), qt_link, QGuiApplication::platformName().toStdString());
    const auto static_plugins = QPluginLoader::staticPlugins();
    if (static_plugins.empty()) {
        LogPrintf("No static plugins.\n");
    } else {
        LogPrintf("Static plugins:\n");
        for (const QStaticPlugin& p : static_plugins) {
            QJsonObject meta_data = p.metaData();
            const std::string plugin_class = meta_data.take(QString("className")).toString().toStdString();
            const int plugin_version = meta_data.take(QString("version")).toInt();
            LogPrintf(" %s, version %d\n", plugin_class, plugin_version);
        }
    }

    LogPrintf("Style: %s / %s\n", QApplication::style()->objectName().toStdString(), QApplication::style()->metaObject()->className());
    LogPrintf("System: %s, %s\n", QSysInfo::prettyProductName().toStdString(), QSysInfo::buildAbi().toStdString());
    for (const QScreen* s : QGuiApplication::screens()) {
        LogPrintf("Screen: %s %dx%d, pixel ratio=%.1f\n", s->name().toStdString(), s->size().width(), s->size().height(), s->devicePixelRatio());
    }
}

void PopupMenu(QMenu* menu, const QPoint& point, QAction* at_action)
{
    // The qminimal plugin does not provide window system integration.
    if (QApplication::platformName() == "minimal") return;
    menu->popup(point, at_action);
}

QDateTime StartOfDay(const QDate& date)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return date.startOfDay();
#else
    return QDateTime(date);
#endif
}

bool HasPixmap(const QLabel* label)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    return !label->pixmap(Qt::ReturnByValue).isNull();
#else
    return label->pixmap() != nullptr;
#endif
}

QImage GetImage(const QLabel* label)
{
    if (!HasPixmap(label)) {
        return QImage();
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    return label->pixmap(Qt::ReturnByValue).toImage();
#else
    return label->pixmap()->toImage();
#endif
}

QString MakeHtmlLink(const QString& source, const QString& link)
{
    return QString(source).replace(
        link,
        QLatin1String("<a href=\"") + link + QLatin1String("\">") + link + QLatin1String("</a>"));
}

void PrintSlotException(
    const std::exception* exception,
    const QObject* sender,
    const QObject* receiver)
{
    std::string description = sender->metaObject()->className();
    description += "->";
    description += receiver->metaObject()->className();
    PrintExceptionContinue(std::make_exception_ptr(exception), description.c_str());
}

void ShowModalDialogAsynchronously(QDialog* dialog)
{
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->show();
}

} // namespace GUIUtil
