// Copyright (c) 2024 Luke Dashjr
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <qt/blockview.h>

#include <addresstype.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <logging.h>
#include <node/context.h>
#include <node/miner.h>
#include <primitives/block.h>
#include <util/strencodings.h>
#include <validation.h>
#include <validationinterface.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/networkstyle.h>
#include <qt/optionsmodel.h>

#include <cmath>
#include <numbers>

#include <QColor>
#include <QComboBox>
#include <QLabel>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPalette>
#include <QHBoxLayout>
#include <QToolTip>
#include <QVariant>
#include <QVBoxLayout>

Q_DECLARE_METATYPE(CTransactionRef)

static constexpr qreal TX_PADDING_NEXT{4};
static constexpr qreal TX_PADDING_NEARBY{2};
static constexpr qreal EXPECTED_WHITESPACE_PERCENT{1.5};
static constexpr auto RADIAN_DIVISOR{8};

void ScalingGraphicsView::mouseMoveEvent(QMouseEvent * const event)
{
    auto * const gi = itemAt(event->pos());
    const auto tx = gi ? gi->data(0).value<CTransactionRef>() : CTransactionRef();
    if (!tx) {
        QToolTip::showText(QPoint{}, QStringLiteral(""), nullptr, {}, 0);
        return;
    }

    QString tx_info_str = "<qt>" + QString::fromStdString(tx->GetHash().ToString()) + "<br>";
    tx_info_str += "<br>" + tr("Size: %1 bytes").arg(tx->GetTotalSize());
    tx_info_str += "<br>Outputs:<div style=\"margin-left:4ex;margin-top:0;padding-top:0\">";

    BitcoinUnit unit;
    QFont font_for_money;
    if (auto* options_model = (m_bv && m_bv->m_client_model) ? m_bv->m_client_model->getOptionsModel() : nullptr; options_model) {
        unit = options_model->getDisplayUnit();
        font_for_money = options_model->getFontForMoney(unit);
    } else {
        unit = BitcoinUnit::BTC;
    }
    int i = 0;
    for (const auto& txout : tx->vout) {
        ++i;
        CTxDestination dest;
        QString address;
        if (ExtractDestination(txout.scriptPubKey, dest)) {
            address = GUIUtil::HtmlEscape(EncodeDestination(dest));
        } else {
            address = tr("(unknown)");
        }
        auto amount_str = BitcoinUnits::formatHtmlWithUnit(font_for_money, unit, txout.nValue);
        if (i > 1) tx_info_str += "<br>";
        tx_info_str += tr("#%1: %2 to %3").arg(i).arg(amount_str).arg(address);
    }
    tx_info_str += "</div></qt>";
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    const QPoint event_global_pos = event->globalPosition().toPoint();
#else
    const QPoint event_global_pos = event->globalPos();
#endif
    QToolTip::showText(event_global_pos, tx_info_str, this, {}, std::numeric_limits<int>::max());
}

void ScalingGraphicsView::resizeEvent(QResizeEvent * const event)
{
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    QGraphicsView::resizeEvent(event);
}

bool ScalingGraphicsView::viewportEvent(QEvent * const event)
{
    if (event->type() == QEvent::ToolTip) {
        // causes QGraphicsScene to destroy our tooltips, so block it here
        return true;
    }
    return QGraphicsView::viewportEvent(event);
}

class BlockViewValidationInterface final : public CValidationInterface
{
private:
    GuiBlockView& m_bv;

public:
    explicit BlockViewValidationInterface(GuiBlockView& bv) : m_bv(bv) {}

    void BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>& block_cached, const CBlockIndex* pblockindex) override {
        static_assert(std::is_same<int, decltype(pblockindex->nHeight)>::value, "nHeight type assumption does not hold");
        QMetaObject::invokeMethod(&m_bv, "updateBestBlock", Qt::QueuedConnection, Q_ARG(int, pblockindex->nHeight));

        if (!m_bv.m_follow_tip) return;

        std::shared_ptr<const CBlock> block = block_cached;
        auto chainman = m_bv.getChainstateManager();
        Assert(chainman);
        if (!block) {
            std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
            if (!chainman->m_blockman.ReadBlock(*pblock, *pblockindex)) {
                // Indicate error somehow?
                return;
            }
            block = pblock;
        }

        const auto block_subsidy = GetBlockSubsidy(pblockindex->nHeight, chainman->GetParams().GetConsensus());

        m_bv.setBlock(block, block_subsidy);
    }

    void NewBlockTemplate(const std::shared_ptr<node::CBlockTemplate>& blocktemplate) override {
        {
            LOCK(m_bv.m_mutex);
            if (m_bv.m_block) {
                // Update cached template, but don't render it
                m_bv.m_block_template = blocktemplate;
                return;
            }
        }

        m_bv.setBlock(blocktemplate);
    }
};

