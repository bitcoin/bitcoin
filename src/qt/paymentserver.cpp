// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "paymentserver.h"

#include "bitcoinunits.h"
#include "guiutil.h"
#include "optionsmodel.h"

#include "base58.h"
#include "chainparams.h"
#include "policy/policy.h"
#include "ui_interface.h"
#include "util.h"
#include "wallet/wallet.h"

#include <cstdlib>

#include <openssl/x509_vfy.h>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileOpenEvent>
#include <QHash>
#include <QList>
#include <QLocalServer>
#include <QLocalSocket>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslCertificate>
#include <QSslError>
#include <QSslSocket>
#include <QStringList>
#include <QTextDocument>

#if QT_VERSION < 0x050000
#include <QUrl>
#else
#include <QUrlQuery>
#endif

const int BITCOIN_IPC_CONNECT_TIMEOUT = 1000; // milliseconds
const QString BITCOIN_IPC_PREFIX("bitcoin:");
// BIP70 payment protocol messages
const char* BIP70_MESSAGE_PAYMENTACK = "PaymentACK";
const char* BIP70_MESSAGE_PAYMENTREQUEST = "PaymentRequest";
// BIP71 payment protocol media types
const char* BIP71_MIMETYPE_PAYMENT = "application/bitcoin-payment";
const char* BIP71_MIMETYPE_PAYMENTACK = "application/bitcoin-paymentack";
const char* BIP71_MIMETYPE_PAYMENTREQUEST = "application/bitcoin-paymentrequest";

struct X509StoreDeleter {
      void operator()(X509_STORE* b) {
          X509_STORE_free(b);
      }
};

struct X509Deleter {
      void operator()(X509* b) { X509_free(b); }
};

namespace // Anon namespace
{
    std::unique_ptr<X509_STORE, X509StoreDeleter> certStore;
}

//
// Create a name that is unique for:
//  testnet / non-testnet
//  data directory
//
static QString ipcServerName()
{
    QString name("BitcoinQt");

    // Append a simple hash of the datadir
    // Note that GetDataDir(true) returns a different path
    // for -testnet versus main net
    QString ddir(GUIUtil::boostPathToQString(GetDataDir(true)));
    name.append(QString::number(qHash(ddir)));

    return name;
}

//
// We store payment URIs and requests received before
// the main GUI window is up and ready to ask the user
// to send payment.

static QList<QString> savedPaymentRequests;

static void ReportInvalidCertificate(const QSslCertificate& cert)
{
#if QT_VERSION < 0x050000
    qDebug() << QString("%1: Payment server found an invalid certificate: ").arg(__func__) << cert.serialNumber() << cert.subjectInfo(QSslCertificate::CommonName) << cert.subjectInfo(QSslCertificate::OrganizationalUnitName);
#else
    qDebug() << QString("%1: Payment server found an invalid certificate: ").arg(__func__) << cert.serialNumber() << cert.subjectInfo(QSslCertificate::CommonName) << cert.subjectInfo(QSslCertificate::DistinguishedNameQualifier) << cert.subjectInfo(QSslCertificate::OrganizationalUnitName);
#endif
}

