#include "wallettests.h"

#include "consensus/validation.h"
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
#include "wallet/test/wallet_test_fixture.h"
#include "wallet/coincontrol.h"
#include "wallet/wallet.h"

#include <QAbstractButton>
#include <QApplication>
#include <QTimer>
#include <QVBoxLayout>

namespace
{

void TestLoadReceiveRequests()
{
    WalletTestingSetup test;
    OptionsModel optionsModel;
    WalletModel walletModel(nullptr, pwalletMain, &optionsModel);

    CTxDestination dest = CKeyID();
    pwalletMain->AddDestData(dest, "misc", "val_misc");
    pwalletMain->AddDestData(dest, "rr0", "val_rr0");
    pwalletMain->AddDestData(dest, "rr1", "val_rr1");

    std::vector<std::string> values;
    walletModel.loadReceiveRequests(values);
    QCOMPARE((int)values.size(), 2);
    QCOMPARE(QString::fromStdString(values[0]), QString("val_rr0"));
    QCOMPARE(QString::fromStdString(values[1]), QString("val_rr1"));
}

class ListCoinsTestingSetup : public TestChain100Setup
{
public:
    ListCoinsTestingSetup()
    {
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        ::bitdb.MakeMock();
        wallet.reset(new CWallet(std::unique_ptr<CWalletDBWrapper>(new CWalletDBWrapper(&bitdb, "wallet_test.dat"))));
        bool firstRun;
        wallet->LoadWallet(firstRun);
        LOCK(wallet->cs_wallet);
        wallet->AddKeyPubKey(coinbaseKey, coinbaseKey.GetPubKey());
        wallet->ScanForWalletTransactions(chainActive.Genesis());
    }

    ~ListCoinsTestingSetup()
    {
        ::bitdb.Flush(true);
        ::bitdb.Reset();
    }

    CWalletTx& AddTx(CRecipient recipient)
    {
        CWalletTx wtx;
        CReserveKey reservekey(wallet.get());
        CAmount fee;
        int changePos = -1;
        std::string error;
        wallet->CreateTransaction({recipient}, wtx, reservekey, fee, changePos, error);
        CValidationState state;
        wallet->CommitTransaction(wtx, reservekey, nullptr, state);
        auto it = wallet->mapWallet.find(wtx.GetHash());
        CreateAndProcessBlock({CMutableTransaction(*it->second.tx)}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        it->second.SetMerkleBranch(chainActive.Tip(), 1);
        return it->second;
    }

    std::unique_ptr<CWallet> wallet;
};

void TestListCoins()
{
    ListCoinsTestingSetup test;
    OptionsModel optionsModel;
    WalletModel walletModel(nullptr, test.wallet.get(), &optionsModel);
    QString coinbaseAddress = QString::fromStdString(CBitcoinAddress(test.coinbaseKey.GetPubKey().GetID()).ToString());

    LOCK(test.wallet->cs_wallet);

    // Confirm ListCoins initially returns 1 coin grouped under coinbaseKey
    // address.
    std::map<QString, std::vector<COutput>> list;
    walletModel.listCoins(list);
    QCOMPARE((int)list.size(), 1);
    QCOMPARE(list.begin()->first, coinbaseAddress);
    QCOMPARE((int)list.begin()->second.size(), 1);

    // Check initial balance from one mature coinbase transaction.
    CCoinControl coinControl;
    QCOMPARE(50 * COIN, walletModel.getBalance(&coinControl));

    // Add a transaction creating a change address, and confirm ListCoins still
    // returns the coin associated with the change address underneath the
    // coinbaseKey pubkey, even though the change address has a different
    // pubkey.
    test.AddTx(CRecipient{GetScriptForRawPubKey({}), 1 * COIN, false /* subtract fee */});
    list.clear();
    walletModel.listCoins(list);
    QCOMPARE((int)list.size(), 1);
    QCOMPARE(list.begin()->first, coinbaseAddress);
    QCOMPARE((int)list.begin()->second.size(), 2);

    // Lock both coins. Confirm number of available coins drops to 0.
    std::vector<COutput> available;
    test.wallet->AvailableCoins(available);
    QCOMPARE((int)available.size(), 2);
    for (const auto& group : list) {
        for (const auto& coin : group.second) {
            test.wallet->LockCoin(COutPoint(coin.tx->GetHash(), coin.i));
        }
    }
    test.wallet->AvailableCoins(available);
    QCOMPARE((int)available.size(), 0);

    // Confirm ListCoins still returns same result as before, despite coins
    // being locked.
    list.clear();
    walletModel.listCoins(list);
    QCOMPARE((int)list.size(), 1);
    QCOMPARE(list.begin()->first, coinbaseAddress);
    QCOMPARE((int)list.begin()->second.size(), 2);
}

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
void TestSendCoins()
{
    // Set up wallet and chain with 101 blocks (1 mature block for spending).
    TestChain100Setup test;
    test.CreateAndProcessBlock({}, GetScriptForRawPubKey(test.coinbaseKey.GetPubKey()));
    bitdb.MakeMock();
    std::unique_ptr<CWalletDBWrapper> dbw(new CWalletDBWrapper(&bitdb, "wallet_test.dat"));
    CWallet wallet(std::move(dbw));
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

}

void WalletTests::walletTests()
{
    TestLoadReceiveRequests();
    TestListCoins();
    TestSendCoins();
}
