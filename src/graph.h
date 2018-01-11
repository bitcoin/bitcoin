#ifndef GRAPH_H
#define GRAPH_H
#include <vector>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/hawick_circuits.hpp>
#include <boost/next_prior.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <unordered_map>
#include <unordered_set>
#include "miner.h"
using namespace std;
unsigned int DAGRemoveCycles(CBlock & pblock, unique_ptr<CBlockTemplate> &pblocktemplate, uint64_t &nBlockTx, uint64_t &nBlockSize, unsigned int &nBlockSigOps, CAmount &nFees);
bool DAGTopologicalSort(CBlock & pblock);
#endif // GRAPH_H