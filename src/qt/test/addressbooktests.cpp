// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/test/addressbooktests.h>
#include <qt/test/util.h>
#include <test/util/setup_common.h>

#include <interfaces/chain.h>
#include <interfaces/node.h>
#include <qt/clientmodel.h>
#include <qt/editaddressdialog.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/walletmodel.h>

#include <key.h>
#include <key_io.h>
#include <wallet/wallet.h>
#include <walletinitinterface.h>

#include <QApplication>
#include <QTimer>
#include <QMessageBox>

namespace
{

/**
 * Fill the edit address dialog box with data, submit it, and ensure that
 * the resulting message meets expectations.
 */
void EditAddressAndSubmit(
        EditAddressDialog* dialog,
        const QString& label, const QString& address, QString expected_msg)
{
    QString warning_text;

    dialog->findChild<QLineEdit*>("labelEdit")->setText(label);
    dialog->findChild<QValidatedLineEdit*>("addressEdit")->setText(address);

    ConfirmMessage(&warning_text, 5);
    dialog->accept();
    QCOMPARE(warning_text, expected_msg);
}

/**
 * Test adding various send addresses to the address book.
 *
 * There are three cases tested:
 *
 *   - new_address: a new address which should add as a send address successfully.
 *   - existing_s_address: an existing sending address which won't add successfully.
 *   - existing_r_address: an existing receiving address which won't add successfully.
 *
 * In each case, verify the resulting state of the address book and optionally
 * the warning message presented to the user.
 */
void TestAddAddressesToSendBook(interfaces::Node& node)
{
    TestChain100Setup test;
    node.setContext(&test.m_node);
    std::shared_ptr<CWallet> wallet = std::make_shared<CWallet>(node.context()->chain.get(), WalletLocation(), CreateMockWalletDatabase());
    wallet->SetupLegacyScriptPubKeyMan();
    bool firstRun;
    wallet->LoadWallet(firstRun);

    auto build_address = [&wallet]() {
        CKey key;
        key.MakeNewKey(true);
        CTxDestination dest(GetDestinationForKey(
            key.GetPubKey(), wallet->m_default_address_type));

        return std::make_pair(dest, QString::fromStdString(EncodeDestination(dest)));
    };

    CTxDestination r_key_dest, s_key_dest;

    // Add a preexisting "receive" entry in the address book.
    QString preexisting_r_address;
    QString r_label("already here (r)");

    // Add a preexisting "send" entry in the address book.
    QString preexisting_s_address;
    QString s_label("already here (s)");

    // Define a new address (which should add to the address book successfully).
    QString new_address;

    std::tie(r_key_dest, preexisting_r_address) = build_address();
    std::tie(s_key_dest, preexisting_s_address) = build_address();
    std::tie(std::ignore, new_address) = build_address();

    {
        LOCK(wallet->cs_wallet);
        wallet->SetAddressBook(r_key_dest, r_label.toStdString(), "receive");
        wallet->SetAddressBook(s_key_dest, s_label.toStdString(), "send");
    }

    auto check_addbook_size = [&wallet](int expected_size) {
        LOCK(wallet->cs_wallet);
        QCOMPARE(static_cast<int>(wallet->m_address_book.size()), expected_size);
    };

    // We should start with the two addresses we added earlier and nothing else.
    check_addbook_size(2);

    // Initialize relevant QT models.
    std::unique_ptr<const PlatformStyle> platformStyle(PlatformStyle::instantiate("other"));
    OptionsModel optionsModel(node);
    ClientModel clientModel(node, &optionsModel);
    AddWallet(wallet);
    WalletModel walletModel(interfaces::MakeWallet(wallet), clientModel, platformStyle.get());
    RemoveWallet(wallet);
    EditAddressDialog editAddressDialog(EditAddressDialog::NewSendingAddress);
    editAddressDialog.setModel(walletModel.getAddressTableModel());

    EditAddressAndSubmit(
        &editAddressDialog, QString("uhoh"), preexisting_r_address,
        QString(
            "Address \"%1\" already exists as a receiving address with label "
            "\"%2\" and so cannot be added as a sending address."
            ).arg(preexisting_r_address).arg(r_label));

    check_addbook_size(2);

    EditAddressAndSubmit(
        &editAddressDialog, QString("uhoh, different"), preexisting_s_address,
        QString(
            "The entered address \"%1\" is already in the address book with "
            "label \"%2\"."
            ).arg(preexisting_s_address).arg(s_label));

    check_addbook_size(2);

    // Submit a new address which should add successfully - we expect the
    // warning message to be blank.
    EditAddressAndSubmit(
        &editAddressDialog, QString("new"), new_address, QString(""));

    check_addbook_size(3);
}

} // namespace

void AddressBookTests::addressBookTests()
{
#ifdef Q_OS_MAC
    if (QApplication::platformName() == "minimal") {
        // Disable for mac on "minimal" platform to avoid crashes inside the Qt
        // framework when it tries to look up unimplemented cocoa functions,
        // and fails to handle returned nulls
        // (https://bugreports.qt.io/browse/QTBUG-49686).
        QWARN("Skipping AddressBookTests on mac build with 'minimal' platform set due to Qt bugs. To run AppTests, invoke "
              "with 'QT_QPA_PLATFORM=cocoa test_bitcoin-qt' on mac, or else use a linux or windows build.");
        return;
    }
#endif
    TestAddAddressesToSendBook(m_node);
}