void GuiBlockView::updateBestBlock(const int height)
{
    m_block_chooser->setItemText(1, tr("Newest block (%1)").arg(height));
}

GuiBlockView::GuiBlockView(const PlatformStyle *platformStyle, const NetworkStyle *networkStyle, QWidget *parent) :
    QDialog(parent, GUIUtil::dialog_flags | Qt::WindowMaximizeButtonHint)
{
    setWindowTitle(tr(CLIENT_NAME) + " - " + tr("Block View") + " " + networkStyle->getTitleAddText());
    setWindowIcon(networkStyle->getTrayAndWindowIcon());
    resize(640, 640);

    updateThemeColors();

    QVBoxLayout * const layout = new QVBoxLayout(this);
    setLayout(layout);

    auto hlayout = new QHBoxLayout;
    layout->addLayout(hlayout);
    hlayout->addWidget(new QLabel(tr("Displayed block: ")));
    m_block_chooser = new QComboBox(this);
    hlayout->addWidget(m_block_chooser, 1);
    connect(m_block_chooser, QOverload<int>::of(&QComboBox::currentIndexChanged), [=, this](const int index){
        m_follow_tip = false;
        auto ud = m_block_chooser->itemData(index).toInt();
        if (ud == -3) {
            m_block_chooser->setEditable(false);
            auto block_template = WITH_LOCK(m_mutex, return m_block_template);
            if (block_template) {
                setBlock(block_template);
            } else {
                clear();
            }
            return;
        }

        auto chainman = getChainstateManager();
        if (!chainman) {
            clear();
            return;
        }
        auto& blockman = chainman->m_blockman;

        CBlockIndex *pblockindex;
        if (ud == -2) {
            m_follow_tip = true;
            pblockindex = WITH_LOCK(::cs_main, return chainman->ActiveChain().Tip());
            if (!pblockindex) {
                clear();
                return;
            }
        } else if (ud == -1) {
            m_block_chooser->setEditable(true);
            m_block_chooser->clearEditText();
            return;
        } else {
            auto qtxt = m_block_chooser->itemText(index);
            auto txt = qtxt.toStdString();
            auto blockhash{uint256::FromHex(txt)};
            if (blockhash) {
                LOCK(cs_main);
                pblockindex = blockman.LookupBlockIndex(*blockhash);
            } else if (auto height = ToIntegral<int>(txt)) {
                LOCK(cs_main);
                pblockindex = chainman->ActiveChain()[*height];
            } else {
                pblockindex = nullptr;
            }
            if (!pblockindex) {
                clear();
                QMessageBox::critical(this, tr("Invalid block"), tr("\"%1\" is not a valid block height or hash!").arg(qtxt));
                m_block_chooser->removeItem(index);
                return;
            }
        }

        std::shared_ptr<CBlock> block = std::make_shared<CBlock>();
        if ((!blockman.ReadBlock(*block, *pblockindex)) || block->vtx.empty()) {
            clear();
            const bool is_pruned = WITH_LOCK(::cs_main, return blockman.IsBlockPruned(*pblockindex));
            if (is_pruned) {
                QMessageBox::critical(this, tr("Pruned block"), tr("Block %1 (%2) is pruned.").arg(pblockindex->nHeight).arg(QString::fromStdString(pblockindex->GetBlockHash().ToString())));
            } else {
                QMessageBox::critical(this, tr("Error reading block"), tr("Block %1 (%2) could not be loaded.").arg(pblockindex->nHeight).arg(QString::fromStdString(pblockindex->GetBlockHash().ToString())));
            }
            m_block_chooser->removeItem(index);
            return;
        }

        m_block_chooser->setEditable(false);

        const auto block_subsidy = GetBlockSubsidy(pblockindex->nHeight, chainman->GetParams().GetConsensus());

        setBlock(block, block_subsidy);
    });
    // Items initialized later, after ClientModel is available

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 1, 1);
    auto view = new ScalingGraphicsView(m_scene, this);
    view->m_bv = this;
    view->setMouseTracking(true);
    layout->addWidget(view);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setStyleSheet("QGraphicsView { background: transparent; }");
    view->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    connect(m_scene, &QGraphicsScene::sceneRectChanged, [view](const QRectF& rect){
        view->fitInView(rect, Qt::KeepAspectRatio);
    });

    hlayout = new QHBoxLayout;
    layout->addLayout(hlayout);
    hlayout->addWidget(new QLabel(tr("Transactions"), this));
    m_lbl_tx_count = new QLabel(this);
    m_lbl_tx_count->setAlignment(Qt::AlignRight);
    hlayout->addWidget(m_lbl_tx_count);

    hlayout = new QHBoxLayout;
    layout->addLayout(hlayout);
    hlayout->addWidget(new QLabel(tr("Txn Fees"), this));
    m_lbl_tx_fees = new QLabel(this);
    m_lbl_tx_fees->setAlignment(Qt::AlignRight);
    hlayout->addWidget(m_lbl_tx_fees);

    connect(&m_timer, &QTimer::timeout, this, &GuiBlockView::updateScene);

    m_validation_interface = new BlockViewValidationInterface(*this);
}

