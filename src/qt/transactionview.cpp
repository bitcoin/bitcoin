// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionview.h>

#include <qt/addresstablemodel.h>
#include <qt/bitcoinunits.h>
#include <qt/csvmodelwriter.h>
#include <qt/editaddressdialog.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/qrdialog.h>
#include <qt/transactiondescdialog.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactionrecord.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>

#include <interfaces/node.h>
#include <node/interface_ui.h>

#include <chrono>
#include <optional>

#include <QCalendarWidget>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QSettings>
#include <QTableView>
#include <QTextCharFormat>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

/** Date format for persistence */
static const char* PERSISTENCE_DATE_FORMAT = "yyyy-MM-dd";

TransactionView::TransactionView(QWidget* parent) :
    QWidget(parent)
{
    QSettings settings;
    // Build filter row
    setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);
    hlayout->setSpacing(1);
    hlayout->addSpacing(STATUS_COLUMN_WIDTH - 2);

    watchOnlyWidget = new QComboBox(this);
    watchOnlyWidget->setFixedWidth(24);
    watchOnlyWidget->addItem("", TransactionFilterProxy::WatchOnlyFilter_All);
    watchOnlyWidget->addItem(GUIUtil::getIcon("eye_plus"), "", TransactionFilterProxy::WatchOnlyFilter_Yes);
    watchOnlyWidget->addItem(GUIUtil::getIcon("eye_minus"), "", TransactionFilterProxy::WatchOnlyFilter_No);
    hlayout->addWidget(watchOnlyWidget);

    dateWidget = new QComboBox(this);
    dateWidget->setFixedWidth(120);
    dateWidget->addItem(tr("All"), All);
    dateWidget->addItem(tr("Today"), Today);
    dateWidget->addItem(tr("This week"), ThisWeek);
    dateWidget->addItem(tr("This month"), ThisMonth);
    dateWidget->addItem(tr("Last month"), LastMonth);
    dateWidget->addItem(tr("This year"), ThisYear);
    dateWidget->addItem(tr("Range…"), Range);
    dateWidget->setCurrentIndex(settings.value("transactionDate").toInt());
    hlayout->addWidget(dateWidget);

    QString strCoinJoinName = QString::fromStdString(gCoinJoinName);
    typeWidget = new QComboBox(this);
    typeWidget->setFixedWidth(TYPE_COLUMN_WIDTH - 1);
    typeWidget->addItem(tr("All"), TransactionFilterProxy::ALL_TYPES);
    typeWidget->addItem(tr("Most Common"), TransactionFilterProxy::COMMON_TYPES);
    typeWidget->addItem(tr("Received with"), TransactionFilterProxy::TYPE(TransactionRecord::RecvWithAddress) |
                                        TransactionFilterProxy::TYPE(TransactionRecord::RecvFromOther));
    typeWidget->addItem(tr("Sent to"), TransactionFilterProxy::TYPE(TransactionRecord::SendToAddress) |
                                  TransactionFilterProxy::TYPE(TransactionRecord::SendToOther));
    typeWidget->addItem(tr("%1 Send").arg(strCoinJoinName), TransactionFilterProxy::TYPE(TransactionRecord::CoinJoinSend));
    typeWidget->addItem(tr("%1 Make Collateral Inputs").arg(strCoinJoinName), TransactionFilterProxy::TYPE(TransactionRecord::CoinJoinMakeCollaterals));
    typeWidget->addItem(tr("%1 Create Denominations").arg(strCoinJoinName), TransactionFilterProxy::TYPE(TransactionRecord::CoinJoinCreateDenominations));
    typeWidget->addItem(tr("%1 Mixing").arg(strCoinJoinName), TransactionFilterProxy::TYPE(TransactionRecord::CoinJoinMixing));
    typeWidget->addItem(tr("%1 Collateral Payment").arg(strCoinJoinName), TransactionFilterProxy::TYPE(TransactionRecord::CoinJoinCollateralPayment));
    typeWidget->addItem(tr("To yourself"), TransactionFilterProxy::TYPE(TransactionRecord::SendToSelf));
    typeWidget->addItem(tr("Mined"), TransactionFilterProxy::TYPE(TransactionRecord::Generated));
    typeWidget->addItem(tr("Platform Transfer"), TransactionFilterProxy::TYPE(TransactionRecord::PlatformTransfer));
    typeWidget->addItem(tr("Other"), TransactionFilterProxy::TYPE(TransactionRecord::Other));
    typeWidget->setCurrentIndex(settings.value("transactionType").toInt());

    hlayout->addWidget(typeWidget);

    search_widget = new QLineEdit(this);
    search_widget->setPlaceholderText(tr("Enter address, transaction id, or label to search"));
    search_widget->setObjectName("search_widget");
    hlayout->addWidget(search_widget);

    amountWidget = new QLineEdit(this);
    amountWidget->setPlaceholderText(tr("Min amount"));
    amountWidget->setFixedWidth(125);
    QDoubleValidator *amountValidator = new QDoubleValidator(0, 1e20, 8, this);
    QLocale amountLocale(QLocale::C);
    amountLocale.setNumberOptions(QLocale::RejectGroupSeparator);
    amountValidator->setLocale(amountLocale);
    amountWidget->setValidator(amountValidator);
    amountWidget->setObjectName("amountWidget");
    hlayout->addWidget(amountWidget);

    // Delay before filtering transactions
    static constexpr auto input_filter_delay{200ms};

    QTimer* amount_typing_delay = new QTimer(this);
    amount_typing_delay->setSingleShot(true);
    amount_typing_delay->setInterval(input_filter_delay);

    QTimer* prefix_typing_delay = new QTimer(this);
    prefix_typing_delay->setSingleShot(true);
    prefix_typing_delay->setInterval(input_filter_delay);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    transactionView = new QTableView(this);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(createDateRangeWidget());
    vlayout->addWidget(transactionView);
    vlayout->setSpacing(0);