//
// Load OpenSSL's list of root certificate authorities
//
void PaymentServer::LoadRootCAs(X509_STORE* _store)
{
    // Unit tests mostly use this, to pass in fake root CAs:
    if (_store)
    {
        certStore.reset(_store);
        return;
    }

    // Normal execution, use either -rootcertificates or system certs:
    certStore.reset(X509_STORE_new());

    // Note: use "-system-" default here so that users can pass -rootcertificates=""
    // and get 'I don't like X.509 certificates, don't trust anybody' behavior:
    QString certFile = QString::fromStdString(gArgs.GetArg("-rootcertificates", "-system-"));

    // Empty store
    if (certFile.isEmpty()) {
        qDebug() << QString("PaymentServer::%1: Payment request authentication via X.509 certificates disabled.").arg(__func__);
        return;
    }

    QList<QSslCertificate> certList;

    if (certFile != "-system-") {
            qDebug() << QString("PaymentServer::%1: Using \"%2\" as trusted root certificate.").arg(__func__).arg(certFile);

        certList = QSslCertificate::fromPath(certFile);
        // Use those certificates when fetching payment requests, too:
        QSslSocket::setDefaultCaCertificates(certList);
    } else
        certList = QSslSocket::systemCaCertificates();

    int nRootCerts = 0;
    const QDateTime currentTime = QDateTime::currentDateTime();

    for (const QSslCertificate& cert : certList) {
        // Don't log NULL certificates
        if (cert.isNull())
            continue;

        // Not yet active/valid, or expired certificate
        if (currentTime < cert.effectiveDate() || currentTime > cert.expiryDate()) {
            ReportInvalidCertificate(cert);
            continue;
        }

#if QT_VERSION >= 0x050000
        // Blacklisted certificate
        if (cert.isBlacklisted()) {
            ReportInvalidCertificate(cert);
            continue;
        }
#endif
        QByteArray certData = cert.toDer();
        const unsigned char *data = (const unsigned char *)certData.data();

        std::unique_ptr<X509, X509Deleter> x509(d2i_X509(0, &data, certData.size()));
        if (x509 && X509_STORE_add_cert(certStore.get(), x509.get()))
        {
            // Note: X509_STORE increases the reference count to the X509 object,
            // we still have to release our reference to it.
            ++nRootCerts;
        }
        else
        {
            ReportInvalidCertificate(cert);
            continue;
        }
    }
    qWarning() << "PaymentServer::LoadRootCAs: Loaded " << nRootCerts << " root certificates";

    // Project for another day:
    // Fetch certificate revocation lists, and add them to certStore.
    // Issues to consider:
    //   performance (start a thread to fetch in background?)
    //   privacy (fetch through tor/proxy so IP address isn't revealed)
    //   would it be easier to just use a compiled-in blacklist?
    //    or use Qt's blacklist?
    //   "certificate stapling" with server-side caching is more efficient
}

//
// Sending to the server is done synchronously, at startup.
// If the server isn't already running, startup continues,
// and the items in savedPaymentRequest will be handled
// when uiReady() is called.
//
// Warning: ipcSendCommandLine() is called early in init,
// so don't use "Q_EMIT message()", but "QMessageBox::"!
//
void PaymentServer::ipcParseCommandLine(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        QString arg(argv[i]);
        if (arg.startsWith("-"))
            continue;

        // If the bitcoin: URI contains a payment request, we are not able to detect the
        // network as that would require fetching and parsing the payment request.
        // That means clicking such an URI which contains a testnet payment request
        // will start a mainnet instance and throw a "wrong network" error.
        if (arg.startsWith(BITCOIN_IPC_PREFIX, Qt::CaseInsensitive)) // bitcoin: URI
        {
            savedPaymentRequests.append(arg);

            SendCoinsRecipient r;
            if (GUIUtil::parseBitcoinURI(arg, &r) && !r.address.isEmpty())
            {
                CBitcoinAddress address(r.address.toStdString());
                auto tempChainParams = CreateChainParams(CBaseChainParams::MAIN);

                if (address.IsValid(*tempChainParams))
                {
                    SelectParams(CBaseChainParams::MAIN);
                }
                else {
                    tempChainParams = CreateChainParams(CBaseChainParams::TESTNET);
                    if (address.IsValid(*tempChainParams))
                        SelectParams(CBaseChainParams::TESTNET);
                }
            }
        }
        else if (QFile::exists(arg)) // Filename
        {
            savedPaymentRequests.append(arg);

            PaymentRequestPlus request;
            if (readPaymentRequestFromFile(arg, request))
            {
                if (request.getDetails().network() == "main")
                {
                    SelectParams(CBaseChainParams::MAIN);
                }
                else if (request.getDetails().network() == "test")
                {
                    SelectParams(CBaseChainParams::TESTNET);
                }
            }
        }
        else
        {
            // Printing to debug.log is about the best we can do here, the
            // GUI hasn't started yet so we can't pop up a message box.
            qWarning() << "PaymentServer::ipcSendCommandLine: Payment request file does not exist: " << arg;
        }
    }
}

