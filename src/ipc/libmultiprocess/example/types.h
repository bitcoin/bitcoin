// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EXAMPLE_TYPES_H
#define EXAMPLE_TYPES_H

#include <calculator.capnp.proxy-types.h>
#include <printer.capnp.proxy-types.h>

// IWYU pragma: begin_exports
#include <mp/type-context.h>
#include <mp/type-decay.h>
#include <mp/type-interface.h>
#include <mp/type-string.h>
#include <mp/type-threadmap.h>
// IWYU pragma: end_exports

struct InitInterface; // IWYU pragma: export
struct CalculatorInterface; // IWYU pragma: export
struct PrinterInterface; // IWYU pragma: export

#endif // EXAMPLE_TYPES_H