#ifndef Q_OS_MACOS
    int width = transactionView->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
    hlayout->addSpacing(width);
    // Always show scroll bar
    transactionView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
#endif
    transactionView->setTabKeyNavigation(false);
    transactionView->setContextMenuPolicy(Qt::CustomContextMenu);

    transactionView->installEventFilter(this);

    transactionView->setObjectName("transactionView");

    contextMenu = new QMenu(this);
    contextMenu->setObjectName("contextMenu");
    copyAddressAction = contextMenu->addAction(tr("&Copy address"), this, &TransactionView::copyAddress);
    copyLabelAction = contextMenu->addAction(tr("Copy &label"), this, &TransactionView::copyLabel);
    contextMenu->addAction(tr("Copy &amount"), this, &TransactionView::copyAmount);
    contextMenu->addAction(tr("Copy transaction &ID"), this, &TransactionView::copyTxID);
    contextMenu->addAction(tr("Copy &raw transaction"), this, &TransactionView::copyTxHex);
    contextMenu->addAction(tr("Copy full transaction &details"), this, &TransactionView::copyTxPlainText);
    contextMenu->addAction(tr("&Show transaction details"), this, &TransactionView::showDetails);
    contextMenu->addSeparator();
    abandonAction = contextMenu->addAction(tr("A&bandon transaction"), this, &TransactionView::abandonTx);
    resendAction = contextMenu->addAction(tr("Rese&nd transaction"), this, &TransactionView::resendTx);
    contextMenu->addAction(tr("&Edit address label"), this, &TransactionView::editLabel);
    [[maybe_unused]] QAction* showAddressQRCodeAction = contextMenu->addAction(tr("Show address &QR code"), this, &TransactionView::showAddressQRCode);
#ifndef USE_QRCODE
    showAddressQRCodeAction->setEnabled(false);
