#ifndef GRAPH_H
#define GRAPH_H
#include <vector>
#include <primitives/transaction.h>

bool OrderBasedOnArrivalTime(std::vector<CTransactionRef>& blockVtx);
#endif // GRAPH_H