//
// Sending to the server is done synchronously, at startup.
// If the server isn't already running, startup continues,
// and the items in savedPaymentRequest will be handled
// when uiReady() is called.
//
bool PaymentServer::ipcSendCommandLine()
{
    bool fResult = false;
    for (const QString& r : savedPaymentRequests)
    {
        QLocalSocket* socket = new QLocalSocket();
        socket->connectToServer(ipcServerName(), QIODevice::WriteOnly);
        if (!socket->waitForConnected(BITCOIN_IPC_CONNECT_TIMEOUT))
        {
            delete socket;
            socket = nullptr;
            return false;
        }

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_0);
        out << r;
        out.device()->seek(0);

        socket->write(block);
        socket->flush();
        socket->waitForBytesWritten(BITCOIN_IPC_CONNECT_TIMEOUT);
        socket->disconnectFromServer();

        delete socket;
        socket = nullptr;
        fResult = true;
    }

    return fResult;
}

PaymentServer::PaymentServer(QObject* parent, bool startLocalServer) :
    QObject(parent),
    saveURIs(true),
    uriServer(0),
    netManager(0),
    optionsModel(0)
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Install global event filter to catch QFileOpenEvents
    // on Mac: sent when you click bitcoin: links
    // other OSes: helpful when dealing with payment request files
    if (parent)
        parent->installEventFilter(this);

    QString name = ipcServerName();

    // Clean up old socket leftover from a crash:
    QLocalServer::removeServer(name);

    if (startLocalServer)
    {
        uriServer = new QLocalServer(this);

        if (!uriServer->listen(name)) {
            // constructor is called early in init, so don't use "Q_EMIT message()" here
            QMessageBox::critical(0, tr("Payment request error"),
                tr("Cannot start bitcoin: click-to-pay handler"));
        }
        else {
            connect(uriServer, SIGNAL(newConnection()), this, SLOT(handleURIConnection()));
            connect(this, SIGNAL(receivedPaymentACK(QString)), this, SLOT(handlePaymentACK(QString)));
        }
    }
}

PaymentServer::~PaymentServer()
{
    google::protobuf::ShutdownProtobufLibrary();
}

//
// OSX-specific way of handling bitcoin: URIs and PaymentRequest mime types.
// Also used by paymentservertests.cpp and when opening a payment request file
// via "Open URI..." menu entry.
//
bool PaymentServer::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent*>(event);
        if (!fileEvent->file().isEmpty())
            handleURIOrFile(fileEvent->file());
        else if (!fileEvent->url().isEmpty())
            handleURIOrFile(fileEvent->url().toString());

        return true;
    }

    return QObject::eventFilter(object, event);
}

void PaymentServer::initNetManager()
{
    if (!optionsModel)
        return;
    if (netManager != nullptr)
        delete netManager;

    // netManager is used to fetch paymentrequests given in bitcoin: URIs
    netManager = new QNetworkAccessManager(this);

    QNetworkProxy proxy;

    // Query active SOCKS5 proxy
    if (optionsModel->getProxySettings(proxy)) {
        netManager->setProxy(proxy);

        qDebug() << "PaymentServer::initNetManager: Using SOCKS5 proxy" << proxy.hostName() << ":" << proxy.port();
    }
    else
        qDebug() << "PaymentServer::initNetManager: No active proxy server found.";

    connect(netManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(netRequestFinished(QNetworkReply*)));
    connect(netManager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
            this, SLOT(reportSslErrors(QNetworkReply*, const QList<QSslError> &)));
}

