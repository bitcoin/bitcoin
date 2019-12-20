
#ifndef SYSCOIN_SERVICES_GRAPH_H
#define SYSCOIN_SERVICES_GRAPH_H
#include <vector>
#include <primitives/transaction.h>

bool OrderBasedOnArrivals(std::vector <CTransactionRef> & blockVtx);
#endif // SYSCOIN_SERVICES_GRAPH_H