// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <qt/optionsdialog.h>
#include <qt/forms/ui_optionsdialog.h>

#include <qt/bitcoinamountfield.h>
#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>

#include <common/args.h>
#include <common/system.h>
#include <consensus/consensus.h> // for MAX_BLOCK_SERIALIZED_SIZE
#include <index/blockfilterindex.h>
#include <interfaces/node.h>
#include <netbase.h>
#include <node/caches.h>
#include <node/chainstatemanager_args.h>
#include <node/mempool_args.h> // for ParseDustDynamicOpt
#include <outputtype.h>
#include <primitives/transaction.h> // for WITNESS_SCALE_FACTOR
#include <txmempool.h> // for maxmempoolMinimum
#include <util/check.h>
#include <util/strencodings.h>
#include <chrono>
#include <utility>

#include <QApplication>
#include <QBoxLayout>
#include <QDataWidgetMapper>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFontDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QIntValidator>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpacerItem>
#include <QString>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

ModScrollArea::ModScrollArea()
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setObjectName(QStringLiteral("scroll"));
    setStyleSheet("QScrollArea#scroll, QScrollArea#scroll > QWidget > QWidget { background: transparent; } QScrollArea#scroll > QWidget > QScrollBar { background: palette(base); }");
}

ModScrollArea *ModScrollArea::fromWidget(QWidget * const parent, QWidget * const o)
{
    auto * const scroll = new ModScrollArea;
    scroll->setWidget(o);
    return scroll;
}

QSize ModScrollArea::minimumSizeHint() const
{
    auto w = widget()->minimumSizeHint().width();
    w += verticalScrollBar()->sizeHint().width();
    const auto h = fontMetrics().height() * 2;
    return QSize(w, h);
}

QSize ModScrollArea::sizeHint() const
{
    QSize sz = widget()->sizeHint();
    sz.rwidth() += verticalScrollBar()->sizeHint().width();
    return sz;
}

void OptionsDialog::FixTabOrder(QWidget * const o)
{
    BitcoinAmountField * const af = qobject_cast<BitcoinAmountField *>(o);
    if (af) {
        prevwidget = af->setupTabChain(prevwidget);
    } else {
        setTabOrder(prevwidget, o);
        prevwidget = o;
    }
}

struct CreateOptionUIOpts {
    QBoxLayout *horizontal_layout{nullptr};
    int stretch{1};
    int indent{0};
};

void OptionsDialog::CreateOptionUI(QBoxLayout * const layout, const QString& text, const std::vector<QWidget *>& objs, const CreateOptionUIOpts& opts)
{
    Assert(!objs.empty());

    auto& first_o = objs[0];
    QWidget * const parent = first_o->parentWidget();

    QBoxLayout * const horizontalLayout = opts.horizontal_layout ? opts.horizontal_layout : (new QHBoxLayout);

    if (opts.indent) horizontalLayout->addSpacing(opts.indent);

    int processed{0}, index_start{0};
    QWidget *last_widget{nullptr};
    while (true) {
        int pos = text.indexOf('%', index_start);
        int idx;
        if (pos == -1) {
            pos = text.size();
            idx = -1;
        } else {
            const int pos_next{pos + 1};
            const auto char_next = text[pos_next];
            idx = (char_next == 's') ? 0 : (char_next.digitValue() - 1);
            if (pos_next == text.size() || idx < 0 || idx > 8 || (unsigned)idx >= objs.size()) {
                index_start = pos_next;
                continue;
            }
        }
        if (processed != pos) {
            auto label_text = text.mid(processed, pos - processed);
            if (auto last_widget_as_qcheckbox = qobject_cast<QCheckBox*>(last_widget)) {
                if (label_text[0].isSpace()) label_text = label_text.mid(1);
                last_widget_as_qcheckbox->setText(label_text);
            } else {
                const auto label = new QLabel(parent);
                label->setText(label_text);
                label->setTextFormat(Qt::PlainText);
                label->setBuddy(first_o);
                label->setToolTip(first_o->toolTip());
                horizontalLayout->addWidget(label);
            }
        }
        if (idx == -1) break;
        last_widget = objs[idx];
        horizontalLayout->addWidget(last_widget);
        index_start = processed = pos + 2;
    }

    if (opts.stretch) horizontalLayout->addStretch(opts.stretch);

    layout->addLayout(horizontalLayout);

    for (auto& o : objs) {
        o->setProperty("L", QVariant::fromValue((QLayout*)horizontalLayout));
        FixTabOrder(o);
    }
}

void OptionsDialog::CreateOptionUI(QBoxLayout * const layout, const QString& text, const std::vector<QWidget *>& objs)
{
    CreateOptionUI(layout, text, objs, {});
}

void OptionsDialog::CreateOptionUI(QBoxLayout * const layout, QWidget * const o, const QString& text, QBoxLayout *horizontalLayout)
{
    CreateOptionUI(layout, text, {o}, { .horizontal_layout = horizontalLayout, });
}

static void setSiblingsEnabled(QWidget * const o, const bool state)
{
    auto layout = o->property("L").value<QLayout*>();
    Assert(layout);
    // NOTE: QLayout::children does not do what we need here
    for (int i = layout->count(); i-- > 0; ) {
        QLayoutItem * const layoutitem = layout->itemAt(i);
        QWidget * const childwidget = layoutitem->widget();
        if (!childwidget) continue;
        childwidget->setEnabled(state);
    }
}

int setFontChoice(QComboBox* cb, const OptionsModel::FontChoice& fc)
{
    int i;
    for (i = cb->count(); --i >= 0; ) {
        QVariant item_data = cb->itemData(i);
        if (!item_data.canConvert<OptionsModel::FontChoice>()) continue;
        if (item_data.value<OptionsModel::FontChoice>() == fc) {
            break;
        }
    }
    if (i == -1) {
        // New item needed
        QFont chosen_font = OptionsModel::getFontForChoice(fc);
        QSignalBlocker block_currentindexchanged_signal(cb);  // avoid triggering QFontDialog
        cb->insertItem(0, QFontInfo(chosen_font).family(), QVariant::fromValue(fc));
        i = 0;
    }

    cb->setCurrentIndex(i);
    return i;
}

void setupFontOptions(QComboBox* cb, QLabel* preview)
{
    QFont embedded_font{GUIUtil::fixedPitchFont(true)};
    QFont system_font{GUIUtil::fixedPitchFont(false)};
    cb->addItem(QObject::tr("%1").arg(QFontInfo(embedded_font).family()), QVariant::fromValue(OptionsModel::FontChoice{OptionsModel::FontChoiceAbstract::EmbeddedFont}));
    cb->addItem(QObject::tr("Default system font \"%1\"").arg(QFontInfo(system_font).family()), QVariant::fromValue(OptionsModel::FontChoice{OptionsModel::FontChoiceAbstract::BestSystemFont}));
    cb->addItem(QObject::tr("Custom…"));

    const auto& on_font_choice_changed = [cb, preview](int index) {
        static int previous_index = -1;
        QVariant item_data = cb->itemData(index);
        QFont f;
        if (item_data.canConvert<OptionsModel::FontChoice>()) {
            f = OptionsModel::getFontForChoice(item_data.value<OptionsModel::FontChoice>());
        } else {
            bool ok;
            f = QFontDialog::getFont(&ok, GUIUtil::fixedPitchFont(false), cb->parentWidget());
            if (!ok) {
                cb->setCurrentIndex(previous_index);
                return;
            }
            index = setFontChoice(cb, OptionsModel::FontChoice{f});
        }
        if (preview) {
            preview->setFont(f);
        }
        previous_index = index;
    };
    QObject::connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), on_font_choice_changed);
    on_font_choice_changed(cb->currentIndex());
}

