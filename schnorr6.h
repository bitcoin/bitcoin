#ifndef SIMPLICITY_SCHNORR6_H
#define SIMPLICITY_SCHNORR6_H

#include <stddef.h>
#include <stdint.h>
#include "bounded.h"

/* A length-prefixed encoding of the following Simplicity program:
 *     (scribe (toWord256 0xDFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659) &&&
 *      scribe (toWord256 0x5E2D58D8B3BCDF1ABADEC7829054F90DDA9805AAB56C77333024B9D0A508B75C)) &&&
 *      witness (toWord512 0xFFF97BD5755EEEA420453A14355235D382F6472F8568A18B2F057A14602975563CC27944640AC607CD107AE10923D9EF7A73C643E166BE5EBEAFA34B1AC553E2) >>>
 *     Simplicity.Programs.LibSecp256k1.Lib.bip_0340_verify
 * with jets.
 */
extern const unsigned char schnorr6[];
extern const size_t sizeof_schnorr6;
extern const unsigned char schnorr6_witness[];
extern const size_t sizeof_schnorr6_witness;

/* The commitment Merkle root of the above schnorr6 Simplicity expression. */
extern const uint32_t schnorr6_cmr[];

/* The identity hash of the root of the above schnorr6 Simplicity expression. */
extern const uint32_t schnorr6_ihr[];

/* The annotated Merkle root of the above schnorr6 Simplicity expression. */
extern const uint32_t schnorr6_amr[];

/* The cost of the above schnorr6 Simplicity expression in milli weight units. */
extern const ubounded schnorr6_cost;

#endif