void PaymentServer::uiReady()
{
    initNetManager();

    saveURIs = false;
    for (const QString& s : savedPaymentRequests)
    {
        handleURIOrFile(s);
    }
    savedPaymentRequests.clear();
}

void PaymentServer::handleURIOrFile(const QString& s)
{
    if (saveURIs)
    {
        savedPaymentRequests.append(s);
        return;
    }

    if (s.startsWith(BITCOIN_IPC_PREFIX, Qt::CaseInsensitive)) // bitcoin: URI
    {
#if QT_VERSION < 0x050000
        QUrl uri(s);
#else
        QUrlQuery uri((QUrl(s)));
#endif
        if (uri.hasQueryItem("r")) // payment request URI
        {
            QByteArray temp;
            temp.append(uri.queryItemValue("r"));
            QString decoded = QUrl::fromPercentEncoding(temp);
            QUrl fetchUrl(decoded, QUrl::StrictMode);

            if (fetchUrl.isValid())
            {
                qDebug() << "PaymentServer::handleURIOrFile: fetchRequest(" << fetchUrl << ")";
                fetchRequest(fetchUrl);
            }
            else
            {
                qWarning() << "PaymentServer::handleURIOrFile: Invalid URL: " << fetchUrl;
                Q_EMIT message(tr("URI handling"),
                    tr("Payment request fetch URL is invalid: %1").arg(fetchUrl.toString()),
                    CClientUIInterface::ICON_WARNING);
            }

            return;
        }
        else // normal URI
        {
            SendCoinsRecipient recipient;
            if (GUIUtil::parseBitcoinURI(s, &recipient))
            {
                CBitcoinAddress address(recipient.address.toStdString());
                if (!address.IsValid()) {
                    Q_EMIT message(tr("URI handling"), tr("Invalid payment address %1").arg(recipient.address),
                        CClientUIInterface::MSG_ERROR);
                }
                else
                    Q_EMIT receivedPaymentRequest(recipient);
            }
            else
                Q_EMIT message(tr("URI handling"),
                    tr("URI cannot be parsed! This can be caused by an invalid Bitcoin address or malformed URI parameters."),
                    CClientUIInterface::ICON_WARNING);

            return;
        }
    }

    if (QFile::exists(s)) // payment request file
    {
        PaymentRequestPlus request;
        SendCoinsRecipient recipient;
        if (!readPaymentRequestFromFile(s, request))
        {
            Q_EMIT message(tr("Payment request file handling"),
                tr("Payment request file cannot be read! This can be caused by an invalid payment request file."),
                CClientUIInterface::ICON_WARNING);
        }
        else if (processPaymentRequest(request, recipient))
            Q_EMIT receivedPaymentRequest(recipient);

        return;
    }
}

void PaymentServer::handleURIConnection()
{
    QLocalSocket *clientConnection = uriServer->nextPendingConnection();

    while (clientConnection->bytesAvailable() < (int)sizeof(quint32))
        clientConnection->waitForReadyRead();

    connect(clientConnection, SIGNAL(disconnected()),
            clientConnection, SLOT(deleteLater()));

    QDataStream in(clientConnection);
    in.setVersion(QDataStream::Qt_4_0);
    if (clientConnection->bytesAvailable() < (int)sizeof(quint16)) {
        return;
    }
    QString msg;
    in >> msg;

    handleURIOrFile(msg);
}

//
// Warning: readPaymentRequestFromFile() is used in ipcSendCommandLine()
// so don't use "Q_EMIT message()", but "QMessageBox::"!
//
bool PaymentServer::readPaymentRequestFromFile(const QString& filename, PaymentRequestPlus& request)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << QString("PaymentServer::%1: Failed to open %2").arg(__func__).arg(filename);
        return false;
    }

    // BIP70 DoS protection
    if (!verifySize(f.size())) {
        return false;
    }

    QByteArray data = f.readAll();

    return request.parse(data);
}