OptionsDialog::OptionsDialog(QWidget* parent, bool enableWallet)
    : QDialog(parent, GUIUtil::dialog_flags | Qt::WindowMaximizeButtonHint),
      ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);

    ui->verticalLayout->setStretchFactor(ui->tabWidget, 1);

    /* Main elements init */
    ui->databaseCache->setRange(MIN_DB_CACHE >> 20, std::numeric_limits<int>::max());
    ui->threadsScriptVerif->setMinimum(-GetNumCores());
    ui->threadsScriptVerif->setMaximum(MAX_SCRIPTCHECK_THREADS);
    ui->pruneWarning->setVisible(false);
    ui->pruneWarning->setStyleSheet("QLabel { color: red; }");

    ui->pruneSizeMiB->setEnabled(false);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 7, 0))
    connect(ui->prune, &QCheckBox::checkStateChanged, [this](const Qt::CheckState state){
#else
    connect(ui->prune, &QCheckBox::stateChanged, [this](const int state){
#endif
        ui->pruneSizeMiB->setEnabled(state == Qt::Checked);
    });

    ui->networkPort->setValidator(new QIntValidator(1024, 65535, this));
    connect(ui->networkPort, SIGNAL(textChanged(const QString&)), this, SLOT(checkLineEdit()));

    /* Network elements init */
#ifndef USE_UPNP
    ui->mapPortUpnp->setEnabled(false);
#endif

    ui->proxyIp->setEnabled(false);
    ui->proxyPort->setEnabled(false);
    ui->proxyPort->setValidator(new QIntValidator(1, 65535, this));

    ui->proxyIpTor->setEnabled(false);
    ui->proxyPortTor->setEnabled(false);
    ui->proxyPortTor->setValidator(new QIntValidator(1, 65535, this));

    connect(ui->connectSocks, &QPushButton::toggled, ui->proxyIp, &QWidget::setEnabled);
    connect(ui->connectSocks, &QPushButton::toggled, ui->proxyPort, &QWidget::setEnabled);
    connect(ui->connectSocks, &QPushButton::toggled, this, &OptionsDialog::updateProxyValidationState);

    connect(ui->connectSocksTor, &QPushButton::toggled, ui->proxyIpTor, &QWidget::setEnabled);
    connect(ui->connectSocksTor, &QPushButton::toggled, ui->proxyPortTor, &QWidget::setEnabled);
    connect(ui->connectSocksTor, &QPushButton::toggled, this, &OptionsDialog::updateProxyValidationState);

    ui->maxuploadtarget->setMinimum(144 /* MiB/day */);
    ui->maxuploadtarget->setMaximum(std::numeric_limits<int>::max());
    connect(ui->maxuploadtargetCheckbox, SIGNAL(toggled(bool)), ui->maxuploadtarget, SLOT(setEnabled(bool)));

    prevwidget = ui->tabWidget;

    walletrbf = new QCheckBox(ui->tabWallet);
    walletrbf->setText(tr("Request Replace-By-Fee"));
    walletrbf->setToolTip(tr("Indicates that the sender may wish to replace this transaction with a new one paying higher fees (prior to being confirmed). Can be overridden per send."));
    ui->verticalLayout_Wallet->insertWidget(0, walletrbf);
    FixTabOrder(walletrbf);

    /* Network tab */
    QLayoutItem *spacer = ui->verticalLayout_Network->takeAt(ui->verticalLayout_Network->count() - 1);
    prevwidget = dynamic_cast<QWidgetItem*>(ui->verticalLayout_Network->itemAt(ui->verticalLayout_Network->count() - 1))->widget();

    blockreconstructionextratxn = new QSpinBox(ui->tabNetwork);
    blockreconstructionextratxn->setMinimum(0);
    blockreconstructionextratxn->setMaximum(std::numeric_limits<int>::max());
    CreateOptionUI(ui->verticalLayout_Network, blockreconstructionextratxn, tr("Keep at most %s extra transactions in memory for compact block reconstruction"));

    blockreconstructionextratxnsize = new QDoubleSpinBox(ui->tabNetwork);
    blockreconstructionextratxnsize->setDecimals(0);
    blockreconstructionextratxnsize->setMinimum(0);
    blockreconstructionextratxnsize->setMaximum(std::numeric_limits<size_t>::max() / 1'000'000);
    CreateOptionUI(ui->verticalLayout_Network, blockreconstructionextratxnsize, tr("Limit extra transactions for compact block reconstruction to %s MB"));

    ui->verticalLayout_Network->addItem(spacer);

    prevwidget = ui->peerbloomfilters;

    /* Mempool tab */

    QWidget * const tabMempool = new QWidget();
    QVBoxLayout * const verticalLayout_Mempool = new QVBoxLayout(tabMempool);
    ui->tabWidget->insertTab(ui->tabWidget->indexOf(ui->tabWindow), tabMempool, tr("Mem&pool"));

    mempoolreplacement = new QValueComboBox(tabMempool);
    mempoolreplacement->addItem(QString("never"), QVariant("never"));
    mempoolreplacement->addItem(QString("with a higher mining fee, and opt-in"), QVariant("fee,optin"));
    mempoolreplacement->addItem(QString("with a higher mining fee (no opt-out)"), QVariant("fee,-optin"));
    CreateOptionUI(verticalLayout_Mempool, mempoolreplacement, tr("Transaction &replacement: %s"));

    incrementalrelayfee = new BitcoinAmountField(tabMempool);
    connect(incrementalrelayfee, SIGNAL(valueChanged()), this, SLOT(incrementalrelayfee_changed()));
    CreateOptionUI(verticalLayout_Mempool, incrementalrelayfee, tr("Require transaction fees to be at least %s per kvB higher than transactions they are replacing."));

    rejectspkreuse = new QCheckBox(tabMempool);
    rejectspkreuse->setText(tr("Disallow most address reuse"));
    rejectspkreuse->setToolTip(tr("With this option enabled, your memory pool will only allow each unique payment destination to be used once, effectively deprioritising address reuse. Address reuse is not technically supported, and harms the privacy of all Bitcoin users. It also has limited real-world utility, and has been known to be common with spam."));
    verticalLayout_Mempool->addWidget(rejectspkreuse);
    FixTabOrder(rejectspkreuse);

    mempooltruc = new QValueComboBox(tabMempool);
    mempooltruc->addItem(QString("do not relay or mine at all"), QVariant("reject"));
    mempooltruc->addItem(QString("handle the same as other transactions"), QVariant("accept"));
    mempooltruc->addItem(QString("impose stricter limits requested"), QVariant("enforce"));
    mempooltruc->setToolTip(tr("Some transactions signal a request to limit both themselves and other related transactions to more restrictive expectations. Specifically, this would disallow more than 1 unconfirmed predecessor or spending transaction, as well as smaller size limits (see BIP 431 for details), regardless of what policy you have configured."));
    CreateOptionUI(verticalLayout_Mempool, mempooltruc, tr("Transactions requesting more restrictive policy limits (TRUC): %s"));

    maxorphantx = new QSpinBox(tabMempool);
    maxorphantx->setMinimum(0);
    maxorphantx->setMaximum(std::numeric_limits<int>::max());
    CreateOptionUI(verticalLayout_Mempool, maxorphantx, tr("Keep at most %s unconnected transactions in memory"));

    maxmempool = new QSpinBox(tabMempool);
    const int64_t nMempoolSizeMinMB = maxmempoolMinimumBytes(gArgs.GetIntArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT_KVB) * 1'000) / 1'000'000;
    maxmempool->setMinimum(nMempoolSizeMinMB);
    maxmempool->setMaximum(std::numeric_limits<int>::max());
    CreateOptionUI(verticalLayout_Mempool, maxmempool, tr("Keep the transaction memory pool below %s MB"));

    mempoolexpiry = new QSpinBox(tabMempool);
    mempoolexpiry->setMinimum(1);
    mempoolexpiry->setMaximum(std::numeric_limits<int>::max());
    CreateOptionUI(verticalLayout_Mempool, mempoolexpiry, tr("Do not keep transactions in memory more than %s hours"));

    verticalLayout_Mempool->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    /* Filters tab */

    QWidget * const tabFilters = new QWidget();
    auto& groupBox_Spamfiltering = tabFilters;
    ui->tabWidget->insertTab(ui->tabWidget->indexOf(ui->tabWindow), ModScrollArea::fromWidget(this, groupBox_Spamfiltering), tr("Spam &filtering"));
    QVBoxLayout * const verticalLayout_Spamfiltering = new QVBoxLayout(groupBox_Spamfiltering);

    rejectunknownscripts = new QCheckBox(groupBox_Spamfiltering);
    rejectunknownscripts->setText(tr("Ignore unrecognised receiver scripts"));
    rejectunknownscripts->setToolTip(tr("With this option enabled, unrecognised receiver (\"pubkey\") scripts will be ignored. Unrecognisable scripts could be used to bypass further spam filters. If your software is outdated, they may also be used to trick you into thinking you were sent bitcoins that will never confirm."));
    verticalLayout_Spamfiltering->addWidget(rejectunknownscripts);
    FixTabOrder(rejectunknownscripts);

    rejectunknownwitness = new QCheckBox(groupBox_Spamfiltering);
    rejectunknownwitness->setText(tr("Reject unknown witness script versions"));
    rejectunknownwitness->setToolTip(tr("Some attempts to spam Bitcoin intentionally use undefined witness script formats reserved for future use. By enabling this option, your node will reject transactions using these undefined/future versions. Note that if you send to many addressses in a single transaction, the entire transaction may be rejected if any single one of them attempts to use an undefined format."));
    verticalLayout_Spamfiltering->addWidget(rejectunknownwitness);
    FixTabOrder(rejectunknownwitness);

    rejectparasites = new QCheckBox(groupBox_Spamfiltering);
    rejectparasites->setText(tr("Reject parasite transactions"));
    rejectparasites->setToolTip(tr("With this option enabled, transactions related to parasitic overlay protocols will be ignored. Parasites are transactions using Bitcoin as a technical infrastructure to animate other protocols, unrelated to ordinary money transfers."));
    verticalLayout_Spamfiltering->addWidget(rejectparasites);
    FixTabOrder(rejectparasites);

    rejecttokens = new QCheckBox(groupBox_Spamfiltering);
    rejecttokens->setText(tr("Ignore transactions involving non-bitcoin token/asset overlay protocols"));
    rejecttokens->setToolTip(tr("With this option enabled, transactions involving non-bitcoin tokens/assets will not be relayed or mined by your node. Due to not having value, and some technical design flaws, token mints and transfers are often spammy and can bog down the network."));
    verticalLayout_Spamfiltering->addWidget(rejecttokens);
    FixTabOrder(rejecttokens);

    minrelaytxfee = new BitcoinAmountField(groupBox_Spamfiltering);
    CreateOptionUI(verticalLayout_Spamfiltering, minrelaytxfee, tr("Ignore transactions offering miners less than %s per kvB in transaction fees."));

    minrelaycoinblocks = new BitcoinAmountField(groupBox_Spamfiltering);
    minrelaycoinblocks->SetMaxValue(std::numeric_limits<CAmount>::max());
    minrelaycoinblocks->setToolTip(tr("This effectively acts as a rate limit. When bitcoins are spent, they reset to zero \"coinblocks\" (aka coin age) and slowly build up more coinblocks based on their value each block afterward. Small coins take longer than large amounts."));
    CreateOptionUI(verticalLayout_Spamfiltering, minrelaycoinblocks, tr("Delay accepting transactions spending coins that have been at rest less than %s per block."));

    minrelaymaturity = new QSpinBox(groupBox_Spamfiltering);
    minrelaymaturity->setMinimum(0);
    minrelaymaturity->setMaximum(std::numeric_limits<int>::max());
    minrelaymaturity->setToolTip(tr("This effectively acts as a rate limit. When bitcoins are spent, they reset to zero blocks and slowly mature each block afterward, regardless of their value."));
    CreateOptionUI(verticalLayout_Spamfiltering, minrelaymaturity, tr("Delay accepting transactions spending coins that have been at rest fewer than %s blocks."));

    bytespersigop = new QSpinBox(groupBox_Spamfiltering);
    bytespersigop->setMinimum(1);
    bytespersigop->setMaximum(std::numeric_limits<int>::max());
    CreateOptionUI(verticalLayout_Spamfiltering, bytespersigop, tr("Treat each consensus-counted sigop as at least %s bytes."));

    bytespersigopstrict = new QSpinBox(groupBox_Spamfiltering);
    bytespersigopstrict->setMinimum(1);
    bytespersigopstrict->setMaximum(std::numeric_limits<int>::max());
    CreateOptionUI(verticalLayout_Spamfiltering, bytespersigopstrict, tr("Ignore transactions with fewer than %s bytes per potentially-executed sigop."));

    limitancestorcount = new QSpinBox(groupBox_Spamfiltering);
    limitancestorcount->setMinimum(1);
    limitancestorcount->setMaximum(std::numeric_limits<int>::max());
    CreateOptionUI(verticalLayout_Spamfiltering, limitancestorcount, tr("Ignore transactions with %s or more unconfirmed ancestors."));

    limitancestorsize = new QSpinBox(groupBox_Spamfiltering);
    limitancestorsize->setMinimum(1);
    limitancestorsize->setMaximum(std::numeric_limits<int>::max());
    CreateOptionUI(verticalLayout_Spamfiltering, limitancestorsize, tr("Ignore transactions whose size with all unconfirmed ancestors exceeds %s kilobytes."));

    limitdescendantcount = new QSpinBox(groupBox_Spamfiltering);
    limitdescendantcount->setMinimum(1);
    limitdescendantcount->setMaximum(std::numeric_limits<int>::max());
    CreateOptionUI(verticalLayout_Spamfiltering, limitdescendantcount, tr("Ignore transactions if any ancestor would have %s or more unconfirmed descendants."));

    limitdescendantsize = new QSpinBox(groupBox_Spamfiltering);
    limitdescendantsize->setMinimum(1);
    limitdescendantsize->setMaximum(std::numeric_limits<int>::max());
    CreateOptionUI(verticalLayout_Spamfiltering, limitdescendantsize, tr("Ignore transactions if any ancestor would have more than %s kilobytes of unconfirmed descendants."));

    rejectbarepubkey = new QCheckBox(groupBox_Spamfiltering);
    rejectbarepubkey->setText(tr("Ignore bare/exposed public keys (pay-to-IP)"));
    rejectbarepubkey->setToolTip(tr("Spam is sometimes disguised to appear as if it is a deprecated pay-to-IP (bare pubkey) transaction, where the \"key\" is actually arbitrary data (not a real key) instead. Support for pay-to-IP was only ever supported by Satoshi's early Bitcoin wallet, which has been abandoned since 2011."));
    verticalLayout_Spamfiltering->addWidget(rejectbarepubkey);
    FixTabOrder(rejectbarepubkey);

    rejectbaremultisig = new QCheckBox(groupBox_Spamfiltering);
    rejectbaremultisig->setText(tr("Ignore bare/exposed \"multisig\" scripts"));
    rejectbaremultisig->setToolTip(tr("Spam is sometimes disguised to appear as if it is an old-style N-of-M multi-party transaction, where most of the keys are really bogus. At the same time, legitimate multi-party transactions typically have always used P2SH format (which is not filtered by this option), which is more secure."));
    verticalLayout_Spamfiltering->addWidget(rejectbaremultisig);
    FixTabOrder(rejectbaremultisig);

    permitephemeral = new QValueComboBox(tabMempool);
    permitephemeral->addItem(QString("(no exception allowed)"), QVariant("reject"));
    permitephemeral->addItem(QString("anchor (recommended)"), QVariant("anchor,-send,-dust"));
    permitephemeral->addItem(QString("zero-value anchor/send"), QVariant("anchor,send,-dust"));
    permitephemeral->addItem(QString("zero-value send-only"), QVariant("-anchor,send,-dust"));
    permitephemeral->addItem(QString("dust send"), QVariant("-anchor,send,dust"));
    permitephemeral->addItem(QString("dust"), QVariant("anchor,send,dust"));
    permitephemeral->addItem(QString("dust anchor"), QVariant("anchor,-send,dust"));
    permitephemeral->setToolTip(tr("For some smart contracts, it is impractical to increase the fee after the transaction is created. For this reason, they may use zero-value \"anchors\" to chain two transactions together, the subsequent transaction simply covering the fee for both. Ordinarily, these anchors might be rejected as dust, so it may make sense to make an exception when they are sent together. Variants of this can however be abused for anti-fungibility attacks and possibly spam."));
    CreateOptionUI(verticalLayout_Spamfiltering, permitephemeral, tr("Allow transactions to have at most one ephemeral %s output"));

    rejectbareanchor = new QCheckBox(groupBox_Spamfiltering);
    rejectbareanchor->setText(tr("Reject transactions that only have an anchor"));
    rejectbareanchor->setToolTip(tr("Anchors are a way to allow fee-bumping smart contract transactions long after they have been created. With this option set, your node will refuse to relay or mine transactions that have only an anchor but no real sends."));
    verticalLayout_Spamfiltering->addWidget(rejectbareanchor);
    FixTabOrder(rejectbareanchor);

    maxscriptsize = new QSpinBox(groupBox_Spamfiltering);
    maxscriptsize->setMinimum(0);
    maxscriptsize->setMaximum(std::numeric_limits<int>::max());
    maxscriptsize->setToolTip(tr("There may be rare smart contracts that require a large amount of code, but more often a larger code segment is actually just spam finding new ways to try to evade filtering. 1650 bytes is sometimes considered the high end of what might be normal, usually for N-of-20 multisig."));
    CreateOptionUI(verticalLayout_Spamfiltering, maxscriptsize, tr("Ignore transactions with smart contract code larger than %s bytes."));

    maxtxlegacysigops = new QSpinBox(groupBox_Spamfiltering);
    maxtxlegacysigops->setMinimum(1);
    maxtxlegacysigops->setMaximum(1000000);
    maxtxlegacysigops->setToolTip(tr("Each signature operation in scripts to spend pre-segwit coins require calculations to be performed on the entire transaction. These \"legacy sigops\" can add up quickly, and there is typically only one per coin spent."));
    CreateOptionUI(verticalLayout_Spamfiltering, maxtxlegacysigops, tr("Ignore transactions with more than %s \"legacy\" signature operations."));

    datacarriersize = new QSpinBox(groupBox_Spamfiltering);
    datacarriersize->setMinimum(0);
    datacarriersize->setMaximum(std::numeric_limits<int>::max());
    datacarriersize->setToolTip(tr("While Bitcoin itself does not support attaching arbitrary data to transactions, despite that various methods for disguising it have been devised over the years. Since it is sometimes impractical to detect small spam disguised as ordinary transactions, it is sometimes considered beneficial to tolerate certain kinds of less harmful data attachments."));
    CreateOptionUI(verticalLayout_Spamfiltering, datacarriersize, tr("Ignore transactions with additional data larger than %s bytes."));

    datacarriercost = new QDoubleSpinBox(groupBox_Spamfiltering);
    datacarriercost->setDecimals(2);
    datacarriercost->setStepType(QAbstractSpinBox::DefaultStepType);
    datacarriercost->setSingleStep(0.25);
    datacarriercost->setMinimum(0.25);
    datacarriercost->setMaximum(MAX_BLOCK_SERIALIZED_SIZE);
    datacarriercost->setToolTip(tr("As an alternative to, or in addition to, limiting the size of disguised data, you can also configure how it is accounted for in comparison to legitimate transaction data. For example, 1 vbyte per actual byte would count it as equivalent to ordinary transaction data; 0.25 vB/B would allow it to benefit from the so-called \"segwit discount\"; or 2 vB/B would establish a bias toward legitimate transactions."));
    CreateOptionUI(verticalLayout_Spamfiltering, datacarriercost, tr("Weigh embedded data as %s virtual bytes per actual byte."));
    connect(datacarriercost, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double d){
        const double w = d * 4;
        const double wf = floor(w);
        if (w != wf) datacarriercost->setValue(wf / 4);
    });

    rejectnonstddatacarrier = new QCheckBox(groupBox_Spamfiltering);
    rejectnonstddatacarrier->setText(tr("Ignore data embedded with non-standard formats"));
    rejectnonstddatacarrier->setToolTip(tr("Some attempts to spam Bitcoin intentionally use non-standard formats in an attempt to bypass the datacarrier limits. Without this option, %1 will attempt to detect these and enforce the intended limits. By enabling this option, your node will ignore these transactions entirely (when detected) even if they fall within the configured limits otherwise."));
    verticalLayout_Spamfiltering->addWidget(rejectnonstddatacarrier);
    FixTabOrder(rejectnonstddatacarrier);

    dustrelayfee = new BitcoinAmountField(groupBox_Spamfiltering);
    CreateOptionUI(verticalLayout_Spamfiltering, dustrelayfee, tr("Ignore transactions with values that would cost more to spend at a fee rate of %s per kvB (\"dust\")."));

    rejectbaredatacarrier = new QCheckBox(groupBox_Spamfiltering);
    rejectbaredatacarrier->setText(tr("Reject \"transactions\" that are only arbitrary data"));
    rejectbaredatacarrier->setToolTip(tr("With this option set, arbitrary data will only be permitted as defined above in addition to an otherwise-valid transaction. If there are no real recipients, the transaction will be rejected no matter how little data it includes."));
    verticalLayout_Spamfiltering->addWidget(rejectbaredatacarrier);
    FixTabOrder(rejectbaredatacarrier);


    dustdynamic_enable = new QCheckBox(groupBox_Spamfiltering);
    dustdynamic_multiplier = new QDoubleSpinBox(groupBox_Spamfiltering);
    dustdynamic_multiplier->setDecimals(3);
    dustdynamic_multiplier->setStepType(QAbstractSpinBox::DefaultStepType);
    dustdynamic_multiplier->setSingleStep(1);
    dustdynamic_multiplier->setMinimum(0.001);
    dustdynamic_multiplier->setMaximum(65);
    dustdynamic_multiplier->setValue(DEFAULT_DUST_RELAY_MULTIPLIER / 1000.0);
    CreateOptionUI(verticalLayout_Spamfiltering, tr("%1 Automatically adjust the dust limit upward to %2 times:"), {dustdynamic_enable, dustdynamic_multiplier});

    QStyleOptionButton styleoptbtn;
    const auto checkbox_indent = dustdynamic_enable->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &styleoptbtn, dustdynamic_enable).width();

    dustdynamic_target = new QRadioButton(groupBox_Spamfiltering);
    dustdynamic_target_blocks = new QSpinBox(groupBox_Spamfiltering);
    dustdynamic_target_blocks->setMinimum(2);
    dustdynamic_target_blocks->setMaximum(1008);  // FIXME: Get this from the fee estimator
    dustdynamic_target_blocks->setValue(1008);
    CreateOptionUI(verticalLayout_Spamfiltering, tr("%1 fee estimate for %2 blocks."), {dustdynamic_target, dustdynamic_target_blocks}, { .indent = checkbox_indent, });
    // FIXME: Make it possible to click labels to select + focus spinbox

    dustdynamic_mempool = new QRadioButton(groupBox_Spamfiltering);
    dustdynamic_mempool_kvB = new QSpinBox(groupBox_Spamfiltering);
    dustdynamic_mempool_kvB->setMinimum(1);
    dustdynamic_mempool_kvB->setMaximum(std::numeric_limits<int32_t>::max());
    dustdynamic_mempool_kvB->setValue(3024000);
    CreateOptionUI(verticalLayout_Spamfiltering, tr("%1 the lowest fee of the best known %2 kvB of unconfirmed transactions."), {dustdynamic_mempool, dustdynamic_mempool_kvB}, { .indent = checkbox_indent, });

    const auto dustdynamic_enable_toggled = [this](const bool state){
        dustdynamic_multiplier->setEnabled(state);
        setSiblingsEnabled(dustdynamic_target_blocks, state);
        setSiblingsEnabled(dustdynamic_mempool_kvB, state);
        if (state) {
            if (!dustdynamic_mempool->isChecked()) dustdynamic_target->setChecked(true);
            dustdynamic_target_blocks->setEnabled(dustdynamic_target->isChecked());
            dustdynamic_mempool_kvB->setEnabled(dustdynamic_mempool->isChecked());
        }
    };
    connect(dustdynamic_enable, &QAbstractButton::toggled, dustdynamic_enable_toggled);
    dustdynamic_enable_toggled(dustdynamic_enable->isChecked());
    connect(dustdynamic_target, &QAbstractButton::toggled, [this](const bool state){
        dustdynamic_target_blocks->setEnabled(state);
    });
    connect(dustdynamic_mempool, &QAbstractButton::toggled, [this](const bool state){
        dustdynamic_mempool_kvB->setEnabled(state);
    });


    connect(rejectunknownscripts, &QAbstractButton::toggled, [this, dustdynamic_enable_toggled](const bool state){
        rejectunknownwitness->setEnabled(state);
        rejectbarepubkey->setEnabled(state);
        rejectbaremultisig->setEnabled(state);
        permitephemeral->setEnabled(state);
        rejectbareanchor->setEnabled(state);
        rejectbaredatacarrier->setEnabled(state);
        rejectparasites->setEnabled(state);
        rejecttokens->setEnabled(state);
        setSiblingsEnabled(dustrelayfee, state);
        setSiblingsEnabled(maxscriptsize, state);
        setSiblingsEnabled(maxtxlegacysigops, state);
        setSiblingsEnabled(dustdynamic_multiplier, state);
        dustdynamic_enable_toggled(state && dustdynamic_enable->isChecked());
    });


    verticalLayout_Spamfiltering->addStretch(1);

    /* Mining tab */

    QWidget * const tabMining = new QWidget();
    QVBoxLayout * const verticalLayout_Mining = new QVBoxLayout(tabMining);
    ui->tabWidget->insertTab(ui->tabWidget->indexOf(ui->tabWindow), tabMining, tr("M&ining"));

    verticalLayout_Mining->addWidget(new QLabel(tr("<strong>Note that mining is heavily influenced by the settings on the Mempool and Spam filtering tabs.</strong>")));

    blockmintxfee = new BitcoinAmountField(tabMining);
    CreateOptionUI(verticalLayout_Mining, blockmintxfee, tr("Only mine transactions paying a fee of at least %s per kvB."));

    blockmaxsize = new QSpinBox(tabMining);
    blockmaxsize->setMinimum(1);
    blockmaxsize->setMaximum((MAX_BLOCK_SERIALIZED_SIZE - 1000) / 1000);
    connect(blockmaxsize, SIGNAL(valueChanged(int)), this, SLOT(blockmaxsize_changed(int)));
    CreateOptionUI(verticalLayout_Mining, blockmaxsize, tr("Never mine a block larger than %s kB."));

    blockprioritysize = new QSpinBox(tabMining);
    blockprioritysize->setMinimum(0);
    blockprioritysize->setMaximum(blockmaxsize->maximum());
    connect(blockprioritysize, SIGNAL(valueChanged(int)), this, SLOT(blockmaxsize_increase(int)));
    CreateOptionUI(verticalLayout_Mining, blockprioritysize, tr("Mine first %s kB of transactions sorted by coin-age priority."));

    blockmaxweight = new QSpinBox(tabMining);
    blockmaxweight->setMinimum(1);
    blockmaxweight->setMaximum((MAX_BLOCK_WEIGHT-4000) / 1000);
    connect(blockmaxweight, SIGNAL(valueChanged(int)), this, SLOT(blockmaxweight_changed(int)));
    CreateOptionUI(verticalLayout_Mining, blockmaxweight, tr("Never mine a block weighing more than %s kWU."));

    verticalLayout_Mining->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    /* Window elements init */
