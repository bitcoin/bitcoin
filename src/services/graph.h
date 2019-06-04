#ifndef SYSCOIN_SERVICES_GRAPH_H
#define SYSCOIN_SERVICES_GRAPH_H
#include <vector>
#include <primitives/transaction.h>

bool OrderBasedOnArrivalTime(std::vector<CTransactionRef>& blockVtx);
#endif // SYSCOIN_SERVICES_GRAPH_H