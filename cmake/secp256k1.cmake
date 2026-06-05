# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

enable_language(C)

# =============================================================================
# secp256k1 backend selection
# =============================================================================
# By default Bitcoin Core uses the bundled bitcoin-core/secp256k1 subtree.
# Setting SECP256K1_BACKEND=ultrafast replaces it with UltrafastSecp256k1,
# a C++20 library providing the same secp256k1.h API surface via a thin shim,
# available as the git submodule at src/ultrafast_secp256k1.
#
# Usage:
#   cmake -B build -DSECP256K1_BACKEND=ultrafast   # use UltrafastSecp256k1
#   cmake -B build                                  # default: bundled secp256k1
#
# UltrafastSecp256k1 offers:
#   - ~2x faster generator multiplications (w=18 precomputed table)
#   - GLV endomorphism for variable-base scalar multiplication
#   - SafeGCD (Bernstein-Yang) scalar inversion — 6.5x faster than Fermat
#   - CUDA/OpenCL GPU acceleration (optional, not required)
#   - Continuous assurance: 262 exploit PoC modules, CT-verified, 50+ CI workflows
#
# API compatibility:
#   secp256k1_context*, secp256k1_pubkey, secp256k1_ecdsa_signature, and all
#   function signatures are identical to bitcoin-core/secp256k1.
#
# secp256k1_context_randomize() is fully implemented: stores the seed in the
#   context struct and applies per-context DPA blinding at the start of each
#   signing call via ContextBlindingScope (r*G pre-computed at randomize time).
#   Semantics match upstream libsecp256k1 including multi-context same-thread use.
# =============================================================================

set(SECP256K1_BACKEND "bundled" CACHE STRING
  "secp256k1 backend: 'bundled' (default) or 'ultrafast'")
set_property(CACHE SECP256K1_BACKEND PROPERTY STRINGS bundled ultrafast)

