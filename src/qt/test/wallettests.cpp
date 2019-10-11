#include <qt/test/wallettests.h>
#include <qt/test/util.h>

#include <interfaces/chain.h>
#include <interfaces/node.h>
#include <qt/bitcoinamountfield.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/sendcompose.h>
#include <qt/senddialog.h>
#include <qt/sendfinish.h>
#include <qt/sendcoinsentry.h>
#include <qt/sendsign.h>
#include <qt/transactiontablemodel.h>
#include <qt/transactionview.h>
#include <qt/walletmodel.h>
#include <key_io.h>
#include <test/setup_common.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <qt/overviewpage.h>
#include <qt/receivecoinsdialog.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/receiverequestdialog.h>

#include <memory>

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QListView>
#include <QDialogButtonBox>

namespace
{

//! Send coins to address and return txid.
void SendCoins(uint256& txid, CWallet& wallet, SendDialog& sendDialog, const CTxDestination& address, CAmount amount, bool rbf)
{
    QVERIFY(sendDialog.draftWidget->isActiveWidget());
    QVBoxLayout* entries = sendDialog.findChild<QVBoxLayout*>("entries");
    SendCoinsEntry* entry = qobject_cast<SendCoinsEntry*>(entries->itemAt(0)->widget());
    entry->findChild<QValidatedLineEdit*>("payTo")->setText(QString::fromStdString(EncodeDestination(address)));
    entry->findChild<BitcoinAmountField*>("payAmount")->setValue(amount);
    sendDialog.findChild<QFrame*>("frameFee")
        ->findChild<QFrame*>("frameFeeSelection")
        ->findChild<QCheckBox*>("optInRBF")
        ->setCheckState(rbf ? Qt::Checked : Qt::Unchecked);
    boost::signals2::scoped_connection c(wallet.NotifyTransactionChanged.connect([&txid](CWallet*, const uint256& hash, ChangeType status) {
        if (status == CT_NEW) txid = hash;
    }));
    bool invoked = QMetaObject::invokeMethod(sendDialog.draftWidget, "on_sendButton_clicked");
    assert(invoked);
    QVERIFY(sendDialog.signWidget->isActiveWidget());
    sendDialog.signWidget->confirm();
    // NotifyTransactionChanged in WalletModel calls "updateTransaction" using a Qt::QueuedConnection
    QTest::qWait(0);
    QVERIFY(sendDialog.finishWidget->isActiveWidget());
    sendDialog.finishWidget->goNewTransaction();
    QVERIFY(sendDialog.draftWidget->isActiveWidget());
}

//! Find index of txid in transaction list.
QModelIndex FindTx(const QAbstractItemModel& model, const uint256& txid)
{
    QString hash = QString::fromStdString(txid.ToString());
    int rows = model.rowCount({});
    for (int row = 0; row < rows; ++row) {
        QModelIndex index = model.index(row, 0, {});
        if (model.data(index, TransactionTableModel::TxHashRole) == hash) {
            return index;
        }
    }
    return {};
}

//! Invoke bumpfee on txid and check results.
void BumpFee(SendDialog& sendDialog, TransactionView& view, const uint256& txid, bool expectDisabled, std::string expectError, bool cancel)
{
    QTableView* table = view.findChild<QTableView*>("transactionView");
    QModelIndex index = FindTx(*table->selectionModel()->model(), txid);
    QVERIFY2(index.isValid(), "Could not find BumpFee txid");

    // Select row in table, invoke context menu, and make sure bumpfee action is
    // enabled or disabled as expected.
    QAction* action = view.findChild<QAction*>("bumpFeeAction");
    table->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    table->customContextMenuRequested({});
    QCOMPARE(action->isEnabled(), !expectDisabled);

    // TransactionView::gotoBumpFee is connected to SendDialog::gotoBumpFee in WalletView
    // Perform this action regardless of isEnabled in order to test error messages
    QString text;
    if (!expectError.empty()) {
        ConfirmMessage(&text);
    }
    sendDialog.gotoBumpFee(txid);

    if (cancel) {
        sendDialog.signWidget->cancel();
    } else {
        if (expectError.empty()) {
            // If the sign widget is not active, it means there was an unexpected error
            QVERIFY(sendDialog.signWidget->isActiveWidget());
        } else {
            // The error message didn't occur during gotoBumpFee(), see if it occurs during broadcast
            ConfirmMessage(&text);
        }
        if (sendDialog.signWidget->isActiveWidget()) {
            sendDialog.signWidget->confirm();
        }
    }

    if (!expectError.empty()) {
        QVERIFY(text.indexOf(QString::fromStdString(expectError)) != -1);
    }

    // NotifyTransactionChanged in WalletModel calls "updateTransaction" using a Qt::QueuedConnection
    QTest::qWait(0);
}

//! Simple qt wallet tests.
//
// Test widgets can be debugged interactively calling show() on them and
// manually running the event loop, e.g.:
//
//     sendDialog.show();
//     QEventLoop().exec();
//
// This also requires overriding the default minimal Qt platform:
//
//     QT_QPA_PLATFORM=xcb     src/qt/test/test_bitcoin-qt  # Linux
//     QT_QPA_PLATFORM=windows src/qt/test/test_bitcoin-qt  # Windows
//     QT_QPA_PLATFORM=cocoa   src/qt/test/test_bitcoin-qt  # macOS
void TestGUI()
{
    // Set up wallet and chain with 105 blocks (5 mature blocks for spending).
    TestChain100Setup test;
    for (int i = 0; i < 5; ++i) {
        test.CreateAndProcessBlock({}, GetScriptForRawPubKey(test.coinbaseKey.GetPubKey()));
    }
    auto chain = interfaces::MakeChain();
    std::shared_ptr<CWallet> wallet = std::make_shared<CWallet>(chain.get(), WalletLocation(), WalletDatabase::CreateMock());
    bool firstRun;
    wallet->LoadWallet(firstRun);
    {
        LOCK(wallet->cs_wallet);
        wallet->SetAddressBook(GetDestinationForKey(test.coinbaseKey.GetPubKey(), wallet->m_default_address_type), "", "receive");
        wallet->AddKeyPubKey(test.coinbaseKey, test.coinbaseKey.GetPubKey());
    }
    {
        auto locked_chain = wallet->chain().lock();
        LockAssertion lock(::cs_main);

        WalletRescanReserver reserver(wallet.get());
        reserver.reserve();
        CWallet::ScanResult result = wallet->ScanForWalletTransactions(locked_chain->getBlockHash(0), {} /* stop_block */, reserver, true /* fUpdate */);
        QCOMPARE(result.status, CWallet::ScanResult::SUCCESS);
        QCOMPARE(result.last_scanned_block, ::ChainActive().Tip()->GetBlockHash());
        QVERIFY(result.last_failed_block.IsNull());
    }
    wallet->SetBroadcastTransactions(true);

    // Create widgets for sending coins and listing transactions.
    std::unique_ptr<const PlatformStyle> platformStyle(PlatformStyle::instantiate("other"));
    SendDialog sendDialog(platformStyle.get());
    TransactionView transactionView(platformStyle.get());
    auto node = interfaces::MakeNode();
    OptionsModel optionsModel(*node);
    AddWallet(wallet);
    WalletModel walletModel(std::move(node->getWallets().back()), *node, platformStyle.get(), &optionsModel);
    RemoveWallet(wallet);
    sendDialog.setModel(&walletModel);
    transactionView.setModel(&walletModel);

    // Send two transactions, and verify they are added to transaction list.
    TransactionTableModel* transactionTableModel = walletModel.getTransactionTableModel();
    QCOMPARE(transactionTableModel->rowCount({}), 105);
    uint256 txid1, txid2;
    SendCoins(txid1, *wallet.get(), sendDialog, PKHash(), 5 * COIN, false /* rbf */);
    SendCoins(txid2, *wallet.get(), sendDialog, PKHash(), 10 * COIN, true /* rbf */);

    QCOMPARE(transactionTableModel->rowCount({}), 107);
    QVERIFY(FindTx(*transactionTableModel, txid1).isValid());
    QVERIFY(FindTx(*transactionTableModel, txid2).isValid());

    // Call bumpfee. Test disabled, canceled, enabled, then failing cases.
    qDebug() << "Bumpfee non-RBF transaction";
    BumpFee(sendDialog, transactionView, txid1, true /* expect disabled */, "not BIP 125 replaceable" /* expected error */, false /* cancel */);
    QCOMPARE(transactionTableModel->rowCount({}), 107);
    qDebug() << "Cancel bumpfee RBF transaction";
    BumpFee(sendDialog, transactionView, txid2, false /* expect disabled */, {} /* expected error */, true /* cancel */);
    QCOMPARE(transactionTableModel->rowCount({}), 107);
    qDebug() << "Bumpfee RBF transaction";
    BumpFee(sendDialog, transactionView, txid2, false /* expect disabled */, {} /* expected error */, false /* cancel */);
    QCOMPARE(transactionTableModel->rowCount({}), 108);
    qDebug() << "Bumpfee already bumped transaction";
    BumpFee(sendDialog, transactionView, txid2, true /* expect disabled */, "already bumped" /* expected error */, false /* cancel */);
    QCOMPARE(transactionTableModel->rowCount({}), 108);

    // Check current balance on OverviewPage
    OverviewPage overviewPage(platformStyle.get());
    overviewPage.setWalletModel(&walletModel);
    QLabel* balanceLabel = overviewPage.findChild<QLabel*>("labelBalance");
    QString balanceText = balanceLabel->text();
    int unit = walletModel.getOptionsModel()->getDisplayUnit();
    CAmount balance = walletModel.wallet().getBalance();
    QString balanceComparison = BitcoinUnits::formatWithUnit(unit, balance, false, BitcoinUnits::separatorAlways);
    QCOMPARE(balanceText, balanceComparison);

    // Check Request Payment button
    ReceiveCoinsDialog receiveCoinsDialog(platformStyle.get());
    receiveCoinsDialog.setModel(&walletModel);
    RecentRequestsTableModel* requestTableModel = walletModel.getRecentRequestsTableModel();

    // Label input
    QLineEdit* labelInput = receiveCoinsDialog.findChild<QLineEdit*>("reqLabel");
    labelInput->setText("TEST_LABEL_1");

    // Amount input
    BitcoinAmountField* amountInput = receiveCoinsDialog.findChild<BitcoinAmountField*>("reqAmount");
    amountInput->setValue(1);

    // Message input
    QLineEdit* messageInput = receiveCoinsDialog.findChild<QLineEdit*>("reqMessage");
    messageInput->setText("TEST_MESSAGE_1");
    int initialRowCount = requestTableModel->rowCount({});
    QPushButton* requestPaymentButton = receiveCoinsDialog.findChild<QPushButton*>("receiveButton");
    requestPaymentButton->click();
    for (QWidget* widget : QApplication::topLevelWidgets()) {
        if (widget->inherits("ReceiveRequestDialog")) {
            ReceiveRequestDialog* receiveRequestDialog = qobject_cast<ReceiveRequestDialog*>(widget);
            QTextEdit* rlist = receiveRequestDialog->QObject::findChild<QTextEdit*>("outUri");
            QString paymentText = rlist->toPlainText();
            QStringList paymentTextList = paymentText.split('\n');
            QCOMPARE(paymentTextList.at(0), QString("Payment information"));
            QVERIFY(paymentTextList.at(1).indexOf(QString("URI: bitcoin:")) != -1);
            QVERIFY(paymentTextList.at(2).indexOf(QString("Address:")) != -1);
            QCOMPARE(paymentTextList.at(3), QString("Amount: 0.00000001 ") + QString::fromStdString(CURRENCY_UNIT));
            QCOMPARE(paymentTextList.at(4), QString("Label: TEST_LABEL_1"));
            QCOMPARE(paymentTextList.at(5), QString("Message: TEST_MESSAGE_1"));
        }
    }

    // Clear button
    QPushButton* clearButton = receiveCoinsDialog.findChild<QPushButton*>("clearButton");
    clearButton->click();
    QCOMPARE(labelInput->text(), QString(""));
    QCOMPARE(amountInput->value(), CAmount(0));
    QCOMPARE(messageInput->text(), QString(""));

    // Check addition to history
    int currentRowCount = requestTableModel->rowCount({});
    QCOMPARE(currentRowCount, initialRowCount+1);

    // Check Remove button
    QTableView* table = receiveCoinsDialog.findChild<QTableView*>("recentRequestsView");
    table->selectRow(currentRowCount-1);
    QPushButton* removeRequestButton = receiveCoinsDialog.findChild<QPushButton*>("removeRequestButton");
    removeRequestButton->click();
    QCOMPARE(requestTableModel->rowCount({}), currentRowCount-1);
}

} // namespace

void WalletTests::walletTests()
{
#ifdef Q_OS_MAC
    if (QApplication::platformName() == "minimal") {
        // Disable for mac on "minimal" platform to avoid crashes inside the Qt
        // framework when it tries to look up unimplemented cocoa functions,
        // and fails to handle returned nulls
        // (https://bugreports.qt.io/browse/QTBUG-49686).
        QWARN("Skipping WalletTests on mac build with 'minimal' platform set due to Qt bugs. To run AppTests, invoke "
              "with 'QT_QPA_PLATFORM=cocoa test_bitcoin-qt' on mac, or else use a linux or windows build.");
        return;
    }
#endif
    TestGUI();
}