#ifdef Q_OS_MACOS
    /* remove Window tab on Mac */
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tabWindow));
    /* hide launch at startup option on macOS */
    ui->bitcoinAtStartup->setVisible(false);
    ui->verticalLayout_Main->removeWidget(ui->bitcoinAtStartup);
    ui->verticalLayout_Main->removeItem(ui->horizontalSpacer_0_Main);
#endif

    /* remove Wallet tab and 3rd party-URL textbox in case of -disablewallet */
    if (!enableWallet) {
        ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tabWallet));
        ui->thirdPartyTxUrlsLabel->setVisible(false);
        ui->thirdPartyTxUrls->setVisible(false);
    } else {
        for (OutputType type : OUTPUT_TYPES) {
            const QString& val = QString::fromStdString(FormatOutputType(type));
            const auto [text, tooltip] = GetOutputTypeDescription(type);

            const auto index = ui->addressType->count();
            ui->addressType->addItem(text, val);
            ui->addressType->setItemData(index, tooltip, Qt::ToolTipRole);
        }
    }

#ifdef ENABLE_EXTERNAL_SIGNER
    ui->externalSignerPath->setToolTip(ui->externalSignerPath->toolTip().arg(CLIENT_NAME));
#else
    //: "External signing" means using devices such as hardware wallets.
    ui->externalSignerPath->setToolTip(tr("Compiled without external signing support (required for external signing)"));
    ui->externalSignerPath->setEnabled(false);