#endif

    connect(dateWidget, qOverload<int>(&QComboBox::activated), this, &TransactionView::chooseDate);
    connect(typeWidget, qOverload<int>(&QComboBox::activated), this, &TransactionView::chooseType);
    connect(watchOnlyWidget, qOverload<int>(&QComboBox::activated), this, &TransactionView::chooseWatchonly);
    connect(amountWidget, &QLineEdit::textChanged, amount_typing_delay, qOverload<>(&QTimer::start));
    connect(amount_typing_delay, &QTimer::timeout, this, &TransactionView::changedAmount);
    connect(search_widget, &QLineEdit::textChanged, prefix_typing_delay, qOverload<>(&QTimer::start));
    connect(prefix_typing_delay, &QTimer::timeout, this, &TransactionView::changedSearch);

    connect(transactionView, &QTableView::doubleClicked, this, &TransactionView::doubleClicked);
    connect(transactionView, &QTableView::clicked, this, &TransactionView::computeSum);
    connect(transactionView, &QTableView::customContextMenuRequested, this, &TransactionView::contextualMenu);

    // Double-clicking on a transaction on the transaction history page shows details
    connect(this, &TransactionView::doubleClicked, this, &TransactionView::showDetails);
}

void TransactionView::setModel(WalletModel *_model)
{
    QSettings settings;
    this->model = _model;
    if(_model)
    {
        transactionProxyModel = new TransactionFilterProxy(this);
        transactionProxyModel->setSourceModel(_model->getTransactionTableModel());
        transactionProxyModel->setDynamicSortFilter(true);
        transactionProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        transactionProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        transactionProxyModel->setSortRole(Qt::EditRole);

        transactionView->setModel(transactionProxyModel);
        transactionView->setAlternatingRowColors(true);
        transactionView->setSelectionBehavior(QAbstractItemView::SelectRows);
        transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        transactionView->horizontalHeader()->setSortIndicator(TransactionTableModel::Date, Qt::DescendingOrder);
        transactionView->setSortingEnabled(true);
        transactionView->verticalHeader()->hide();

        transactionView->setColumnWidth(TransactionTableModel::Status, STATUS_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Watchonly, WATCHONLY_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Date, DATE_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Type, TYPE_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Amount, AMOUNT_MINIMUM_COLUMN_WIDTH);

        // Note: it's a good idea to connect this signal AFTER the model is set
        connect(transactionView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TransactionView::computeSum);

        transactionView->horizontalHeader()->setMinimumSectionSize(MINIMUM_COLUMN_WIDTH);
        transactionView->horizontalHeader()->setSectionResizeMode(TransactionTableModel::Status, QHeaderView::Fixed);
        transactionView->horizontalHeader()->setSectionResizeMode(TransactionTableModel::Watchonly, QHeaderView::Fixed);
        transactionView->horizontalHeader()->setSectionResizeMode(TransactionTableModel::Date, QHeaderView::Interactive);
        transactionView->horizontalHeader()->setSectionResizeMode(TransactionTableModel::Type, QHeaderView::Interactive);
        transactionView->horizontalHeader()->setSectionResizeMode(TransactionTableModel::ToAddress, QHeaderView::Stretch);
        transactionView->horizontalHeader()->setSectionResizeMode(TransactionTableModel::Amount, QHeaderView::Fixed);

        if (_model->getOptionsModel())
        {
            // Add third party transaction URLs to context menu
            QStringList listUrls = GUIUtil::SplitSkipEmptyParts(_model->getOptionsModel()->getThirdPartyTxUrls(), "|");
            bool actions_created = false;
            for (int i = 0; i < listUrls.size(); ++i)
            {
                QString url = listUrls[i].trimmed();
                QString host = QUrl(url, QUrl::StrictMode).host();
                if (!host.isEmpty())
                {
                    if (!actions_created) {
                        contextMenu->addSeparator();
                        actions_created = true;
                    }
                    /*: Transactions table context menu action to show the
                        selected transaction in a third-party block explorer.
                        %1 is a stand-in argument for the URL of the explorer. */
                    contextMenu->addAction(tr("Show in %1").arg(host), [this, url] { openThirdPartyTxUrl(url); });
                }
            }

            connect(_model->getOptionsModel(), &OptionsModel::coinJoinEnabledChanged, this, &TransactionView::updateCoinJoinVisibility);
        }

        // show/hide column Watch-only
        updateWatchOnlyColumn(_model->wallet().haveWatchOnly());

        // Watch-only signal
        connect(_model, &WalletModel::notifyWatchonlyChanged, this, &TransactionView::updateWatchOnlyColumn);

        // Update transaction list with persisted settings
        chooseType(settings.value("transactionType").toInt());
        chooseDate(settings.value("transactionDate").toInt());

        updateCoinJoinVisibility();
    }
}