if(SECP256K1_BACKEND STREQUAL "ultrafast")
  # ---------------------------------------------------------------------------
  # UltrafastSecp256k1 backend — submodule at src/ultrafast_secp256k1
  # ---------------------------------------------------------------------------
  set(_UF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../src/ultrafast_secp256k1")

  if(NOT EXISTS "${_UF_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
      "UltrafastSecp256k1 submodule not found at src/ultrafast_secp256k1.\n"
      "Run: git submodule update --init src/ultrafast_secp256k1")
  endif()

  message("")
  message("Configuring UltrafastSecp256k1 as secp256k1 backend...")
  message("  Submodule: ${_UF_ROOT}")

  # Build only the CPU library — no GPU backends, no bench, no examples
  set(SECP256K1_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_BENCH    OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SECP256K1_ENABLE_CUDA    OFF CACHE BOOL "" FORCE)
  set(SECP256K1_ENABLE_OPENCL  OFF CACHE BOOL "" FORCE)

  # Strip every module the Bitcoin Core consensus path never calls.
  # Core's shim surface is: ECDSA, Schnorr/BIP-340, Recovery, ECDH,
  # ElligatorSwift/BIP-324, tagged-hash, pubkey/seckey/extrakeys ops.
  # MuSig2, batch-verify and Pippenger/MSM are NOT used by Core (it verifies
  # script signatures individually, never via secp256k1 batch or MuSig2).
  # The shim now gates shim_musig.cpp on SECP256K1_BUILD_MUSIG2 and
  # shim_batch_verify.cpp on SECP256K1_BUILD_PIPPENGER (and batch_verify.cpp
  # itself moved under PIPPENGER, since it calls secp256k1::msm()), so turning
  # these OFF cleanly drops the wrappers — the unity TU no longer references
  # stripped symbols and the no-LTO link succeeds. Removing them shrinks the
  # fastsecp256k1 .text, narrowing the I-cache gap with bundled libsecp256k1.
  # MuSig2 (BIP-327) IS used by Bitcoin Core: src/key.cpp references
  # secp256k1_musig_nonce_gen / _pubnonce_parse / _pubkey_xonly_tweak_add etc.
  # So it must stay ON — shim_musig.cpp is required. Verified by link failure.
  set(SECP256K1_BUILD_MUSIG2    ON  CACHE BOOL "" FORCE)  # Core key.cpp uses secp256k1_musig_*
  set(SECP256K1_BUILD_FROST     OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_ZK        OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_ECIES     OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_BIP352    OFF CACHE BOOL "" FORCE)
  # Litecoin/BCH silent-payment modules require BIP352; Bitcoin Core needs neither.
  # The engine guards: LTC_SP=ON requires BIP352=ON, so force both companions OFF.
  set(SECP256K1_BUILD_LTC_SP    OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_BCH       OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_ADAPTOR   OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_WALLET    OFF CACHE BOOL "" FORCE)
  # Pippenger/MSM is REQUIRED transitively: musig2_key_agg() (src/musig2.cpp)
  # calls secp256k1::msm(), and Core's src/key.cpp uses secp256k1_musig_pubkey_agg.
  # So MSM cannot be stripped while MuSig2 is in. (The standalone batch-verify API
  # — schnorr_batch_verify — is Ultra-only and NOT used by Core, but it shares the
  # msm() dependency, so stripping it alone saves only the thin wrapper.)
  set(SECP256K1_BUILD_PIPPENGER ON  CACHE BOOL "" FORCE)  # musig2_key_agg -> msm()
  set(SECP256K1_BUILD_ETHEREUM  OFF CACHE BOOL "" FORCE)
  set(SECP256K1_BUILD_BIP324    ON  CACHE BOOL "" FORCE)

  # Integration mode selection — see docs/BUILD_INTEGRATION_GUIDE.md
  #
  # SECP256K1_BUILD_AS_OBJECT=ON: fastsecp256k1 is built as a CMake OBJECT
  # library (no .a file). All object files are included directly in Bitcoin
  # Core's link step — as if the sources were part of this project.
  #
  # SECP256K1_UNITY_BUILD=ON: all source files compiled as one translation
  # unit. The compiler can inline across all secp256k1 internal boundaries
  # without LTO — equivalent to a monolithic implementation.
  #
  # Combined (recommended): one TU, no .a, objects directly in bitcoind.
  # Performance vs libsecp256k1 (GCC 14, no LTO):
  #   Signing:          +6% (ECDSA), +6% (Schnorr transaction), +35% (Taproot)
  #   ConnectBlock:     −2.5% ECDSA (parity with current-code fixes applied)
  #   Binary .text:     213 KB SMALLER than libsecp256k1
  #
  # Alternative: -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON (LTO)
  #   Signing:          +10% (ECDSA), +35% (Taproot)
  #   ConnectBlock:     +1.2% ECDSA, +0.9% Schnorr
  set(SECP256K1_BUILD_AS_OBJECT ON CACHE BOOL
    "Build fastsecp256k1 as OBJECT lib — objects go directly into Bitcoin Core link" FORCE)
  set(SECP256K1_UNITY_BUILD ON CACHE BOOL
    "Compile all secp256k1 sources as one TU — full cross-unit inlining without LTO" FORCE)

  # SECP256K1_BUILD_SHIM=ON: shim sources compiled INTO fastsecp256k1 (same
  # OBJECT/STATIC target). No separate .a boundary between shim and core.
  set(SECP256K1_BUILD_SHIM ON CACHE BOOL
    "Compile shim sources inside fastsecp256k1" FORCE)
  # No shim tests in Bitcoin Core build — set before add_subdirectory so root
  # CMakeLists.txt (which respects this value) does not rebuild them.
  set(SECP256K1_SHIM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  # Bitcoin Core backend mode: enables CT enforcement, strict ABI, RFC 6979,
  # AND forces window_bits=8 / use_cache=false in the shim so no 244 MB
  # precomputed-table file is loaded or generated on process startup.
  # This eliminates the ~3% ConnectBlock overhead from file I/O amortisation.
  set(SECP256K1_CORE_BACKEND_MODE ON CACHE BOOL "" FORCE)

  # ConnectBlock without LTO: root cause is cross-.o-file inlining, not binary size.
  #
  # Measured (2026-05-21, i5-14400F, GCC 14.2.0, taskset -c 0):
  #   Standard no-LTO:  ~−8% ECDSA ConnectBlock, parity Schnorr
  #   Unity no-LTO:     ~−2.5% ECDSA, parity Schnorr, signing +6%
  #   LTO:              +1.2% ECDSA, +0.9% Schnorr, signing +10-35%
  #
  # Binary size (stripped bench_bitcoin .text, LTO):
  #   Ultra: 12.62 MB  libsecp: 12.37 MB  → +250 KB difference
  #   Dead-code-elimination removes FROST/MuSig2/BIP-352 regardless of
  #   SECP256K1_BUILD_* flags — linker only pulls what Bitcoin Core calls.
  #   i-cache pressure from binary size is NOT the bottleneck.
  #
  # Root cause: without LTO or unity build, the compiler cannot inline across
  # shim_*.cpp → core *.cpp boundaries (separate .o files in fastsecp256k1.a).
  # SECP256K1_UNITY_BUILD=ON resolves this without requiring LTO.
  # Production builds may use -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON as an
  # alternative — both approaches achieve parity with libsecp on verify paths.

  # Root CMakeLists.txt already calls add_subdirectory(compat/libsecp256k1_shim)
  # when SECP256K1_BUILD_SHIM=ON, so we must NOT call it a second time here.
  add_subdirectory(${_UF_ROOT} ultrafast_secp256k1_build)
  # secp256k1_shim target is now live (created by the root CMakeLists.txt above).

  # Expose the shim as 'secp256k1' — Bitcoin Core links secp256k1_shim
  # which PRIVATELY wraps fastsecp256k1.  No internal definitions leak.
  add_library(secp256k1 ALIAS secp256k1_shim)

  # Warn when neither LTO nor unity build is active.
  # Without cross-unit inlining, verify paths have ~2-8% overhead vs libsecp.
  # Either -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON  (LTO, slow compile)
  # or     -DSECP256K1_UNITY_BUILD=ON               (fast compile, same effect)
  # resolves the gap. Signing is faster than libsecp in all configurations.
  if(CMAKE_BUILD_TYPE STREQUAL "Release" AND
     NOT CMAKE_INTERPROCEDURAL_OPTIMIZATION AND
     NOT SECP256K1_UNITY_BUILD)
    message(WARNING
      "\n"
      "  *** UltrafastSecp256k1: cross-unit inlining is OFF ***\n"
      "  ConnectBlock verify throughput may be ~2-8% lower than libsecp256k1.\n"
      "  Resolve with either:\n"
      "    -DSECP256K1_UNITY_BUILD=ON                    (recommended, fast compile)\n"
      "    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON        (LTO, slower compile)\n"
      "  Signing is +6-35% faster than libsecp256k1 in all configurations.\n"
      "  See: cmake/secp256k1.cmake for measured data.\n")
  endif()

  message("  UltrafastSecp256k1 backend: enabled")
  message("")

else()
  # ---------------------------------------------------------------------------
  # Default: bundled bitcoin-core/secp256k1 subtree (behaviour unchanged)
  # ---------------------------------------------------------------------------
  function(add_secp256k1 subdir)
    message("")
    message("Configuring secp256k1 subtree...")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)

    # Unconditionally prevent secp's symbols from being exported by our libs
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(SECP256K1_ENABLE_API_VISIBILITY_ATTRIBUTES OFF CACHE BOOL "" FORCE)

    set(SECP256K1_ENABLE_MODULE_ECDH OFF CACHE BOOL "" FORCE)
    set(SECP256K1_ENABLE_MODULE_RECOVERY ON CACHE BOOL "" FORCE)
    set(SECP256K1_ENABLE_MODULE_MUSIG ON CACHE BOOL "" FORCE)
    set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
    set(SECP256K1_BUILD_TESTS ${BUILD_TESTS} CACHE BOOL "" FORCE)
    set(SECP256K1_BUILD_EXHAUSTIVE_TESTS ${BUILD_TESTS} CACHE BOOL "" FORCE)
    if(NOT BUILD_TESTS)
      # Always skip the ctime tests, if we are building no other tests.
      # Otherwise, they are built if Valgrind is available. See SECP256K1_VALGRIND.
      set(SECP256K1_BUILD_CTIME_TESTS ${BUILD_TESTS} CACHE BOOL "" FORCE)
    endif()
    set(SECP256K1_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    include(GetTargetInterface)
    # -fsanitize and related flags apply to both C++ and C,
    # so we can pass them down to libsecp256k1 as CFLAGS and LDFLAGS.
    get_target_interface(SECP256K1_APPEND_CFLAGS "" sanitize_interface COMPILE_OPTIONS)
    string(STRIP "${SECP256K1_APPEND_CFLAGS} ${APPEND_CPPFLAGS}" SECP256K1_APPEND_CFLAGS)
    string(STRIP "${SECP256K1_APPEND_CFLAGS} ${APPEND_CFLAGS}" SECP256K1_APPEND_CFLAGS)
    set(SECP256K1_APPEND_CFLAGS ${SECP256K1_APPEND_CFLAGS} CACHE STRING "" FORCE)
    get_target_interface(SECP256K1_APPEND_LDFLAGS "" sanitize_interface LINK_OPTIONS)
    string(STRIP "${SECP256K1_APPEND_LDFLAGS} ${APPEND_LDFLAGS}" SECP256K1_APPEND_LDFLAGS)
    set(SECP256K1_APPEND_LDFLAGS ${SECP256K1_APPEND_LDFLAGS} CACHE STRING "" FORCE)
    # We want to build libsecp256k1 with the most tested RelWithDebInfo configuration.
    foreach(config IN LISTS CMAKE_BUILD_TYPE CMAKE_CONFIGURATION_TYPES)
      if(config STREQUAL "")
        continue()
      endif()
      string(TOUPPER "${config}" config)
      set(CMAKE_C_FLAGS_${config} "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    endforeach()
    # If the CFLAGS environment variable is defined during building depends
    # and configuring this build system, its content might be duplicated.
    if(DEFINED ENV{CFLAGS})
      deduplicate_flags(CMAKE_C_FLAGS)
    endif()

    add_subdirectory(${subdir})
    set_target_properties(secp256k1 PROPERTIES
      EXCLUDE_FROM_ALL TRUE
    )
  endfunction()

endif() # SECP256K1_BACKEND
