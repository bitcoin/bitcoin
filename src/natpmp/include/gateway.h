#ifndef BITCOIN_NATPMP_INCLUDE_GATEWAY_H
#define BITCOIN_NATPMP_INCLUDE_GATEWAY_H

#ifdef WIN32
#if !defined(_MSC_VER)
#include <stdint.h>
#endif
#endif
#define in_addr_t uint32_t
#include <cstdint>

/// @brief Get default gateway
///
/// @param addr Address
///
/// @return 0 Success, -1 Error
int GetDefaultGateway(in_addr_t * addr);
#endif // BITCOIN_NATPMP_INCLUDE_GATEWAY_H