bool PaymentServer::processPaymentRequest(const PaymentRequestPlus& request, SendCoinsRecipient& recipient)
{
    if (!optionsModel)
        return false;

    if (request.IsInitialized()) {
        // Payment request network matches client network?
        if (!verifyNetwork(request.getDetails())) {
            Q_EMIT message(tr("Payment request rejected"), tr("Payment request network doesn't match client network."),
                CClientUIInterface::MSG_ERROR);

            return false;
        }

        // Make sure any payment requests involved are still valid.
        // This is re-checked just before sending coins in WalletModel::sendCoins().
        if (verifyExpired(request.getDetails())) {
            Q_EMIT message(tr("Payment request rejected"), tr("Payment request expired."),
                CClientUIInterface::MSG_ERROR);

            return false;
        }
    } else {
        Q_EMIT message(tr("Payment request error"), tr("Payment request is not initialized."),
            CClientUIInterface::MSG_ERROR);

        return false;
    }

    recipient.paymentRequest = request;
    recipient.message = GUIUtil::HtmlEscape(request.getDetails().memo());

    request.getMerchant(certStore.get(), recipient.authenticatedMerchant);

    QList<std::pair<CScript, CAmount> > sendingTos = request.getPayTo();
    QStringList addresses;

    for (const std::pair<CScript, CAmount>& sendingTo : sendingTos) {
        // Extract and check destination addresses
        CTxDestination dest;
        if (ExtractDestination(sendingTo.first, dest)) {
            // Append destination address
            addresses.append(QString::fromStdString(CBitcoinAddress(dest).ToString()));
        }
        else if (!recipient.authenticatedMerchant.isEmpty()) {
            // Unauthenticated payment requests to custom bitcoin addresses are not supported
            // (there is no good way to tell the user where they are paying in a way they'd
            // have a chance of understanding).
            Q_EMIT message(tr("Payment request rejected"),
                tr("Unverified payment requests to custom payment scripts are unsupported."),
                CClientUIInterface::MSG_ERROR);
            return false;
        }

        // Bitcoin amounts are stored as (optional) uint64 in the protobuf messages (see paymentrequest.proto),
        // but CAmount is defined as int64_t. Because of that we need to verify that amounts are in a valid range
        // and no overflow has happened.
        if (!verifyAmount(sendingTo.second)) {
            Q_EMIT message(tr("Payment request rejected"), tr("Invalid payment request."), CClientUIInterface::MSG_ERROR);
            return false;
        }

        // Extract and check amounts
        CTxOut txOut(sendingTo.second, sendingTo.first);
        if (IsDust(txOut, ::dustRelayFee)) {
            Q_EMIT message(tr("Payment request error"), tr("Requested payment amount of %1 is too small (considered dust).")
                .arg(BitcoinUnits::formatWithUnit(optionsModel->getDisplayUnit(), sendingTo.second)),
                CClientUIInterface::MSG_ERROR);

            return false;
        }

        recipient.amount += sendingTo.second;
        // Also verify that the final amount is still in a valid range after adding additional amounts.
        if (!verifyAmount(recipient.amount)) {
            Q_EMIT message(tr("Payment request rejected"), tr("Invalid payment request."), CClientUIInterface::MSG_ERROR);
            return false;
        }
    }
    // Store addresses and format them to fit nicely into the GUI
    recipient.address = addresses.join("<br />");

    if (!recipient.authenticatedMerchant.isEmpty()) {
        qDebug() << "PaymentServer::processPaymentRequest: Secure payment request from " << recipient.authenticatedMerchant;
    }
    else {
        qDebug() << "PaymentServer::processPaymentRequest: Insecure payment request to " << addresses.join(", ");
    }

    return true;
}

