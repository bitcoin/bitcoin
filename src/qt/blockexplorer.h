#ifndef BLOCKEXPLORER_H
#define BLOCKEXPLORER_H

#include <QWidget>

#include "base58.h"
#include "uint256.h"
#undef loop

namespace Ui
{
class BlockExplorer;
}

class CBlockIndex;
class CTransaction;
class PlatformStyle;
class CBlockTreeDB;

std::string getexplorerBlockHash(int64_t);
const CBlockIndex* getexplorerBlockIndex(int64_t);
CTxOut getPrevOut(const COutPoint& out);
void getNextIn(const COutPoint* Out, uint256* Hash, unsigned int n);

class BlockExplorer : public QWidget
{
    Q_OBJECT

public:
    explicit BlockExplorer(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~BlockExplorer();

protected:
    void keyPressEvent(QKeyEvent* event);
    void showEvent(QShowEvent*);

private Q_SLOTS:
    void onSearch();
    void goTo(const QString& query);
    void back();
    void forward();
//lib
private:
    Ui::BlockExplorer* ui;
    bool m_NeverShown;
    int m_HistoryIndex;
    QStringList m_History;

    void setBlock(CBlockIndex* pBlock);
    bool switchTo(const QString& query);
    void setContent(const std::string& content);
    void updateNavButtons();
    const PlatformStyle *platformStyle;
};

#endif // BLOCKEXPLORER_H
