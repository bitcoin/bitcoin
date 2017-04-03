#include "wallettests.h"

#include "qt/bitcoinamountfield.h"
#include "qt/callback.h"
#include "qt/optionsmodel.h"
#include "qt/platformstyle.h"
#include "qt/qvalidatedlineedit.h"
#include "qt/sendcoinsdialog.h"
#include "qt/sendcoinsentry.h"
#include "qt/transactiontablemodel.h"
#include "qt/walletmodel.h"
#include "test/test_bitcoin.h"
#include "validation.h"
#include "wallet/wallet.h"

#include <QAbstractButton>
#include <QApplication>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
//! Press "Yes" button in modal send confirmation dialog.
void ConfirmSend()
{
    QTimer::singleShot(0, makeCallback([](Callback* callback) {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (widget->inherits("SendConfirmationDialog")) {
                SendConfirmationDialog* dialog = qobject_cast<SendConfirmationDialog*>(widget);
                QAbstractButton* button = dialog->button(QMessageBox::Yes);
                button->setEnabled(true);
                button->click();
            }
        }
        delete callback;
    }), SLOT(call()));
}

//! Send coins to address and return txid.
uint256 SendCoins(CWallet& wallet, SendCoinsDialog& sendCoinsDialog, const CBitcoinAddress& address, CAmount amount)
{
    QVBoxLayout* entries = sendCoinsDialog.findChild<QVBoxLayout*>("entries");
    SendCoinsEntry* entry = qobject_cast<SendCoinsEntry*>(entries->itemAt(0)->widget());
    entry->findChild<QValidatedLineEdit*>("payTo")->setText(QString::fromStdString(address.ToString()));
    entry->findChild<BitcoinAmountField*>("payAmount")->setValue(amount);
    uint256 txid;
    boost::signals2::scoped_connection c(wallet.NotifyTransactionChanged.connect([&txid](CWallet*, const uint256& hash, ChangeType status) {
        if (status == CT_NEW) txid = hash;
    }));
    ConfirmSend();
    QMetaObject::invokeMethod(&sendCoinsDialog, "on_sendButton_clicked");
    return txid;
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
}

//! Simple qt wallet tests.
//
// Test widgets can be debugged interactively calling show() on them and
// manually running the event loop, e.g.:
//
//     sendCoinsDialog.show();
//     QEventLoop().exec();
//
// This also requires overriding the default minimal Qt platform:
//
//     src/qt/test/test_bitcoin-qt -platform xcb      # Linux
//     src/qt/test/test_bitcoin-qt -platform windows  # Windows
//     src/qt/test/test_bitcoin-qt -platform cocoa    # macOS
void WalletTests::walletTests()
{
    // Set up wallet and chain with 101 blocks (1 mature block for spending).
    TestChain100Setup test;
    test.CreateAndProcessBlock({}, GetScriptForRawPubKey(test.coinbaseKey.GetPubKey()));
    bitdb.MakeMock();
    CWallet wallet("wallet_test.dat");
    bool firstRun;
    wallet.LoadWallet(firstRun);
    {
        LOCK(wallet.cs_wallet);
        wallet.SetAddressBook(test.coinbaseKey.GetPubKey().GetID(), "", "receive");
        wallet.AddKeyPubKey(test.coinbaseKey, test.coinbaseKey.GetPubKey());
    }
    wallet.ScanForWalletTransactions(chainActive.Genesis(), true);
    wallet.SetBroadcastTransactions(true);

    // Create widgets for sending coins and listing transactions.
    std::unique_ptr<const PlatformStyle> platformStyle(PlatformStyle::instantiate("other"));
    SendCoinsDialog sendCoinsDialog(platformStyle.get());
    OptionsModel optionsModel;
    WalletModel walletModel(platformStyle.get(), &wallet, &optionsModel);
    sendCoinsDialog.setModel(&walletModel);

    // Send two transactions, and verify they are added to transaction list.
    TransactionTableModel* transactionTableModel = walletModel.getTransactionTableModel();
    QCOMPARE(transactionTableModel->rowCount({}), 101);
    uint256 txid1 = SendCoins(wallet, sendCoinsDialog, CBitcoinAddress(CKeyID()), 5 * COIN);
    uint256 txid2 = SendCoins(wallet, sendCoinsDialog, CBitcoinAddress(CKeyID()), 10 * COIN);
    QCOMPARE(transactionTableModel->rowCount({}), 103);
    QVERIFY(FindTx(*transactionTableModel, txid1).isValid());
    QVERIFY(FindTx(*transactionTableModel, txid2).isValid());

    bitdb.Flush(true);
    bitdb.Reset();
}