GuiBlockView::~GuiBlockView()
{
    if (m_validation_interface) {
        setClientModel(nullptr);
        delete m_validation_interface;
        m_validation_interface = nullptr;
    }
}

void GuiBlockView::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::PaletteChange) {
        updateThemeColors();
    }
    QDialog::changeEvent(e);
}

void GuiBlockView::setClientModel(ClientModel *model)
{
    if (m_client_model) {
        auto& validation_signals = m_client_model->node().context()->validation_signals;
        if (validation_signals) {
            validation_signals->UnregisterValidationInterface(m_validation_interface);
        }
        disconnect(m_client_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &GuiBlockView::updateDisplayUnit);
    }
    m_client_model = model;
    if (model) {
        connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &GuiBlockView::updateDisplayUnit);
        updateDisplayUnit();

        if (m_block_chooser->count() == 0) {
            m_block_chooser->addItem(tr("This node's preferred block template"), -3);
            m_block_chooser->addItem("", -2);
            m_block_chooser->addItem(tr("Specific block"), -1);
            m_block_chooser->setCurrentIndex(1);
        }

        auto chainman = getChainstateManager();
        if (chainman) {
            const auto pblockindex = WITH_LOCK(::cs_main, return chainman->ActiveChain().Tip());
            updateBestBlock(pblockindex->nHeight);
        }
        auto& validation_signals = model->node().context()->validation_signals;
        if (validation_signals) {
            validation_signals->RegisterValidationInterface(m_validation_interface);
        }
    }
}

ChainstateManager* GuiBlockView::getChainstateManager() const
{
    if (!m_client_model) return nullptr;
    auto node_ctx = m_client_model->node().context();
    if (!node_ctx) return nullptr;
    auto& chainman = node_ctx->chainman;
    if (!chainman) return nullptr;
    return &(*chainman);
}

void GuiBlockView::clear()
{
    LOCK(m_mutex);
    m_block_fees = -1;
    m_lbl_tx_count->setText("");
    m_block.reset();
    m_block_template.reset();
    for (auto& [wtxid, elem] : m_elements) {
        const auto gi = elem.gi;
        m_scene->removeItem(gi);
        delete gi;
    }
    m_elements.clear();
}

bool GuiBlockView::any_overlap(const Bubble& proposed, const std::vector<Bubble>& others)
{
    for (const auto& other : others) {
        const auto x_dist = std::abs(other.pos.x() - proposed.pos.x());
        const auto y_dist = std::abs(other.pos.y() - proposed.pos.y());
        const auto dist = std::sqrt((x_dist * x_dist) + (y_dist * y_dist));
        if (dist < proposed.radius + other.radius + TX_PADDING_NEARBY) {
            return true;
        }
    }
    return false;
}

void GuiBlockView::setBlock(std::shared_ptr<const CBlock> block, const CAmount block_subsidy)
{
    LOCK(m_mutex);
    m_block_fees = [&] {
        CAmount total{0};
        Assert(!block->vtx.empty());
        for (const auto& outp : block->vtx[0]->vout) {
            total += outp.nValue;
        }
        return total - block_subsidy;
    }();
    m_block = block;
    m_block_template.reset();
    m_block_changed = true;
    updateElements(/*instant=*/ true);
}

void GuiBlockView::setBlock(std::shared_ptr<const node::CBlockTemplate> blocktemplate)
{
    LOCK(m_mutex);
    const bool instant = (bool)m_block;  // force instant if changing from real block to template
    m_block_fees = -blocktemplate->vTxFees.front();
    m_block.reset();
    m_block_template = blocktemplate;
    m_block_changed = true;
    updateElements(/*instant=*/ instant);
}

