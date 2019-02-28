#include <map>
#include <set>

class BlockWitness;
class uint256;

typedef uint256 BLOCKHASH;
typedef uint256 STAKEHASH;

class ProofTracker
{
private:
    std::map<STAKEHASH, std::set<BLOCKHASH>> m_mapStakes;
    std::map<BLOCKHASH, std::set<BlockWitness>> m_mapBlockWitness;
    void AddNewStake(const STAKEHASH& hashStake, const BLOCKHASH& hashBlock);

public:
    bool IsSuspicious(const STAKEHASH& hash, const BLOCKHASH& hashBlock);
    void AddWitness(const BlockWitness& witness);
};