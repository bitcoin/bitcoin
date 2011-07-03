#include "transactionview.h"

// Temp includes for filtering prototype
// Move to TransactionFilterRow class
#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "transactiontablemodel.h"
#include "guiutil.h"

#include <QScrollBar>
#include <QComboBox>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QTableView>
#include <QHeaderView>

#include <QDebug>

TransactionView::TransactionView(QWidget *parent) :
    QWidget(parent), model(0), transactionProxyModel(0),
    transactionView(0)
{
    // Build filter row
    setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);
    hlayout->setSpacing(0);

    hlayout->addSpacing(23);

    dateWidget = new QComboBox(this);
    dateWidget->setMaximumWidth(120);
    dateWidget->setMinimumWidth(120);
    dateWidget->addItem(tr("All"), All);
    dateWidget->addItem(tr("Today"), Today);
    dateWidget->addItem(tr("This week"), ThisWeek);
    dateWidget->addItem(tr("This month"), ThisMonth);
    dateWidget->addItem(tr("Last month"), LastMonth);
    dateWidget->addItem(tr("This year"), ThisYear);
    dateWidget->addItem(tr("Range..."), Range);
    hlayout->addWidget(dateWidget);

    typeWidget = new QComboBox(this);
    typeWidget->setMaximumWidth(120);
    typeWidget->setMinimumWidth(120);

    typeWidget->addItem(tr("All"), TransactionFilterProxy::ALL_TYPES);
    typeWidget->addItem(tr("Received with"), TransactionFilterProxy::TYPE(TransactionRecord::RecvWithAddress) |
                                        TransactionFilterProxy::TYPE(TransactionRecord::RecvFromIP));
    typeWidget->addItem(tr("Sent to"), TransactionFilterProxy::TYPE(TransactionRecord::SendToAddress) |
                                  TransactionFilterProxy::TYPE(TransactionRecord::SendToIP));
    typeWidget->addItem(tr("To yourself"), TransactionFilterProxy::TYPE(TransactionRecord::SendToSelf));
    typeWidget->addItem(tr("Generated"), TransactionFilterProxy::TYPE(TransactionRecord::Generated));
    typeWidget->addItem(tr("Other"), TransactionFilterProxy::TYPE(TransactionRecord::Other));

    hlayout->addWidget(typeWidget);

    addressWidget = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    addressWidget->setPlaceholderText("Enter address or label to search");
#endif
    hlayout->addWidget(addressWidget);

    amountWidget = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    amountWidget->setPlaceholderText("Min amount");
#endif
    amountWidget->setMaximumWidth(100);
    amountWidget->setMinimumWidth(100);
    amountWidget->setValidator(new QDoubleValidator(0, 1e20, 8, this));
    hlayout->addWidget(amountWidget);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(view);
    vlayout->setSpacing(0);
    int width = view->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
    hlayout->addSpacing(width);
    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    transactionView = view;

    connect(dateWidget, SIGNAL(activated(int)), this, SLOT(chooseDate(int)));
    connect(typeWidget, SIGNAL(activated(int)), this, SLOT(chooseType(int)));
    connect(addressWidget, SIGNAL(textChanged(const QString&)), this, SLOT(changedPrefix(const QString&)));
    connect(amountWidget, SIGNAL(textChanged(const QString&)), this, SLOT(changedAmount(const QString&)));

    connect(view, SIGNAL(doubleClicked(const QModelIndex&)), this, SIGNAL(doubleClicked(const QModelIndex&)));
}

void TransactionView::setModel(TransactionTableModel *model)
{
    this->model = model;

    transactionProxyModel = new TransactionFilterProxy(this);
    transactionProxyModel->setSourceModel(model);
    transactionProxyModel->setDynamicSortFilter(true);

    transactionProxyModel->setSortRole(Qt::EditRole);

    transactionView->setModel(transactionProxyModel);
    transactionView->setAlternatingRowColors(true);
    transactionView->setSelectionBehavior(QAbstractItemView::SelectRows);
    transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    transactionView->setSortingEnabled(true);
    transactionView->sortByColumn(TransactionTableModel::Status, Qt::DescendingOrder);
    transactionView->verticalHeader()->hide();

    transactionView->horizontalHeader()->resizeSection(
            TransactionTableModel::Status, 23);
    transactionView->horizontalHeader()->resizeSection(
            TransactionTableModel::Date, 120);
    transactionView->horizontalHeader()->resizeSection(
            TransactionTableModel::Type, 120);
    transactionView->horizontalHeader()->setResizeMode(
            TransactionTableModel::ToAddress, QHeaderView::Stretch);
    transactionView->horizontalHeader()->resizeSection(
            TransactionTableModel::Amount, 100);

}

void TransactionView::chooseDate(int idx)
{
    QDate current = QDate::currentDate();
    switch(dateWidget->itemData(idx).toInt())
    {
    case All:
        transactionProxyModel->setDateRange(
                TransactionFilterProxy::MIN_DATE,
                TransactionFilterProxy::MAX_DATE);
        break;
    case Today:
        transactionProxyModel->setDateRange(
                QDateTime(current),
                TransactionFilterProxy::MAX_DATE);
        break;
    case ThisWeek: {
        // Find last monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        transactionProxyModel->setDateRange(
                QDateTime(startOfWeek),
                TransactionFilterProxy::MAX_DATE);

        } break;
    case ThisMonth:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month(), 1)),
                TransactionFilterProxy::MAX_DATE);
        break;
    case LastMonth:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month()-1, 1)),
                QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), 1, 1)),
                TransactionFilterProxy::MAX_DATE);
        break;
    case Range:
        // TODO ask specific range
        break;
    }

}

void TransactionView::chooseType(int idx)
{
    transactionProxyModel->setTypeFilter(
        typeWidget->itemData(idx).toInt());
}

void TransactionView::changedPrefix(const QString &prefix)
{
    transactionProxyModel->setAddressPrefix(prefix);
}

void TransactionView::changedAmount(const QString &amount)
{
    qint64 amount_parsed = 0;
    if(GUIUtil::parseMoney(amount, &amount_parsed))
    {
        transactionProxyModel->setMinAmount(amount_parsed);
    }
    else
    {
        transactionProxyModel->setMinAmount(0);
    }
}
