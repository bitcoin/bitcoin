#ifndef BITCOIN_P2P_ROUTING_H
#define BITCOIN_P2P_ROUTING_H

namespace p2p {

/** Initialize experimental routing layer (no-op unless PoA enabled) */
void InitRouting();

/** Shutdown routing layer */
void ShutdownRouting();

} // namespace p2p

#endif // BITCOIN_P2P_ROUTING_H