void TransactionView::chooseDate(int idx)
{
    if (!transactionProxyModel) return;

    QSettings settings;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(dateWidget->itemData(idx).toInt())
    {
    case All:
        transactionProxyModel->setDateRange(
                std::nullopt,
                std::nullopt);
        break;
    case Today:
        transactionProxyModel->setDateRange(
                GUIUtil::StartOfDay(current),
                std::nullopt);
        break;
    case ThisWeek: {
        // Find last Monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        transactionProxyModel->setDateRange(
                GUIUtil::StartOfDay(startOfWeek),
                std::nullopt);

        } break;
    case ThisMonth:
        transactionProxyModel->setDateRange(
                GUIUtil::StartOfDay(QDate(current.year(), current.month(), 1)),
                std::nullopt);
        break;
    case LastMonth:
        transactionProxyModel->setDateRange(
                GUIUtil::StartOfDay(QDate(current.year(), current.month(), 1).addMonths(-1)),
                GUIUtil::StartOfDay(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        transactionProxyModel->setDateRange(
                GUIUtil::StartOfDay(QDate(current.year(), 1, 1)),
                std::nullopt);
        break;
    case Range:
        dateRangeWidget->setVisible(true);
        dateRangeChanged();
        break;
    }
    // Persist new date settings
    settings.setValue("transactionDate", idx);
    if (dateWidget->itemData(idx).toInt() == Range){
        settings.setValue("transactionDateFrom", dateFrom->date().toString(PERSISTENCE_DATE_FORMAT));
        settings.setValue("transactionDateTo", dateTo->date().toString(PERSISTENCE_DATE_FORMAT));
    }
}

void TransactionView::chooseType(int idx)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setTypeFilter(
        typeWidget->itemData(idx).toUInt());
    // Persist settings
    QSettings settings;
    settings.setValue("transactionType", idx);
}

void TransactionView::chooseWatchonly(int idx)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setWatchOnlyFilter(
        static_cast<TransactionFilterProxy::WatchOnlyFilter>(watchOnlyWidget->itemData(idx).toInt()));
}

void TransactionView::changedSearch()
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setSearchString(search_widget->text());
}

void TransactionView::changedAmount()
{
    if(!transactionProxyModel)
        return;
    CAmount amount_parsed = 0;

    // Replace "," by "." so BitcoinUnits::parse will not fail for users entering "," as decimal separator
    QString newAmount = amountWidget->text();
    newAmount.replace(QString(","), QString("."));

    if (BitcoinUnits::parse(model->getOptionsModel()->getDisplayUnit(), newAmount, &amount_parsed)) {
        transactionProxyModel->setMinAmount(amount_parsed);
    }
    else
    {
        transactionProxyModel->setMinAmount(0);
    }
}

