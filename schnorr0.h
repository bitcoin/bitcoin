#ifndef SIMPLICITY_SCHNORR0_H
#define SIMPLICITY_SCHNORR0_H

#include <stddef.h>
#include <stdint.h>
#include "bounded.h"

/* A length-prefixed encoding of the following Simplicity program:
 *     (scribe (toWord256 0xF9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9) &&&
 *      zero word256) &&&
 *      witness (toWord512 0xE907831F80848D1069A5371B402410364BDF1C5F8307B0084C55F1CE2DCA821525F66A4A85EA8B71E482A74F382D2CE5EBEEE8FDB2172F477DF4900D310536C0) >>>
 *     Simplicity.Programs.LibSecp256k1.Lib.bip_0340_verify
 * with jets.
 */
extern const unsigned char schnorr0[];
extern const size_t sizeof_schnorr0;
extern const unsigned char schnorr0_witness[];
extern const size_t sizeof_schnorr0_witness;

/* The commitment Merkle root of the above schnorr0 Simplicity expression. */
extern const uint32_t schnorr0_cmr[];

/* The identity hash of the root of the above schnorr0 Simplicity expression. */
extern const uint32_t schnorr0_ihr[];

/* The annotated Merkle root of the above schnorr0 Simplicity expression. */
extern const uint32_t schnorr0_amr[];

/* The cost of the above schnorr0 Simplicity expression in milli weight units. */
extern const ubounded schnorr0_cost;

#endif