void PaymentServer::fetchRequest(const QUrl& url)
{
    QNetworkRequest netRequest;
    netRequest.setAttribute(QNetworkRequest::User, BIP70_MESSAGE_PAYMENTREQUEST);
    netRequest.setUrl(url);
    netRequest.setRawHeader("User-Agent", CLIENT_NAME.c_str());
    netRequest.setRawHeader("Accept", BIP71_MIMETYPE_PAYMENTREQUEST);
    netManager->get(netRequest);
}

void PaymentServer::fetchPaymentACK(CWallet* wallet, SendCoinsRecipient recipient, QByteArray transaction)
{
    const payments::PaymentDetails& details = recipient.paymentRequest.getDetails();
    if (!details.has_payment_url())
        return;

    QNetworkRequest netRequest;
    netRequest.setAttribute(QNetworkRequest::User, BIP70_MESSAGE_PAYMENTACK);
    netRequest.setUrl(QString::fromStdString(details.payment_url()));
    netRequest.setHeader(QNetworkRequest::ContentTypeHeader, BIP71_MIMETYPE_PAYMENT);
    netRequest.setRawHeader("User-Agent", CLIENT_NAME.c_str());
    netRequest.setRawHeader("Accept", BIP71_MIMETYPE_PAYMENTACK);

    payments::Payment payment;
    payment.set_merchant_data(details.merchant_data());
    payment.add_transactions(transaction.data(), transaction.size());

    // Create a new refund address, or re-use:
    QString account = tr("Refund from %1").arg(recipient.authenticatedMerchant);
    std::string strAccount = account.toStdString();
    std::set<CTxDestination> refundAddresses = wallet->GetAccountAddresses(strAccount);
    if (!refundAddresses.empty()) {
        CScript s = GetScriptForDestination(*refundAddresses.begin());
        payments::Output* refund_to = payment.add_refund_to();
        refund_to->set_script(&s[0], s.size());
    }
    else {
        CPubKey newKey;
        if (wallet->GetKeyFromPool(newKey)) {
            CKeyID keyID = newKey.GetID();
            wallet->SetAddressBook(keyID, strAccount, "refund");

            CScript s = GetScriptForDestination(keyID);
            payments::Output* refund_to = payment.add_refund_to();
            refund_to->set_script(&s[0], s.size());
        }
        else {
            // This should never happen, because sending coins should have
            // just unlocked the wallet and refilled the keypool.
            qWarning() << "PaymentServer::fetchPaymentACK: Error getting refund key, refund_to not set";
        }
    }

    int length = payment.ByteSize();
    netRequest.setHeader(QNetworkRequest::ContentLengthHeader, length);
    QByteArray serData(length, '\0');
    if (payment.SerializeToArray(serData.data(), length)) {
        netManager->post(netRequest, serData);
    }
    else {
        // This should never happen, either.
        qWarning() << "PaymentServer::fetchPaymentACK: Error serializing payment message";
    }
}

