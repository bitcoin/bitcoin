// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "paymentservertests.h"

#include "optionsmodel.h"
#include "paymentrequestdata.h"

#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <QFileOpenEvent>
#include <QTemporaryFile>

X509 *parse_b64der_cert(const char* cert_data)
{
    std::vector<unsigned char> data = DecodeBase64(cert_data);
    assert(data.size() > 0);
    const unsigned char* dptr = &data[0];
    X509 *cert = d2i_X509(NULL, &dptr, data.size());
    assert(cert);
    return cert;
}

//
// Test payment request handling
//

static SendCoinsRecipient handleRequest(PaymentServer* server, std::vector<unsigned char>& data)
{
    RecipientCatcher sigCatcher;
    QObject::connect(server, SIGNAL(receivedPaymentRequest(SendCoinsRecipient)),
        &sigCatcher, SLOT(getRecipient(SendCoinsRecipient)));

    // Write data to a temp file:
    QTemporaryFile f;
    f.open();
    f.write((const char*)&data[0], data.size());
    f.close();

    // Create a QObject, install event filter from PaymentServer
    // and send a file open event to the object
    QObject object;
    object.installEventFilter(server);
    QFileOpenEvent event(f.fileName());
    // If sending the event fails, this will cause sigCatcher to be empty,
    // which will lead to a test failure anyway.
    QCoreApplication::sendEvent(&object, &event);

    QObject::disconnect(server, SIGNAL(receivedPaymentRequest(SendCoinsRecipient)),
        &sigCatcher, SLOT(getRecipient(SendCoinsRecipient)));

    // Return results from sigCatcher
    return sigCatcher.recipient;
}

void PaymentServerTests::paymentServerTests()
{
    SelectParams(CBaseChainParams::MAIN);
    OptionsModel optionsModel;
    PaymentServer* server = new PaymentServer(NULL, false);
    X509_STORE* caStore = X509_STORE_new();
    X509_STORE_add_cert(caStore, parse_b64der_cert(caCert1_BASE64));
    PaymentServer::LoadRootCAs(caStore);
    server->setOptionsModel(&optionsModel);
    server->uiReady();

    std::vector<unsigned char> data;
    SendCoinsRecipient r;
    QString merchant;

    // Now feed PaymentRequests to server, and observe signals it produces

    // This payment request validates directly against the
    // caCert1 certificate authority:
    data = DecodeBase64(paymentrequest1_cert1_BASE64);
    r = handleRequest(server, data);
    r.paymentRequest.getMerchant(caStore, merchant);
    QCOMPARE(merchant, QString("testmerchant.org"));

    // Signed, but expired, merchant cert in the request:
    data = DecodeBase64(paymentrequest2_cert1_BASE64);
    r = handleRequest(server, data);
    r.paymentRequest.getMerchant(caStore, merchant);
    QCOMPARE(merchant, QString(""));

    // 10-long certificate chain, all intermediates valid:
    data = DecodeBase64(paymentrequest3_cert1_BASE64);
    r = handleRequest(server, data);
    r.paymentRequest.getMerchant(caStore, merchant);
    QCOMPARE(merchant, QString("testmerchant8.org"));

    // Long certificate chain, with an expired certificate in the middle:
    data = DecodeBase64(paymentrequest4_cert1_BASE64);
    r = handleRequest(server, data);
    r.paymentRequest.getMerchant(caStore, merchant);
    QCOMPARE(merchant, QString(""));

    // Validly signed, but by a CA not in our root CA list:
    data = DecodeBase64(paymentrequest5_cert1_BASE64);
    r = handleRequest(server, data);
    r.paymentRequest.getMerchant(caStore, merchant);
    QCOMPARE(merchant, QString(""));

    // Try again with no root CA's, verifiedMerchant should be empty:
    caStore = X509_STORE_new();
    PaymentServer::LoadRootCAs(caStore);
    data = DecodeBase64(paymentrequest1_cert1_BASE64);
    r = handleRequest(server, data);
    r.paymentRequest.getMerchant(caStore, merchant);
    QCOMPARE(merchant, QString(""));

    // Load second root certificate
    caStore = X509_STORE_new();
    X509_STORE_add_cert(caStore, parse_b64der_cert(caCert2_BASE64));
    PaymentServer::LoadRootCAs(caStore);

    QByteArray byteArray;

    // For the tests below we just need the payment request data from
    // paymentrequestdata.h parsed + stored in r.paymentRequest.
    //
    // These tests require us to bypass the following normal client execution flow
    // shown below to be able to explicitly just trigger a certain condition!
    //
    // handleRequest()
    // -> PaymentServer::eventFilter()
    //   -> PaymentServer::handleURIOrFile()
    //     -> PaymentServer::readPaymentRequestFromFile()
    //       -> PaymentServer::processPaymentRequest()

    // Contains a testnet paytoaddress, so payment request network doesn't match client network:
    data = DecodeBase64(paymentrequest1_cert2_BASE64);
    byteArray = QByteArray((const char*)&data[0], data.size());
    r.paymentRequest.parse(byteArray);
    // Ensure the request is initialized, because network "main" is default, even for
    // uninizialized payment requests and that will fail our test here.
    QVERIFY(r.paymentRequest.IsInitialized());
    QCOMPARE(PaymentServer::verifyNetwork(r.paymentRequest.getDetails()), false);

    // Just get some random data big enough to trigger BIP70 DoS protection
    unsigned char randData[BIP70_MAX_PAYMENTREQUEST_SIZE + 1];
    GetRandBytes(randData, sizeof(randData));
    // Write data to a temp file:
    QTemporaryFile tempFile;
    tempFile.open();
    tempFile.write((const char*)randData, sizeof(randData));
    tempFile.close();
    // Trigger BIP70 DoS protection
    QCOMPARE(PaymentServer::readPaymentRequestFromFile(tempFile.fileName(), r.paymentRequest), false);

    delete server;
}

void RecipientCatcher::getRecipient(SendCoinsRecipient r)
{
    recipient = r;
}