#endif
    /* Display elements init */
    QDir translations(":translations");

    ui->bitcoinAtStartup->setToolTip(ui->bitcoinAtStartup->toolTip().arg(CLIENT_NAME));
    ui->bitcoinAtStartup->setText(ui->bitcoinAtStartup->text().arg(CLIENT_NAME));

    ui->openBitcoinConfButton->setToolTip(ui->openBitcoinConfButton->toolTip().arg(CLIENT_NAME));

    ui->lang->setToolTip(ui->lang->toolTip().arg(CLIENT_NAME));
    ui->lang->addItem(QString("(") + tr("default") + QString(")"), QVariant(""));
    for (const QString &langStr : translations.entryList())
    {
        QLocale locale(langStr);

        /** check if the locale name consists of 2 parts (language_country) */
        if(langStr.contains("_"))
        {
            /** display language strings as "native language - native country/territory (locale name)", e.g. "Deutsch - Deutschland (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" - ") +
#if (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
                              locale.nativeTerritoryName() +
#else
                              locale.nativeCountryName() +
#endif
                              QString(" (") + langStr + QString(")"), QVariant(langStr));

        }
        else
        {
            /** display language strings as "native language (locale name)", e.g. "Deutsch (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
        }
    }
    ui->unit->setModel(new BitcoinUnits(this));

    /* Widget-to-option mapper */
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);

    GUIUtil::ItemDelegate* delegate = new GUIUtil::ItemDelegate(mapper);
    connect(delegate, &GUIUtil::ItemDelegate::keyEscapePressed, this, &OptionsDialog::reject);
    mapper->setItemDelegate(delegate);

    /* setup/change UI elements when proxy IPs are invalid/valid */
    ui->proxyIp->setCheckValidator(new ProxyAddressValidator(parent));
    ui->proxyIpTor->setCheckValidator(new ProxyAddressValidator(parent));
    connect(ui->proxyIp, &QValidatedLineEdit::validationDidChange, this, &OptionsDialog::updateProxyValidationState);
    connect(ui->proxyIpTor, &QValidatedLineEdit::validationDidChange, this, &OptionsDialog::updateProxyValidationState);
    connect(ui->proxyPort, &QLineEdit::textChanged, this, &OptionsDialog::updateProxyValidationState);
    connect(ui->proxyPortTor, &QLineEdit::textChanged, this, &OptionsDialog::updateProxyValidationState);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        ui->showTrayIcon->setChecked(false);
        ui->showTrayIcon->setEnabled(false);
        ui->minimizeToTray->setChecked(false);
        ui->minimizeToTray->setEnabled(false);
    }

    setupFontOptions(ui->moneyFont, ui->moneyFont_preview);
    setupFontOptions(ui->qrFont, ui->qrFont_preview);