void PaymentServer::netRequestFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    // BIP70 DoS protection
    if (!verifySize(reply->size())) {
        Q_EMIT message(tr("Payment request rejected"),
            tr("Payment request %1 is too large (%2 bytes, allowed %3 bytes).")
                .arg(reply->request().url().toString())
                .arg(reply->size())
                .arg(BIP70_MAX_PAYMENTREQUEST_SIZE),
            CClientUIInterface::MSG_ERROR);
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString msg = tr("Error communicating with %1: %2")
            .arg(reply->request().url().toString())
            .arg(reply->errorString());

        qWarning() << "PaymentServer::netRequestFinished: " << msg;
        Q_EMIT message(tr("Payment request error"), msg, CClientUIInterface::MSG_ERROR);
        return;
    }

    QByteArray data = reply->readAll();

    QString requestType = reply->request().attribute(QNetworkRequest::User).toString();
    if (requestType == BIP70_MESSAGE_PAYMENTREQUEST)
    {
        PaymentRequestPlus request;
        SendCoinsRecipient recipient;
        if (!request.parse(data))
        {
            qWarning() << "PaymentServer::netRequestFinished: Error parsing payment request";
            Q_EMIT message(tr("Payment request error"),
                tr("Payment request cannot be parsed!"),
                CClientUIInterface::MSG_ERROR);
        }
        else if (processPaymentRequest(request, recipient))
            Q_EMIT receivedPaymentRequest(recipient);

        return;
    }
    else if (requestType == BIP70_MESSAGE_PAYMENTACK)
    {
        payments::PaymentACK paymentACK;
        if (!paymentACK.ParseFromArray(data.data(), data.size()))
        {
            QString msg = tr("Bad response from server %1")
                .arg(reply->request().url().toString());

            qWarning() << "PaymentServer::netRequestFinished: " << msg;
            Q_EMIT message(tr("Payment request error"), msg, CClientUIInterface::MSG_ERROR);
        }
        else
        {
            Q_EMIT receivedPaymentACK(GUIUtil::HtmlEscape(paymentACK.memo()));
        }
    }
}

void PaymentServer::reportSslErrors(QNetworkReply* reply, const QList<QSslError> &errs)
{
    Q_UNUSED(reply);

    QString errString;
    for (const QSslError& err : errs) {
        qWarning() << "PaymentServer::reportSslErrors: " << err;
        errString += err.errorString() + "\n";
    }
    Q_EMIT message(tr("Network request error"), errString, CClientUIInterface::MSG_ERROR);
}

void PaymentServer::setOptionsModel(OptionsModel *_optionsModel)
{
    this->optionsModel = _optionsModel;
}

void PaymentServer::handlePaymentACK(const QString& paymentACKMsg)
{
    // currently we don't further process or store the paymentACK message
    Q_EMIT message(tr("Payment acknowledged"), paymentACKMsg, CClientUIInterface::ICON_INFORMATION | CClientUIInterface::MODAL);
}

bool PaymentServer::verifyNetwork(const payments::PaymentDetails& requestDetails)
{
    bool fVerified = requestDetails.network() == Params().NetworkIDString();
    if (!fVerified) {
        qWarning() << QString("PaymentServer::%1: Payment request network \"%2\" doesn't match client network \"%3\".")
            .arg(__func__)
            .arg(QString::fromStdString(requestDetails.network()))
            .arg(QString::fromStdString(Params().NetworkIDString()));
    }
    return fVerified;
}

bool PaymentServer::verifyExpired(const payments::PaymentDetails& requestDetails)
{
    bool fVerified = (requestDetails.has_expires() && (int64_t)requestDetails.expires() < GetTime());
    if (fVerified) {
        const QString requestExpires = QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M:%S", (int64_t)requestDetails.expires()));
        qWarning() << QString("PaymentServer::%1: Payment request expired \"%2\".")
            .arg(__func__)
            .arg(requestExpires);
    }
    return fVerified;
}

bool PaymentServer::verifySize(qint64 requestSize)
{
    bool fVerified = (requestSize <= BIP70_MAX_PAYMENTREQUEST_SIZE);
    if (!fVerified) {
        qWarning() << QString("PaymentServer::%1: Payment request too large (%2 bytes, allowed %3 bytes).")
            .arg(__func__)
            .arg(requestSize)
            .arg(BIP70_MAX_PAYMENTREQUEST_SIZE);
    }
    return fVerified;
}

bool PaymentServer::verifyAmount(const CAmount& requestAmount)
{
    bool fVerified = MoneyRange(requestAmount);
    if (!fVerified) {
        qWarning() << QString("PaymentServer::%1: Payment request amount out of allowed range (%2, allowed 0 - %3).")
            .arg(__func__)
            .arg(requestAmount)
            .arg(MAX_MONEY);
    }
    return fVerified;
}

X509_STORE* PaymentServer::getCertStore()
{
    return certStore.get();
}
