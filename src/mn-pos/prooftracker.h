#include <map>
#include <set>

class BlockWitness;
class uint256;

typedef uint256 BLOCKHASH;
typedef uint256 STAKEHASH;

class ProofTracker
{
private:
    std::map<STAKEHASH, std::pair<int, std::set<BLOCKHASH> > > m_mapStakes; // {stakehash => [height, blockhash]}
    std::map<BLOCKHASH, std::set<BlockWitness>> m_mapBlockWitness;
    void AddNewStake(const STAKEHASH& hashStake, const BLOCKHASH& hashBlock, int nHeight);

public:
    std::set<BlockWitness> GetWitnesses(const uint256& hashBlock) const;
    bool HasSufficientProof(const BLOCKHASH& hashBlock) const;
    bool IsSuspicious(const STAKEHASH& hash, const BLOCKHASH& hashBlock, int nHeight);
    void AddWitness(const BlockWitness& witness);
    int GetWitnessCount(const BLOCKHASH& hashBlock) const;
    void EraseBeforeHeight(int nHeight);
};