#ifndef USE_QRCODE
    ui->qrFontLabel->setVisible(false);
    ui->qrFont->setVisible(false);
    ui->qrFont_preview->setVisible(false);
#endif

    adjustSize();

    GUIUtil::handleCloseWindowShortcut(this);
    updateThemeColors();
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::setClientModel(ClientModel* client_model)
{
    m_client_model = client_model;
}

void OptionsDialog::setModel(OptionsModel *_model)
{
    this->model = _model;

    if(_model)
    {
        /* check if client restart is needed and show persistent message */
        if (_model->isRestartRequired())
            showRestartWarning(true);

        static constexpr uint64_t nMinDiskSpace = (MIN_DISK_SPACE_FOR_BLOCK_FILES + MiB_BYTES - 1) / MiB_BYTES;
        ui->pruneSizeMiB->setRange(nMinDiskSpace, std::numeric_limits<int>::max());

        QString strLabel = _model->getOverriddenByCommandLine();
        if (strLabel.isEmpty())
            strLabel = tr("none");
        ui->overriddenByCommandLineLabel->setText(strLabel);

        mapper->setModel(_model);
        setMapper();
        mapper->toFirst();

        const auto& font_for_money = _model->data(_model->index(OptionsModel::FontForMoney, 0), Qt::EditRole).value<OptionsModel::FontChoice>();
        setFontChoice(ui->moneyFont, font_for_money);

        const auto& font_for_qrcodes = _model->data(_model->index(OptionsModel::FontForQRCodes, 0), Qt::EditRole).value<OptionsModel::FontChoice>();
        setFontChoice(ui->qrFont, font_for_qrcodes);

        updateDefaultProxyNets();
    }

    /* warn when one of the following settings changes by user action (placed here so init via mapper doesn't trigger them) */

    /* Main */
    connect(ui->prune, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->prune, &QCheckBox::clicked, this, &OptionsDialog::togglePruneWarning);
    connect(ui->pruneSizeMiB, qOverload<int>(&QSpinBox::valueChanged), this, &OptionsDialog::showRestartWarning);
    connect(ui->databaseCache, qOverload<int>(&QSpinBox::valueChanged), this, &OptionsDialog::showRestartWarning);
    connect(ui->externalSignerPath, &QLineEdit::textChanged, [this]{ showRestartWarning(); });
    connect(ui->threadsScriptVerif, qOverload<int>(&QSpinBox::valueChanged), this, &OptionsDialog::showRestartWarning);
    /* Wallet */
    connect(ui->spendZeroConfChange, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    /* Network */
    connect(ui->networkPort, SIGNAL(textChanged(const QString &)), this, SLOT(showRestartWarning()));
    connect(ui->allowIncoming, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->enableServer, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->connectSocks, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->connectSocksTor, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->peerbloomfilters, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->peerblockfilters, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    /* Mempool */
    connect(rejectspkreuse, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    /* Display */
    connect(ui->lang, qOverload<>(&QValueComboBox::valueChanged), [this]{ showRestartWarning(); });
    connect(ui->thirdPartyTxUrls, &QLineEdit::textChanged, [this]{ showRestartWarning(); });
}

void OptionsDialog::setCurrentTab(OptionsDialog::Tab tab)
{
    QWidget *tab_widget = nullptr;
    if (tab == OptionsDialog::Tab::TAB_NETWORK) tab_widget = ui->tabNetwork;
    if (tab == OptionsDialog::Tab::TAB_MAIN) tab_widget = ui->tabMain;
    if (tab_widget && ui->tabWidget->currentWidget() != tab_widget) {
        ui->tabWidget->setCurrentWidget(tab_widget);
    }
}

void OptionsDialog::setMapper()
{
    /* Main */
    mapper->addMapping(ui->bitcoinAtStartup, OptionsModel::StartAtStartup);
    mapper->addMapping(ui->threadsScriptVerif, OptionsModel::ThreadsScriptVerif);
    mapper->addMapping(ui->databaseCache, OptionsModel::DatabaseCache);

    const auto prune_checkstate = model->data(model->index(OptionsModel::PruneTristate, 0), Qt::EditRole).value<Qt::CheckState>();
    if (prune_checkstate == Qt::PartiallyChecked) {
        ui->prune->setTristate();
    }
    ui->prune->setCheckState(prune_checkstate);
    mapper->addMapping(ui->pruneSizeMiB, OptionsModel::PruneSizeMiB);

    /* Wallet */
    mapper->addMapping(walletrbf, OptionsModel::walletrbf);
    mapper->addMapping(ui->addressType, OptionsModel::addresstype);
    mapper->addMapping(ui->spendZeroConfChange, OptionsModel::SpendZeroConfChange);
    mapper->addMapping(ui->coinControlFeatures, OptionsModel::CoinControlFeatures);
    mapper->addMapping(ui->subFeeFromAmount, OptionsModel::SubFeeFromAmount);
    mapper->addMapping(ui->externalSignerPath, OptionsModel::ExternalSignerPath);
    mapper->addMapping(ui->m_enable_psbt_controls, OptionsModel::EnablePSBTControls);

    /* Network */
    mapper->addMapping(ui->networkPort, OptionsModel::NetworkPort);
    mapper->addMapping(ui->mapPortUpnp, OptionsModel::MapPortUPnP);
    mapper->addMapping(ui->mapPortNatpmp, OptionsModel::MapPortNatpmp);
    mapper->addMapping(ui->allowIncoming, OptionsModel::Listen);
    mapper->addMapping(ui->enableServer, OptionsModel::Server);

    mapper->addMapping(ui->connectSocks, OptionsModel::ProxyUse);
    mapper->addMapping(ui->proxyIp, OptionsModel::ProxyIP);
    mapper->addMapping(ui->proxyPort, OptionsModel::ProxyPort);

    mapper->addMapping(ui->connectSocksTor, OptionsModel::ProxyUseTor);
    mapper->addMapping(ui->proxyIpTor, OptionsModel::ProxyIPTor);
    mapper->addMapping(ui->proxyPortTor, OptionsModel::ProxyPortTor);

    int current_maxuploadtarget = model->data(model->index(OptionsModel::maxuploadtarget, 0), Qt::EditRole).toInt();
    if (current_maxuploadtarget == 0) {
        ui->maxuploadtargetCheckbox->setChecked(false);
        ui->maxuploadtarget->setEnabled(false);
        ui->maxuploadtarget->setValue(ui->maxuploadtarget->minimum());
    } else {
        if (current_maxuploadtarget < ui->maxuploadtarget->minimum()) {
            ui->maxuploadtarget->setMinimum(current_maxuploadtarget);
        }
        ui->maxuploadtargetCheckbox->setChecked(true);
        ui->maxuploadtarget->setEnabled(true);
        ui->maxuploadtarget->setValue(current_maxuploadtarget);
    }

    mapper->addMapping(ui->peerbloomfilters, OptionsModel::peerbloomfilters);
    mapper->addMapping(ui->peerblockfilters, OptionsModel::peerblockfilters);
    if (prune_checkstate != Qt::Unchecked && !GetBlockFilterIndex(BlockFilterType::BASIC)) {
        // Once pruning begins, it's too late to enable block filters, and doing so will prevent starting the client
        // Rather than try to monitor sync state, just disable the option once pruning is enabled
        // Advanced users can override this manually anyway
        ui->peerblockfilters->setEnabled(false);
        ui->peerblockfilters->setToolTip(ui->peerblockfilters->toolTip() + " " + tr("(only available if enabled at least once before turning on pruning)"));
    }

    mapper->addMapping(blockreconstructionextratxn, OptionsModel::blockreconstructionextratxn);
    mapper->addMapping(blockreconstructionextratxnsize, OptionsModel::blockreconstructionextratxnsize);

    /* Mempool tab */

    QVariant current_mempoolreplacement = model->data(model->index(OptionsModel::mempoolreplacement, 0), Qt::EditRole);
    int current_mempoolreplacement_index = mempoolreplacement->findData(current_mempoolreplacement);
    if (current_mempoolreplacement_index == -1) {
        mempoolreplacement->addItem(current_mempoolreplacement.toString(), current_mempoolreplacement);
        current_mempoolreplacement_index = mempoolreplacement->count() - 1;
    }
    mempoolreplacement->setCurrentIndex(current_mempoolreplacement_index);

    QVariant current_mempooltruc = model->data(model->index(OptionsModel::mempooltruc, 0), Qt::EditRole);
    int current_mempooltruc_index = mempooltruc->findData(current_mempooltruc);
    if (current_mempooltruc_index == -1) {
        mempooltruc->addItem(current_mempooltruc.toString(), current_mempooltruc);
        current_mempooltruc_index = mempooltruc->count() - 1;
    }
    mempooltruc->setCurrentIndex(current_mempooltruc_index);

    mapper->addMapping(maxorphantx, OptionsModel::maxorphantx);
    mapper->addMapping(maxmempool, OptionsModel::maxmempool);
    mapper->addMapping(incrementalrelayfee, OptionsModel::incrementalrelayfee);
    mapper->addMapping(mempoolexpiry, OptionsModel::mempoolexpiry);

    mapper->addMapping(rejectunknownscripts, OptionsModel::rejectunknownscripts);
    mapper->addMapping(rejectunknownwitness, OptionsModel::rejectunknownwitness);
    mapper->addMapping(rejectparasites, OptionsModel::rejectparasites);
    mapper->addMapping(rejecttokens, OptionsModel::rejecttokens);
    mapper->addMapping(rejectspkreuse, OptionsModel::rejectspkreuse);
    mapper->addMapping(minrelaytxfee, OptionsModel::minrelaytxfee);
    mapper->addMapping(minrelaycoinblocks, OptionsModel::minrelaycoinblocks);
    mapper->addMapping(minrelaymaturity, OptionsModel::minrelaymaturity);
    mapper->addMapping(bytespersigop, OptionsModel::bytespersigop);
    mapper->addMapping(bytespersigopstrict, OptionsModel::bytespersigopstrict);
    mapper->addMapping(limitancestorcount, OptionsModel::limitancestorcount);
    mapper->addMapping(limitancestorsize, OptionsModel::limitancestorsize);
    mapper->addMapping(limitdescendantcount, OptionsModel::limitdescendantcount);
    mapper->addMapping(limitdescendantsize, OptionsModel::limitdescendantsize);
    mapper->addMapping(rejectbarepubkey, OptionsModel::rejectbarepubkey);
    mapper->addMapping(rejectbaremultisig, OptionsModel::rejectbaremultisig);
    mapper->addMapping(rejectbareanchor, OptionsModel::rejectbareanchor);
    mapper->addMapping(rejectbaredatacarrier, OptionsModel::rejectbaredatacarrier);
    mapper->addMapping(maxscriptsize, OptionsModel::maxscriptsize);
    mapper->addMapping(maxtxlegacysigops, OptionsModel::maxtxlegacysigops);
    mapper->addMapping(datacarriercost, OptionsModel::datacarriercost);
    mapper->addMapping(datacarriersize, OptionsModel::datacarriersize);
    mapper->addMapping(rejectnonstddatacarrier, OptionsModel::rejectnonstddatacarrier);
    mapper->addMapping(dustrelayfee, OptionsModel::dustrelayfee);

    QVariant current_permitephemeral = model->data(model->index(OptionsModel::permitephemeral, 0), Qt::EditRole);
    int current_permitephemeral_index = permitephemeral->findData(current_permitephemeral);
    if (current_permitephemeral_index == -1) {
        permitephemeral->addItem(current_permitephemeral.toString(), current_permitephemeral);
        current_permitephemeral_index = permitephemeral->count() - 1;
    }
    permitephemeral->setCurrentIndex(current_permitephemeral_index);

    QVariant current_dustdynamic = model->data(model->index(OptionsModel::dustdynamic, 0), Qt::EditRole);
    const util::Result<std::pair<int32_t, int>> parsed_dustdynamic = ParseDustDynamicOpt(current_dustdynamic.toString().toStdString(), std::numeric_limits<unsigned int>::max());
    if (parsed_dustdynamic) {
        if (parsed_dustdynamic->first == 0) {
            dustdynamic_enable->setChecked(false);
        } else {
            dustdynamic_multiplier->setValue(parsed_dustdynamic->second / 1000.0);
            if (parsed_dustdynamic->first < 0) {
                dustdynamic_target->setChecked(true);
                dustdynamic_target_blocks->setValue(-parsed_dustdynamic->first);
            } else {
                dustdynamic_mempool->setChecked(true);
                dustdynamic_mempool_kvB->setValue(parsed_dustdynamic->first);
            }
            dustdynamic_enable->setChecked(true);
        }
    }

    /* Mining tab */

    mapper->addMapping(blockmintxfee, OptionsModel::blockmintxfee);
    mapper->addMapping(blockmaxsize, OptionsModel::blockmaxsize);
    mapper->addMapping(blockprioritysize, OptionsModel::blockprioritysize);
    mapper->addMapping(blockmaxweight, OptionsModel::blockmaxweight);

    /* Window */
#ifndef Q_OS_MACOS
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        mapper->addMapping(ui->showTrayIcon, OptionsModel::ShowTrayIcon);
        mapper->addMapping(ui->minimizeToTray, OptionsModel::MinimizeToTray);
    }
    mapper->addMapping(ui->minimizeOnClose, OptionsModel::MinimizeOnClose);
#endif

    /* Display */
    mapper->addMapping(ui->peersTabAlternatingRowColors, OptionsModel::PeersTabAlternatingRowColors);
    mapper->addMapping(ui->lang, OptionsModel::Language);
    mapper->addMapping(ui->unit, OptionsModel::DisplayUnit);
    mapper->addMapping(ui->displayAddresses, OptionsModel::DisplayAddresses);
    mapper->addMapping(ui->thirdPartyTxUrls, OptionsModel::ThirdPartyTxUrls);
}

void OptionsDialog::checkLineEdit()
{
    QLineEdit * const lineedit = qobject_cast<QLineEdit*>(QObject::sender());
    if (lineedit->hasAcceptableInput()) {
        lineedit->setStyleSheet("");
    } else {
        // Check the line edit's actual background to choose appropriate warning color
        const bool lineedit_dark = GUIUtil::isDarkMode(lineedit->palette().color(lineedit->backgroundRole()));
        const QColor lineedit_warning = lineedit_dark ? QColor("#FF8080") : QColor("#FF0000");
        lineedit->setStyleSheet(QStringLiteral("color: %1;").arg(lineedit_warning.name()));
    }
}

void OptionsDialog::setOkButtonState(bool fState)
{
    ui->okButton->setEnabled(fState);
}

void OptionsDialog::incrementalrelayfee_changed()
{
    if (incrementalrelayfee->value() > minrelaytxfee->value()) {
        minrelaytxfee->setValue(incrementalrelayfee->value());
    }
}

void OptionsDialog::blockmaxsize_changed(int i)
{
    if (blockprioritysize->value() > i) {
        blockprioritysize->setValue(i);
    }

    if (blockmaxweight->value() < i) {
        blockmaxweight->setValue(i);
    } else if (blockmaxweight->value() > i * WITNESS_SCALE_FACTOR) {
        blockmaxweight->setValue(i * WITNESS_SCALE_FACTOR);
    }
}

void OptionsDialog::blockmaxsize_increase(int i)
{
    if (blockmaxsize->value() < i) {
        blockmaxsize->setValue(i);
    }
}

void OptionsDialog::blockmaxweight_changed(int i)
{
    if (blockmaxsize->value() < i / WITNESS_SCALE_FACTOR) {
        blockmaxsize->setValue(i / WITNESS_SCALE_FACTOR);
    } else if (blockmaxsize->value() > i) {
        blockmaxsize->setValue(i);
    }
}

void OptionsDialog::on_resetButton_clicked()
{
    if (model) {
        // confirmation dialog
        /*: Text explaining that the settings changed will not come into effect
            until the client is restarted. */
        QString reset_dialog_text = tr("Client restart required to activate changes.") + "<br><br>";
        /*: Text explaining to the user that the client's current settings
            will be backed up at a specific location. %1 is a stand-in
            argument for the backup location's path. */
        reset_dialog_text.append(tr("Current settings will be backed up at \"%1\".").arg(m_client_model->dataDir()) + "<br><br>");
        /*: Text asking the user to confirm if they would like to proceed
            with a client shutdown. */
        reset_dialog_text.append(tr("Client will be shut down. Do you want to proceed?"));
        //: Window title text of pop-up window shown when the user has chosen to reset options.
        QStringList items;
        QString strPrefix = tr("Use policy defaults for %1");
        items << strPrefix.arg(tr(CLIENT_NAME));
        items << strPrefix.arg(tr("Bitcoin Core")+" ");

        QInputDialog dialog(this);
        dialog.setWindowTitle(tr("Confirm options reset"));
        dialog.setLabelText(reset_dialog_text);
        dialog.setComboBoxItems(items);
        dialog.setTextValue(items[0]);
        dialog.setComboBoxEditable(false);

        if (!dialog.exec()) {
            return;
        }

        /* reset all options and close GUI */
        model->Reset();
        model->setData(model->index(OptionsModel::corepolicy, 0), items.indexOf(dialog.textValue()));
        close();
        Q_EMIT quitOnReset();
    }
}

void OptionsDialog::on_openBitcoinConfButton_clicked()
{
    QMessageBox config_msgbox(this);
    config_msgbox.setIcon(QMessageBox::Information);
    //: Window title text of pop-up box that allows opening up of configuration file.
    config_msgbox.setWindowTitle(tr("Configuration options"));
    /*: Explanatory text about the priority order of instructions considered by client.
        The order from high to low being: command-line, configuration file, GUI settings. */
    config_msgbox.setText(tr("The configuration file is used to specify advanced user options which override GUI settings. "
                             "Additionally, any command-line options will override this configuration file."));

    QPushButton* open_button = config_msgbox.addButton(tr("Continue"), QMessageBox::ActionRole);
    config_msgbox.addButton(tr("Cancel"), QMessageBox::RejectRole);
    open_button->setDefault(true);

    config_msgbox.exec();

    if (config_msgbox.clickedButton() != open_button) return;

    /* show an error if there was some problem opening the file */
    if (!GUIUtil::openBitcoinConf())
        QMessageBox::critical(this, tr("Error"), tr("The configuration file could not be opened."));
}

void OptionsDialog::on_okButton_clicked()
{
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        QWidget * const tab = ui->tabWidget->widget(i);
        Q_FOREACH(QObject* o, tab->children()) {
            QLineEdit * const lineedit = qobject_cast<QLineEdit*>(o);
            if (lineedit && !lineedit->hasAcceptableInput()) {
                int row = mapper->mappedSection(lineedit);
                if (model->data(model->index(row, 0), Qt::EditRole) == lineedit->text()) {
                    // Allow unchanged fields through
                    continue;
                }
                ui->tabWidget->setCurrentWidget(tab);
                lineedit->setFocus(Qt::OtherFocusReason);
                lineedit->selectAll();
                QMessageBox::critical(this, tr("Invalid setting"), tr("The value entered is invalid."));
                return;
            }
        }
    }

    model->setData(model->index(OptionsModel::PruneTristate, 0), ui->prune->checkState());

    model->setData(model->index(OptionsModel::FontForMoney, 0), ui->moneyFont->itemData(ui->moneyFont->currentIndex()));
    model->setData(model->index(OptionsModel::FontForQRCodes, 0), ui->qrFont->itemData(ui->qrFont->currentIndex()));

    if (ui->maxuploadtargetCheckbox->isChecked()) {
        model->setData(model->index(OptionsModel::maxuploadtarget, 0), ui->maxuploadtarget->value());
    } else {
        model->setData(model->index(OptionsModel::maxuploadtarget, 0), 0);
    }

    model->setData(model->index(OptionsModel::mempoolreplacement, 0), mempoolreplacement->itemData(mempoolreplacement->currentIndex()));
    model->setData(model->index(OptionsModel::mempooltruc, 0), mempooltruc->itemData(mempooltruc->currentIndex()));
    model->setData(model->index(OptionsModel::permitephemeral, 0), permitephemeral->itemData(permitephemeral->currentIndex()));

    if (dustdynamic_enable->isChecked()) {
        if (dustdynamic_target->isChecked()) {
            model->setData(model->index(OptionsModel::dustdynamic, 0), QStringLiteral("%2*target:%1").arg(dustdynamic_target_blocks->value()).arg(dustdynamic_multiplier->value()));
        } else if (dustdynamic_mempool->isChecked()) {
            model->setData(model->index(OptionsModel::dustdynamic, 0), QStringLiteral("%2*mempool:%1").arg(dustdynamic_mempool_kvB->value()).arg(dustdynamic_multiplier->value()));
        }
    } else {
        model->setData(model->index(OptionsModel::dustdynamic, 0), "off");
    }

    mapper->submit();
    accept();
    updateDefaultProxyNets();
}

void OptionsDialog::on_cancelButton_clicked()
{
    reject();
}

void OptionsDialog::on_showTrayIcon_stateChanged(int state)
{
    if (state == Qt::Checked) {
        ui->minimizeToTray->setEnabled(true);
    } else {
        ui->minimizeToTray->setChecked(false);
        ui->minimizeToTray->setEnabled(false);
    }
}

void OptionsDialog::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::PaletteChange) {
        updateThemeColors();
    }

    QWidget::changeEvent(e);
}

