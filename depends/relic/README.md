![](https://github.com/relic-toolkit/relic/blob/master/art/rlc_logo.png)
=====

[![Project stats](https://www.openhub.net/p/relic-toolkit/widgets/project_thin_badge.gif)](https://www.openhub.net/p/relic-toolkit)
[![Code Quality: Cpp](https://img.shields.io/lgtm/grade/cpp/g/relic-toolkit/relic.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/relic-toolkit/relic/context:cpp)
[![Total Alerts](https://img.shields.io/lgtm/alerts/g/relic-toolkit/relic.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/relic-toolkit/relic/alerts)

[![Travis Status](https://travis-ci.org/relic-toolkit/relic.svg?branch=main)](https://travis-ci.org/relic-toolkit/relic)
[![GHA Status](https://github.com/relic-toolkit/relic/actions/workflows/easy.yml/badge.svg)](https://github.com/relic-toolkit/relic/actions/workflows/easy.yml)
[![GHA Status](https://github.com/relic-toolkit/relic/actions/workflows/gmp.yml/badge.svg)](https://github.com/relic-toolkit/relic/actions/workflows/gmp.yml)
[![GHA Status](https://github.com/relic-toolkit/relic/actions/workflows/bls12-381.yml/badge.svg)](https://github.com/relic-toolkit/relic/actions/workflows/bls12-381.yml)


RELIC is a modern research-oriented cryptographic meta-toolkit with emphasis on efficiency and flexibility. RELIC can be used to build efficient and usable cryptographic toolkits tailored for specific security levels and algorithmic choices.

### Goals

RELIC is an ongoing project and features will be added on demand. The focus is to provide:

 * Ease of portability and inclusion of architecture-dependent code
 * Simple experimentation with alternative implementations
 * Tests and benchmarks for every implemented function
 * Flexible configuration
 * Maximum efficiency

### Algorithms

RELIC implements to date:

 * Multiple-precision integer arithmetic
 * Prime and Binary field arithmetic
 * Elliptic curves over prime and binary fields (NIST curves and pairing-friendly curves)
 * Bilinear maps and related extension fields
 * Cryptographic protocols (RSA, Rabin, ECDSA, ECMQV, ECSS (Schnorr), ECIES, Sakai-Ohgishi-Kasahara ID-based authenticated key agreement, Boneh-Lynn-Schacham and Boneh-Boyen short signatures, Paillier and Benaloh homomorphic encryption systems)

### Citing

If you use RELIC, please cite using the template below:

    @misc{relic-toolkit,
        author = {D. F. Aranha and C. P. L. Gouvêa and T. Markmann and R. S. Wahby and K. Liao},        
        title = {{RELIC is an Efficient LIbrary for Cryptography}},
        howpublished = {\url{https://github.com/relic-toolkit/relic}},
    }

### Build instructions

Instructions for building the library can be found in the [Wiki](https://github.com/relic-toolkit/relic/wiki/Building).

### Support

You can probably get some help over the official mailing list at `relic-discuss@googlegroups.com`

If you like the library, please consider supporting development through [Paypal](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=R7D6ZE3BLMTF2&lc=BR&item_name=RELIC%20Development&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted).

### Licensing

This work is dual-licensed under Apache 2.0 and LGPL 2.1-or-above to encourage collaboration with other research groups and contributions from the industry. You can choose between one of them if you use this work.

`SPDX-License-Identifier: Apache-2.0 OR LGPL-2.1`

Starting from version 0.3.3, static linking and changes in the configuration or build system are explicitly exempted from representing derived works. Please refer to the LICENSE files for additional details.

### Disclaimer

RELIC is at most alpha-quality software. Implementations may not be correct or secure and may include patented algorithms. There are *many* configuration options which make the library horribly insecure. Backward API compatibility with early versions may not necessarily be maintained. Use at your own risk.