void TransactionView::exportClicked()
{
    if (!model || !model->getOptionsModel()) {
        return;
    }

    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Export Transaction History"), QString(),
        /*: Expanded name of the CSV file format.
            See: https://en.wikipedia.org/wiki/Comma-separated_values. */
        tr("Comma separated file") + QLatin1String(" (*.csv)"), nullptr);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(transactionProxyModel);
    writer.addColumn(tr("Confirmed"), 0, TransactionTableModel::ConfirmedRole);
    if (model->wallet().haveWatchOnly())
        writer.addColumn(tr("Watch-only"), TransactionTableModel::Watchonly);
    writer.addColumn(tr("Date"), 0, TransactionTableModel::DateRole);
    writer.addColumn(tr("Type"), TransactionTableModel::Type, Qt::EditRole);
    writer.addColumn(tr("Label"), 0, TransactionTableModel::LabelRole);
    writer.addColumn(tr("Address"), 0, TransactionTableModel::AddressRole);
    writer.addColumn(BitcoinUnits::getAmountColumnTitle(model->getOptionsModel()->getDisplayUnit()), 0, TransactionTableModel::FormattedAmountRole);
    writer.addColumn(tr("ID"), 0, TransactionTableModel::TxHashRole);

    if(!writer.write()) {
        Q_EMIT message(tr("Exporting Failed"), tr("There was an error trying to save the transaction history to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
    }
    else {
        Q_EMIT message(tr("Exporting Successful"), tr("The transaction history was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void TransactionView::contextualMenu(const QPoint &point)
{
    if (!transactionView || !transactionView->selectionModel())
        return;
    QModelIndex index = transactionView->indexAt(point);
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    // check if transaction can be abandoned, disable context menu action in case it doesn't
    uint256 hash;
    hash.SetHex(selection.at(0).data(TransactionTableModel::TxHashRole).toString().toStdString());
    abandonAction->setEnabled(model->wallet().transactionCanBeAbandoned(hash));
    resendAction->setEnabled(selection.size() == 1 && model->wallet().transactionCanBeResent(hash));
    copyAddressAction->setEnabled(GUIUtil::hasEntryData(transactionView, 0, TransactionTableModel::AddressRole));
    copyLabelAction->setEnabled(GUIUtil::hasEntryData(transactionView, 0, TransactionTableModel::LabelRole));

    if (index.isValid()) {
        GUIUtil::PopupMenu(contextMenu, transactionView->viewport()->mapToGlobal(point));
    }
}

void TransactionView::abandonTx()
{
    if(!transactionView || !transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);

    // get the hash from the TxHashRole (QVariant / QString)
    uint256 hash;
    QString hashQStr = selection.at(0).data(TransactionTableModel::TxHashRole).toString();
    hash.SetHex(hashQStr.toStdString());

    // Abandon the wallet transaction over the walletModel
    model->wallet().abandonTransaction(hash);
}

void TransactionView::resendTx()
{
    if(!transactionView || !transactionView->selectionModel()) {
        return;
    }
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);

    // get the hash from the TxHashRole (QVariant / QString)
    uint256 hash;
    QString hashQStr = selection.at(0).data(TransactionTableModel::TxHashRole).toString();
    hash.SetHex(hashQStr.toStdString());

    // Abandon the wallet transaction over the walletModel
    model->wallet().resendTransaction(hash);
}

void TransactionView::copyAddress()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::AddressRole);
}

void TransactionView::copyLabel()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::LabelRole);
}

void TransactionView::copyAmount()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::FormattedAmountRole);
}

void TransactionView::copyTxID()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxHashRole);
}

void TransactionView::copyTxHex()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxHexRole);
}

void TransactionView::copyTxPlainText()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxPlainTextRole);
}

void TransactionView::editLabel()
{
    if(!transactionView->selectionModel() ||!model)
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        AddressTableModel *addressBook = model->getAddressTableModel();
        if(!addressBook)
            return;
        QString address = selection.at(0).data(TransactionTableModel::AddressRole).toString();
        if(address.isEmpty())
        {
            // If this transaction has no associated address, exit
            return;
        }
        // Is address in address book? Address book can miss address when a transaction is
        // sent from outside the UI.
        int idx = addressBook->lookupAddress(address);
        if(idx != -1)
        {
            // Edit sending / receiving address
            QModelIndex modelIdx = addressBook->index(idx, 0, QModelIndex());
            // Determine type of address, launch appropriate editor dialog type
            QString type = modelIdx.data(AddressTableModel::TypeRole).toString();

            auto dlg = new EditAddressDialog(
                type == AddressTableModel::Receive
                ? EditAddressDialog::EditReceivingAddress
                : EditAddressDialog::EditSendingAddress, this);
            dlg->setModel(addressBook);
            dlg->loadRow(idx);
            GUIUtil::ShowModalDialogAsynchronously(dlg);
        }
        else
        {
            // Add sending address
            auto dlg = new EditAddressDialog(EditAddressDialog::NewSendingAddress,
                this);
            dlg->setModel(addressBook);
            dlg->setAddress(address);
            GUIUtil::ShowModalDialogAsynchronously(dlg);
        }
    }
}