void OptionsDialog::togglePruneWarning(bool enabled)
{
    ui->pruneWarning->setVisible(!ui->pruneWarning->isVisible());
}

void OptionsDialog::showRestartWarning(bool fPersistent)
{
    if(fPersistent)
    {
        ui->statusLabel->setText(tr("Client restart required to activate changes."));
    }
    else
    {
        ui->statusLabel->setText(tr("This change would require a client restart."));
        // clear non-persistent status label after 10 seconds
        // Todo: should perhaps be a class attribute, if we extend the use of statusLabel
        QTimer::singleShot(10s, this, &OptionsDialog::clearStatusLabel);
    }
}

void OptionsDialog::clearStatusLabel()
{
    ui->statusLabel->clear();
    if (model && model->isRestartRequired()) {
        showRestartWarning(true);
    }
}

void OptionsDialog::updateProxyValidationState()
{
    QValidatedLineEdit *pUiProxyIp = ui->proxyIp;
    QValidatedLineEdit *otherProxyWidget = (pUiProxyIp == ui->proxyIpTor) ? ui->proxyIp : ui->proxyIpTor;
    if (pUiProxyIp->isValid() && (!ui->proxyPort->isEnabled() || ui->proxyPort->text().toInt() > 0) && (!ui->proxyPortTor->isEnabled() || ui->proxyPortTor->text().toInt() > 0))
    {
        setOkButtonState(otherProxyWidget->isValid()); //only enable ok button if both proxies are valid
        clearStatusLabel();
    }
    else
    {
        setOkButtonState(false);
        ui->statusLabel->setText(tr("The supplied proxy address is invalid."));
    }
}