void GuiBlockView::updateBlockFees(CAmount block_fees)
{
    if (block_fees < 0) {
        m_lbl_tx_fees->setText("");
        return;
    }
    BitcoinUnit unit;
    QFont font_for_money;
    if (auto* options_model = m_client_model ? m_client_model->getOptionsModel() : nullptr; options_model) {
        unit = options_model->getDisplayUnit();
        font_for_money = options_model->getFontForMoney(unit);
    } else {
        unit = BitcoinUnit::BTC;
    }
    m_lbl_tx_fees->setFont(font_for_money);
    m_lbl_tx_fees->setText(BitcoinUnits::formatWithUnit(unit, block_fees));
}

void GuiBlockView::updateDisplayUnit()
{
    const auto block_fees = WITH_LOCK(m_mutex, return m_block_fees);
    updateBlockFees(block_fees);
}

constexpr qreal offscreen{99};

void GuiBlockView::updateElements(bool instant)
{
    if (!m_block_changed) return;

    m_timer.stop();
    m_block_changed = false;
    auto pblocktemplate = m_block_template;
    auto pblock = m_block;
    auto& block = pblock ? *pblock : pblocktemplate->block;

    instant |= m_elements.empty();
    for (auto& el : m_elements) {
        el.second.target_loc.setY(offscreen);
    }
    m_bubblegraph = std::make_unique<BubbleGraph>();
    m_bubblegraph->txs_count = block.vtx.size() - 1;
    auto& bubbles = m_bubblegraph->bubbles;
    qreal limit_halfwidth{std::sqrt(::GetSerializeSize(TX_WITH_WITNESS(block))) * EXPECTED_WHITESPACE_PERCENT / 2};
    for (size_t i = 1; i < block.vtx.size(); ++i) {
        auto& tx = *block.vtx[i];
        auto& el = m_elements[tx.GetWitnessHash()];
        QPointF preferred_loc;
        double diameter;
        const auto tx_size = tx.GetTotalSize();
        m_bubblegraph->txs_size += tx_size;
        const bool fresh_bubble = !el.gi;
        if (fresh_bubble) {
            diameter = 2 * std::sqrt(tx_size / std::numbers::pi);
        } else {
            // preferred_loc = el.gi->pos();
            diameter = el.gi->boundingRect().height();
        }
        Bubble proposed{ .tx = block.vtx[i], .pos = {}, .radius = diameter / 2, .el = &el, };
        qreal x_extremity{proposed.radius};
        if (bubbles.empty()) {
            proposed.pos.setY(-proposed.radius);
        }
        for (auto bubble_it = bubbles.rbegin(); bubble_it != bubbles.rend(); ++bubble_it) {
            const auto& centre = bubble_it->pos;
            QPointF preferred_loc_rel(preferred_loc.x() - centre.x(), preferred_loc.y() - centre.y());
            double preferred_angle;
            if (preferred_loc_rel.isNull()) {
                preferred_angle = std::numbers::pi / 2;
            } else {
                preferred_angle = std::atan2(preferred_loc.y() - centre.y(), preferred_loc.x() - centre.x());
            }
            const auto distance = bubble_it->radius + proposed.radius + TX_PADDING_NEXT;
            double angle = preferred_angle;
            bool found{false};
            while (true) {
                proposed.pos = QPointF(centre.x() + (distance * std::cos(angle)), centre.y() + (distance * std::sin(angle)));

                x_extremity = std::abs(proposed.pos.x()) + proposed.radius;
                if (proposed.pos.y() < -proposed.radius && x_extremity <= limit_halfwidth && !any_overlap(proposed, bubbles)) {
                    found = true;
                    break;
                }

                if (angle < preferred_angle) {
                    angle = preferred_angle + (preferred_angle - angle);
                } else {
                    angle = preferred_angle - (angle - preferred_angle) - (std::numbers::pi / RADIAN_DIVISOR);
                }
                if (angle > preferred_angle + std::numbers::pi) {
                    break;
                }
            }
            if (found) break;
        }
        m_bubblegraph->min_x = std::min(m_bubblegraph->min_x, proposed.pos.x() - proposed.radius);
        m_bubblegraph->max_x = std::max(m_bubblegraph->max_x, proposed.pos.x() + proposed.radius);
        m_bubblegraph->min_y = std::min(m_bubblegraph->min_y, proposed.pos.y() - proposed.radius);
        bubbles.push_back(proposed);
        el.target_loc = proposed.pos;
    }
    m_bubblegraph->instant = instant;
    QMetaObject::invokeMethod(this, "updateSceneInit", Qt::QueuedConnection);
}

