/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2009 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

/**
 * @mainpage
 *
 * RELIC is a modern cryptographic meta-toolkit with emphasis on efficiency and
 * flexibility. RELIC can be used to build efficient and usable cryptographic
 * toolkits tailored for specific security levels and algorithmic choices.
 *
 * @section goals_sec Goals
 *
 * RELIC is an ongoing project and features will be added on demand.
 * The focus is to provide:
 *
 * <ul>
 * <li> Ease of portability and inclusion of architecture-dependent code
 * <li> Simple experimentation with alternative implementations
 * <li> Tests and benchmarks for every implemented function
 * <li> Flexible configuration
 * <li> Maximum efficiency
 * </ul>
 *
 * @section algo_sec Algorithms
 *
 * RELIC implements to date:
 *
 * <ul>
 * <li> Multiple-precision integer arithmetic
 * <li> Prime and Binary field arithmetic
 * <li> Elliptic curves over prime and binary fields (NIST curves and
 * pairing-friendly curves)
 * <li> Bilinear maps and related extension fields
 * <li> Cryptographic protocols
 * </ul>
 *
 * @section lic_sec Licensing
 *
 * RELIC is dual-licensed under Apache 2.0 and LGPL 2.1-or-above to encourage
 * collaboration with other research groups and contributions from the industry.
 * You can choose between one of them.
 *
 * @section disc_sec Disclaimer
 *
 * RELIC is at most alpha-quality software. Implementations may not be correct
 * or secure and may include patented algorithms. There are many configuration
 * options which make the library horribly insecure. Backward API compatibility
 * with early versions may not necessarily be maintained. Use at your own risk.
 */

/**
 * @file
 *
 * Library interface.
 *
 */

#ifndef RELIC_H
#define RELIC_H

#include "relic_arch.h"
#include "relic_conf.h"
#include "relic_core.h"
#include "relic_multi.h"
#include "relic_types.h"
#include "relic_bn.h"
#include "relic_dv.h"
#include "relic_fp.h"
#include "relic_fpx.h"
#include "relic_fb.h"
#include "relic_fbx.h"
#include "relic_ep.h"
#include "relic_eb.h"
#include "relic_ed.h"
#include "relic_ec.h"
#include "relic_pp.h"
#include "relic_pc.h"
#include "relic_cp.h"
#include "relic_bc.h"
#include "relic_md.h"
#include "relic_mpc.h"
#include "relic_err.h"
#include "relic_rand.h"
#include "relic_util.h"

#endif /* !RELIC_H */