void TransactionView::showDetails()
{
    if(!transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog* dlg = new TransactionDescDialog(selection.at(0), this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }
}

void TransactionView::showAddressQRCode()
{
    QList<QModelIndex> entries = GUIUtil::getEntryData(transactionView, 0);
    if (entries.empty()) {
        return;
    }

    QString strAddress = entries.at(0).data(TransactionTableModel::AddressRole).toString();
    QRDialog* dialog = new QRDialog(this);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setInfo(tr("QR code"), "dash:"+strAddress, "", strAddress);
    dialog->show();
}

/** Compute sum of all selected transactions */
void TransactionView::computeSum()
{
    qint64 amount = 0;
    BitcoinUnit nDisplayUnit = model->getOptionsModel()->getDisplayUnit();
    if(!transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();

    for (QModelIndex index : selection){
        amount += index.data(TransactionTableModel::AmountRole).toLongLong();
    }
    QString strAmount(BitcoinUnits::formatWithUnit(nDisplayUnit, amount, true, BitcoinUnits::SeparatorStyle::ALWAYS));
    if (amount < 0) strAmount = "<span style='" + GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR) + "'>" + strAmount + "</span>";
    Q_EMIT trxAmount(strAmount);
}

void TransactionView::openThirdPartyTxUrl(QString url)
{
    if(!transactionView || !transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);
    if(!selection.isEmpty())
         QDesktopServices::openUrl(QUrl::fromUserInput(url.replace("%s", selection.at(0).data(TransactionTableModel::TxHashRole).toString())));
}

QWidget *TransactionView::createDateRangeWidget()
{
    // Create default dates in case nothing is persisted
    QString defaultDateFrom = QDate::currentDate().toString(PERSISTENCE_DATE_FORMAT);
    QString defaultDateTo = QDate::currentDate().addDays(1).toString(PERSISTENCE_DATE_FORMAT);
    QSettings settings;

    dateRangeWidget = new QFrame();
    dateRangeWidget->setFrameStyle(static_cast<int>(QFrame::Panel) | static_cast<int>(QFrame::Raised));
    dateRangeWidget->setContentsMargins(1,1,1,1);
    QHBoxLayout *layout = new QHBoxLayout(dateRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->addSpacing(23);
    layout->addWidget(new QLabel(tr("Range:")));

    dateFrom = new QDateTimeEdit(this);
    dateFrom->setCalendarPopup(true);
    dateFrom->setMinimumWidth(100);
    // Load persisted FROM date
    dateFrom->setDate(QDate::fromString(settings.value("transactionDateFrom", defaultDateFrom).toString(), PERSISTENCE_DATE_FORMAT));

    layout->addWidget(dateFrom);
    layout->addWidget(new QLabel(tr("to")));

    dateTo = new QDateTimeEdit(this);
    dateTo->setCalendarPopup(true);
    dateTo->setMinimumWidth(100);
    // Load persisted TO date
    dateTo->setDate(QDate::fromString(settings.value("transactionDateTo", defaultDateTo).toString(), PERSISTENCE_DATE_FORMAT));

    layout->addWidget(dateTo);
    layout->addStretch();

    // Hide by default
    dateRangeWidget->setVisible(false);

    // Notify on change
    connect(dateFrom, &QDateTimeEdit::dateChanged, this, &TransactionView::dateRangeChanged);
    connect(dateTo, &QDateTimeEdit::dateChanged, this, &TransactionView::dateRangeChanged);

    updateCalendarWidgets();

    return dateRangeWidget;
}

void TransactionView::updateCalendarWidgets()
{
    auto adjustWeekEndColors = [](QCalendarWidget* w) {
        QTextCharFormat format = w->weekdayTextFormat(Qt::Saturday);
        format.setForeground(QBrush(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::DEFAULT), Qt::SolidPattern));

        w->setWeekdayTextFormat(Qt::Saturday, format);
        w->setWeekdayTextFormat(Qt::Sunday, format);
    };

    adjustWeekEndColors(dateFrom->calendarWidget());
    adjustWeekEndColors(dateTo->calendarWidget());
}

void TransactionView::dateRangeChanged()
{
    if(!transactionProxyModel)
        return;

    // Persist new date range
    QSettings settings;
    settings.setValue("transactionDateFrom", dateFrom->date().toString(PERSISTENCE_DATE_FORMAT));
    settings.setValue("transactionDateTo", dateTo->date().toString(PERSISTENCE_DATE_FORMAT));

    transactionProxyModel->setDateRange(
            GUIUtil::StartOfDay(dateFrom->date()),
            GUIUtil::StartOfDay(dateTo->date()));
}

void TransactionView::focusTransaction(const QModelIndex &idx)
{
    if(!transactionProxyModel)
        return;
    QModelIndex targetIdx = transactionProxyModel->mapFromSource(idx);
    transactionView->selectRow(targetIdx.row());
    computeSum();
    transactionView->scrollTo(targetIdx);
    transactionView->setCurrentIndex(targetIdx);
    transactionView->setFocus();
}

void TransactionView::focusTransaction(const uint256& txid)
{
    if (!transactionProxyModel)
        return;

    const QModelIndexList results = this->model->getTransactionTableModel()->match(
        this->model->getTransactionTableModel()->index(0,0),
        TransactionTableModel::TxHashRole,
        QString::fromStdString(txid.ToString()), -1);

    transactionView->setFocus();
    transactionView->selectionModel()->clearSelection();
    for (const QModelIndex& index : results) {
        const QModelIndex targetIndex = transactionProxyModel->mapFromSource(index);
        transactionView->selectionModel()->select(
            targetIndex,
            QItemSelectionModel::Rows | QItemSelectionModel::Select);
        // Called once per destination to ensure all results are in view, unless
        // transactions are not ordered by (ascending or descending) date.
        transactionView->scrollTo(targetIndex);
        // scrollTo() does not scroll far enough the first time when transactions
        // are ordered by ascending date.
        if (index == results[0]) transactionView->scrollTo(targetIndex);
    }
}

void TransactionView::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::StyleChange) {
        updateCalendarWidgets();
    }
}