void OptionsDialog::updateDefaultProxyNets()
{
    std::string proxyIpText{ui->proxyIp->text().toStdString()};
    if (!IsUnixSocketPath(proxyIpText)) {
        const std::optional<CNetAddr> ui_proxy_netaddr{LookupHost(proxyIpText, /*fAllowLookup=*/false)};
        const CService ui_proxy{ui_proxy_netaddr.value_or(CNetAddr{}), ui->proxyPort->text().toUShort()};
        proxyIpText = ui_proxy.ToStringAddrPort();
    }

    Proxy proxy;
    bool has_proxy;

    has_proxy = model->node().getProxy(NET_IPV4, proxy);
    ui->proxyReachIPv4->setChecked(has_proxy && proxy.ToString() == proxyIpText);

    has_proxy = model->node().getProxy(NET_IPV6, proxy);
    ui->proxyReachIPv6->setChecked(has_proxy && proxy.ToString() == proxyIpText);

    has_proxy = model->node().getProxy(NET_ONION, proxy);
    ui->proxyReachTor->setChecked(has_proxy && proxy.ToString() == proxyIpText);
}

void OptionsDialog::updateThemeColors()
{
    // Detect dark mode for color palette selection
    const bool dark_mode = GUIUtil::isDarkMode(palette().color(backgroundRole()));

    // set message warning color based on dark mode
    const QColor warning_color = dark_mode ? QColor("#FF8080") : QColor("#FF0000");
    ui->pruneWarning->setStyleSheet(QStringLiteral("QLabel { color: %1; }").arg(warning_color.name()));
    ui->statusLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; }").arg(warning_color.name()));

    // Update networkPort line edit color if it has validation errors
    if (!ui->networkPort->hasAcceptableInput()) {
        // Check networkPort's actual background for appropriate warning color
        const bool networkport_dark = GUIUtil::isDarkMode(ui->networkPort->palette().color(ui->networkPort->backgroundRole()));
        const QColor networkport_warning = networkport_dark ? QColor("#FF8080") : QColor("#FF0000");
        ui->networkPort->setStyleSheet(QStringLiteral("color: %1;").arg(networkport_warning.name()));
    }
}

ProxyAddressValidator::ProxyAddressValidator(QObject *parent) :
QValidator(parent)
{
}

QValidator::State ProxyAddressValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);
    uint16_t port{0};
    std::string hostname;
    if (!SplitHostPort(input.toStdString(), port, hostname) || port != 0) return QValidator::Invalid;

    CService serv(LookupNumeric(input.toStdString(), DEFAULT_GUI_PROXY_PORT));
    Proxy addrProxy = Proxy(serv, true);
    if (addrProxy.IsValid())
        return QValidator::Acceptable;

    return QValidator::Invalid;
}
