// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2025 The Dash Core developers
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
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QJsonObject>
#include <QKeySequence>
#include <QLatin1String>
#include <QList>
#include <QLocale>
#include <QMenu>
#include <QMouseEvent>
#include <QPluginLoader>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QSize>
#include <QStandardPaths>
#include <QString>
#include <QTextDocument> // for Qt::mightBeRichText
#include <QThread>
#include <QUrlQuery>
#include <QVBoxLayout>

#include <chrono>
#include <exception>
#include <algorithm>
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
    { ThemedStyle::TS_INVALID, "border: 3px solid #a84832;" },
    { ThemedStyle::TS_ERROR, "color:#a84832;" },
    { ThemedStyle::TS_WARNING, "color:#999900;" },
    { ThemedStyle::TS_SUCCESS, "color:#5e8c41;" },
    { ThemedStyle::TS_COMMAND, "color:#008de4;" },
    { ThemedStyle::TS_PRIMARY, "color:#333;" },
    { ThemedStyle::TS_SECONDARY, "color:#444;" },
};

static const std::map<ThemedStyle, QString> themedDarkStyles = {
    { ThemedStyle::TS_INVALID, "border: 3px solid #a84832;" },
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
        setFont({&lblHeading}, {GUIUtil::g_font_registry.GetWeightBold(), 16});
        setFont({&lblSubHeading}, {GUIUtil::g_font_registry.GetWeightNormal(), 14, true});
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
    if (w) {
        if (QGuiApplication::platformName() == "wayland") {
            auto flags = w->windowFlags();
            w->setWindowFlags(flags|Qt::WindowStaysOnTopHint);
            w->show();
            w->setWindowFlags(flags);
            w->show();
        } else {
#ifdef Q_OS_MACOS
            ForceActivation();
#endif
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

std::vector<QString> listStyleSheets()
{
    std::vector<QString> vecStylesheets;
    for (const auto& it : mapThemeToStyle) {
        vecStylesheets.push_back(it.second);
    }
    return vecStylesheets;
}

std::vector<QString> listThemes()
{
    std::vector<QString> vecThemes;
    for (const auto& it : mapThemeToStyle) {
        if (it.first == generalTheme) continue;
        vecThemes.push_back(it.first);
    }
    return vecThemes;
}

QString getDefaultTheme()
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
                QRegularExpression osStyleExp(
                        "^"
                        "(<os=(?:'|\").+(?:'|\")>)" // group 1
                        "((?:.|\n)+?)"              // group 2
                        "(</os>?)"                  // group 3
                        "$");
                osStyleExp.setPatternOptions(QRegularExpression::MultilineOption);

                // Collect matches first to avoid modifying the string while iterating
                QList<QRegularExpressionMatch> matches;
                {
                    QRegularExpressionMatchIterator it = osStyleExp.globalMatch(strStyle);
                    while (it.hasNext()) {
                        QRegularExpressionMatch m = it.next();
                        if (m.hasMatch()) {
                            matches.append(m);
                        }
                    }
                }

                // Build replacement operations using absolute positions
                struct Replacement { int start; int end; QString replacement; };
                QVector<Replacement> replacements;
                for (const auto& m : matches) {
                    const QString openTag = m.captured(1);
                    const QString inner = m.captured(2);
                    Q_UNUSED(inner);
                    // Remove entire block if OS doesn't match, otherwise drop only the tags
                    if (!openTag.contains(QString::fromStdString(platformName))) {
                        replacements.push_back({m.capturedStart(0), m.capturedEnd(0), QString()});
                    } else {
                        // Remove opening and closing tags, keep inner content
                        replacements.push_back({m.capturedStart(1), m.capturedEnd(1), QString()});
                        replacements.push_back({m.capturedStart(3), m.capturedEnd(3), QString()});
                    }
                }

                // Apply replacements from end to start so offsets stay valid
                std::sort(replacements.begin(), replacements.end(), [](const Replacement& a, const Replacement& b) {
                    return a.start > b.start;
                });
                for (const auto& r : replacements) {
                    strStyle.replace(r.start, r.end - r.start, r.replacement);
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