// Need to override default Ctrl+C action for amount as default behaviour is just to copy DisplayRole text
bool TransactionView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_C && ke->modifiers().testFlag(Qt::ControlModifier))
        {
             GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxPlainTextRole);
             return true;
        }
    }
    if (event->type() == QEvent::Show) {
        // Give the search field the first focus on startup
        static bool fGotFirstFocus = false;
        if (!fGotFirstFocus) {
            search_widget->setFocus();
            fGotFirstFocus = true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// show/hide column Watch-only
void TransactionView::updateWatchOnlyColumn(bool fHaveWatchOnly)
{
    watchOnlyWidget->setVisible(fHaveWatchOnly);
    transactionView->setColumnHidden(TransactionTableModel::Watchonly, !fHaveWatchOnly);
}

void TransactionView::updateCoinJoinVisibility()
{
    if (model == nullptr) {
        return;
    }
    bool fEnabled = model->node().coinJoinOptions().isEnabled();
    // If CoinJoin gets enabled use "All" else "Most common"
    int idx = fEnabled ? 0 : 1;
    chooseType(idx);
    typeWidget->setCurrentIndex(idx);
    // Hide all CoinJoin related filters
    QListView* typeList = qobject_cast<QListView*>(typeWidget->view());
    std::vector<int> vecRows{4, 5, 6, 7, 8};
    for (auto nRow : vecRows) {
        typeList->setRowHidden(nRow, !fEnabled);
    }
}