void GuiBlockView::updateSceneInit()
{
    LOCK(m_mutex);
    if (!m_bubblegraph) return;

    m_lbl_tx_count->setText(tr("%1 (%2)").arg(m_bubblegraph->txs_count).arg(tr("%1 kB").arg(m_bubblegraph->txs_size / 1000.0, 0, 'f', 1)));
    updateBlockFees(m_block_fees);

    for (auto& bubble : m_bubblegraph->bubbles) {
        auto& el = *bubble.el;
        if (!el.gi) {
            const auto diameter = bubble.radius * 2;
            auto gi = m_scene->addEllipse(0, 0, diameter, diameter, QPen(palette().window(), TX_PADDING_NEARBY));
            el.gi = gi;
            gi->setData(0, QVariant::fromValue(std::move(bubble.tx)));
            gi->setBrush(m_bubble_color);
            gi->setPos(bubble.pos.x() - bubble.radius, m_bubblegraph->instant ? (bubble.pos.y() - bubble.radius) : offscreen);
        }
    }
    for (auto it = m_elements.begin(); it != m_elements.end(); ) {
        const auto& target_loc = it->second.target_loc;
        const auto gi = it->second.gi;
        bool delete_el{false};
        if (target_loc.y() == offscreen || !gi /* never got a chance to exist */) {
            delete_el = true;
            // TODO: if confirmed, slide it off the bottom
            // TODO: if conflicted, pop the bubble?
            // TODO: if delayed, move off the top
        } else {
            if (gi->y() == offscreen) {
                gi->setY(m_bubblegraph->min_y - gi->boundingRect().height());
            }
        }
        if (delete_el) {
            if (gi) {
                m_scene->removeItem(gi);
                delete gi;
            }
            it = m_elements.erase(it);
        } else {
            ++it;
        }
    }
    m_scene->setSceneRect(m_bubblegraph->min_x, m_bubblegraph->min_y, m_bubblegraph->max_x - m_bubblegraph->min_x, -m_bubblegraph->min_y);
    if (!m_bubblegraph->instant) {
        m_frame_div = 4;
        updateScene();
        m_timer.start(100);
    }
    m_bubblegraph.reset();
}

void GuiBlockView::updateScene()
{
    LOCK(m_mutex);
    bool all_completed{true};
    for (auto it = m_elements.begin(); it != m_elements.end(); ) {
        const auto& target_loc = it->second.target_loc;
        QGraphicsItem* gi = it->second.gi;
        const auto radius = gi->boundingRect().width() / 2;
        const QPointF current_loc(gi->pos().x() + radius, gi->pos().y() + radius);
        bool delete_el{false};
        if (target_loc != current_loc) {
            // Get 25% closer each tick
            QPointF new_loc(current_loc.x() + ((target_loc.x() - current_loc.x()) / m_frame_div),
                            current_loc.y() + ((target_loc.y() - current_loc.y()) / m_frame_div));
            if (std::abs(new_loc.x() - target_loc.x()) < TX_PADDING_NEXT) {
                new_loc.setX(target_loc.x());
            }
            if (std::abs(new_loc.y() - target_loc.y()) < TX_PADDING_NEXT) {
                new_loc.setY(target_loc.y());
            }
            gi->setPos(new_loc.x() - radius, new_loc.y() - radius);
            if (new_loc == target_loc) {
                if (target_loc.y() + radius < m_scene->sceneRect().y() || target_loc.y() - radius > 0) {
                    delete_el = true;
                }
            } else {
                all_completed = false;
            }
        }
        if (delete_el) {
            m_scene->removeItem(gi);
            delete gi;
            it = m_elements.erase(it);
        } else {
            ++it;
        }
    }
    --m_frame_div;
    if (all_completed) {
        m_timer.stop();
    }
}

void GuiBlockView::updateThemeColors()
{
    // Store old color to check if it actually changes
    const QColor old_color = m_bubble_color;

    // Detect dark mode for color palette selection
    const bool dark_mode = GUIUtil::isDarkMode(palette().color(backgroundRole()));
    m_bubble_color = dark_mode ? QColor(137, 170, 255) : QColor(2, 61, 204);

    // Only update existing bubbles if color actually changed
    if (old_color != m_bubble_color) {
        LOCK(m_mutex);
        for (auto& el : m_elements) {
            if (el.second.gi) {
                if (auto* ellipse = dynamic_cast<QGraphicsEllipseItem*>(el.second.gi)) {
                    ellipse->setBrush(m_bubble_color);
                }
            }
        }
    }
}
