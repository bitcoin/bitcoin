// Copyright (c) 2024 Luke Dashjr
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_BLOCKVIEW_H
#define BITCOIN_QT_BLOCKVIEW_H

#include <consensus/amount.h>
#include <sync.h>
#include <threadsafety.h>
#include <util/transaction_identifier.h>

#include <atomic>
#include <map>
#include <memory>
#include <vector>

#include <QDialog>
#include <QGraphicsView>
#include <QPointF>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QComboBox;
class QGraphicsItem;
class QGraphicsScene;
class QLabel;
class QWidget;
QT_END_NAMESPACE

class CBlock;
namespace node { struct CBlockTemplate; }
class ChainstateManager;
class ClientModel;
class CValidationInterface;
class NetworkStyle;
class PlatformStyle;

class ScalingGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    using QGraphicsView::QGraphicsView;

    void resizeEvent(QResizeEvent *event) override;
};

class BlockViewValidationInterface;

class GuiBlockView : public QDialog
{
    Q_OBJECT

public:
    RecursiveMutex m_mutex;

private:
    struct SceneElement {
        QGraphicsItem* gi;
        QPointF target_loc;
    };
    struct Bubble {
        QPointF pos;
        double radius;
        SceneElement *el;
    };
    struct BubbleGraph {
        std::vector<Bubble> bubbles;
        qreal min_x{0};
        qreal max_x{0};
        qreal min_y{0};
        bool instant;
        size_t txs_count;
        size_t txs_size{0};
    };
    std::map<Wtxid, SceneElement> m_elements GUARDED_BY(m_mutex);
    std::unique_ptr<BubbleGraph> m_bubblegraph GUARDED_BY(m_mutex);
    QGraphicsScene *m_scene;
    QTimer m_timer;
    unsigned int m_frame_div;

    QComboBox *m_block_chooser;
    QLabel *m_lbl_tx_count;
    QLabel *m_lbl_tx_fees;

    BlockViewValidationInterface *m_validation_interface;

    static bool any_overlap(const Bubble& proposed, const std::vector<Bubble>& others);

protected:
    void updateElements(bool instant) EXCLUSIVE_LOCKS_REQUIRED(m_mutex);
    void updateBlockFees(CAmount block_fees);

protected Q_SLOTS:
    void updateDisplayUnit();
    void updateSceneInit();
    void updateScene();

public:
    ClientModel *m_client_model{nullptr};

    bool m_block_changed GUARDED_BY(m_mutex);
    CAmount m_block_fees GUARDED_BY(m_mutex) {-1};
    std::shared_ptr<const node::CBlockTemplate> m_block_template GUARDED_BY(m_mutex);
    std::shared_ptr<const CBlock> m_block GUARDED_BY(m_mutex);
    std::atomic<bool> m_follow_tip;

    GuiBlockView(const PlatformStyle *, const NetworkStyle *, QWidget * parent = nullptr);
    ~GuiBlockView();

    void setClientModel(ClientModel *model);
    ChainstateManager* getChainstateManager() const;

    void clear();
    void setBlock(std::shared_ptr<const CBlock> block, CAmount block_subsidy);
    void setBlock(std::shared_ptr<const node::CBlockTemplate> blocktemplate);

public Q_SLOTS:
    void updateBestBlock(int height);
};

#endif // BITCOIN_QT_BLOCKVIEW_H
