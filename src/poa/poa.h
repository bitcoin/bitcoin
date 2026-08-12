// Prototype PoA/BFT module (feature-flagged prototype)
#ifndef BITCOIN_POA_POA_H
#define BITCOIN_POA_POA_H

namespace poa {

/** Initialize PoA subsystem (no-op unless -enable-poa is set) */
void InitPoA();

/** Shutdown PoA subsystem */
void ShutdownPoA();

/** Return whether PoA is enabled at runtime */
bool IsPoAEnabled();

} // namespace poa

#endif // BITCOIN_POA_POA_H
