/* $Id: sph_types.h 260 2011-07-21 01:02:38Z tp $ */
/**
 * Basic type definitions.
 *
 * This header file defines the generic integer types that will be used
 * for the implementation of hash functions; it also contains helper
 * functions which encode and decode multi-byte integer values, using
 * either little-endian or big-endian conventions.
 *
 * This file contains a compile-time test on the size of a byte
 * (the <code>unsigned char</code> C type). If bytes are not octets,
 * i.e. if they do not have a size of exactly 8 bits, then compilation
 * is aborted. Architectures where bytes are not octets are relatively
 * rare, even in the embedded devices market. We forbid non-octet bytes
 * because there is no clear convention on how octet streams are encoded
 * on such systems.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2007-2010  Projet RNRT SAPHIR
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @file     sph_types.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_TYPES_H__
#define SPH_TYPES_H__

#include <limits.h>

/*
 * All our I/O functions are defined over octet streams. We do not know
 * how to handle input data if bytes are not octets.
 */
#if CHAR_BIT != 8
#error This code requires 8-bit bytes
#endif

/* ============= BEGIN documentation block for Doxygen ============ */

#ifdef DOXYGEN_IGNORE

/** @mainpage sphlib C code documentation
 *
 * @section overview Overview
 *
 * <code>sphlib</code> is a library which contains implementations of
 * various cryptographic hash functions. These pages have been generated
 * with <a href="http://www.doxygen.org/index.html">doxygen</a> and
 * document the API for the C implementations.
 *
 * The API is described in appropriate header files, which are available
 * in the "Files" section. Each hash function family has its own header,
 * whose name begins with <code>"sph_"</code> and contains the family
 * name. For instance, the API for the RIPEMD hash functions is available
 * in the header file <code>sph_ripemd.h</code>.
 *
 * @section principles API structure and conventions
 *
 * @subsection io Input/output conventions
 *
 * In all generality, hash functions operate over strings of bits.
 * Individual bits are rarely encountered in C programming or actual
 * communication protocols; most protocols converge on the ubiquitous
 * "octet" which is a group of eight bits. Data is thus expressed as a
 * stream of octets. The C programming language contains the notion of a
 * "byte", which is a data unit managed under the type <code>"unsigned
 * char"</code>. The C standard prescribes that a byte should hold at
 * least eight bits, but possibly more. Most modern architectures, even
 * in the embedded world, feature eight-bit bytes, i.e. map bytes to
 * octets.
 *
 * Nevertheless, for some of the implemented hash functions, an extra
 * API has been added, which allows the input of arbitrary sequences of
 * bits: when the computation is about to be closed, 1 to 7 extra bits
 * can be added. The functions for which this API is implemented include
 * the SHA-2 functions and all SHA-3 candidates.
 *
 * <code>sphlib</code> defines hash function which may hash octet streams,
 * i.e. streams of bits where the number of bits is a multiple of eight.
 * The data input functions in the <code>sphlib</code> API expect data
 * as anonymous pointers (<code>"const void *"</code>) with a length
 * (of type <code>"size_t"</code>) which gives the input data chunk length
 * in bytes. A byte is assumed to be an octet; the <code>sph_types.h</code>
 * header contains a compile-time test which prevents compilation on
 * architectures where this property is not met.
 *
 * The hash function output is also converted into bytes. All currently
 * implemented hash functions have an output width which is a multiple of
 * eight, and this is likely to remain true for new designs.
 *
 * Most hash functions internally convert input data into 32-bit of 64-bit
 * words, using either little-endian or big-endian conversion. The hash
 * output also often consists of such words, which are encoded into output
 * bytes with a similar endianness convention. Some hash functions have
 * been only loosely specified on that subject; when necessary,
 * <code>sphlib</code> has been tested against published "reference"
 * implementations in order to use the same conventions.
 *
 * @subsection shortname Function short name
 *
 * Each implemented hash function has a "short name" which is used
 * internally to derive the identifiers for the functions and context
 * structures which the function uses. For instance, MD5 has the short
 * name <code>"md5"</code>. Short names are listed in the next section,
 * for the implemented hash functions. In subsequent sections, the
 * short name will be assumed to be <code>"XXX"</code>: replace with the
 * actual hash function name to get the C identifier.
 *
 * Note: some functions within the same family share the same core
 * elements, such as update function or context structure. Correspondingly,
 * some of the defined types or functions may actually be macros which
 * transparently evaluate to another type or function name.
 *
 * @subsection context Context structure
 *
 * Each implemented hash fonction has its own context structure, available
 * under the type name <code>"sph_XXX_context"</code> for the hash function
 * with short name <code>"XXX"</code>. This structure holds all needed
 * state for a running hash computation.
 *
 * The contents of these structures are meant to be opaque, and private
 * to the implementation. However, these contents are specified in the
 * header files so that application code which uses <code>sphlib</code>
 * may access the size of those structures.
 *
 * The caller is responsible for allocating the context structure,
 * whether by dynamic allocation (<code>malloc()</code> or equivalent),
 * static allocation (a global permanent variable), as an automatic
 * variable ("on the stack"), or by any other mean which ensures proper
 * structure alignment. <code>sphlib</code> code performs no dynamic
 * allocation by itself.
 *
 * The context must be initialized before use, using the
 * <code>sph_XXX_init()</code> function. This function sets the context
 * state to proper initial values for hashing.
 *
 * Since all state data is contained within the context structure,
 * <code>sphlib</code> is thread-safe and reentrant: several hash
 * computations may be performed in parallel, provided that they do not
 * operate on the same context. Moreover, a running computation can be
 * cloned by copying the context (with a simple <code>memcpy()</code>):
 * the context and its clone are then independant and may be updated
 * with new data and/or closed without interfering with each other.
 * Similarly, a context structure can be moved in memory at will:
 * context structures contain no pointer, in particular no pointer to
 * themselves.
 *
 * @subsection dataio Data input
 *
 * Hashed data is input with the <code>sph_XXX()</code> fonction, which
 * takes as parameters a pointer to the context, a pointer to the data
 * to hash, and the number of data bytes to hash. The context is updated
 * with the new data.
 *
 * Data can be input in one or several calls, with arbitrary input lengths.
 * However, it is best, performance wise, to input data by relatively big
 * chunks (say a few kilobytes), because this allows <code>sphlib</code> to
 * optimize things and avoid internal copying.
 *
 * When all data has been input, the context can be closed with
 * <code>sph_XXX_close()</code>. The hash output is computed and written
 * into the provided buffer. The caller must take care to provide a
 * buffer of appropriate length; e.g., when using SHA-1, the output is
 * a 20-byte word, therefore the output buffer must be at least 20-byte
 * long.
 *
 * For some hash functions, the <code>sph_XXX_addbits_and_close()</code>
 * function can be used instead of <code>sph_XXX_close()</code>. This
 * function can take a few extra <strong>bits</strong> to be added at
 * the end of the input message. This allows hashing messages with a
 * bit length which is not a multiple of 8. The extra bits are provided
 * as an unsigned integer value, and a bit count. The bit count must be
 * between 0 and 7, inclusive. The extra bits are provided as bits 7 to
 * 0 (bits of numerical value 128, 64, 32... downto 0), in that order.
 * For instance, to add three bits of value 1, 1 and 0, the unsigned
 * integer will have value 192 (1*128 + 1*64 + 0*32) and the bit count
 * will be 3.
 *
 * The <code>SPH_SIZE_XXX</code> macro is defined for each hash function;
 * it evaluates to the function output size, expressed in bits. For instance,
 * <code>SPH_SIZE_sha1</code> evaluates to <code>160</code>.
 *
 * When closed, the context is automatically reinitialized and can be
 * immediately used for another computation. It is not necessary to call
 * <code>sph_XXX_init()</code> after a close. Note that
 * <code>sph_XXX_init()</code> can still be called to "reset" a context,
 * i.e. forget previously input data, and get back to the initial state.
 *
 * @subsection alignment Data alignment
 *
 * "Alignment" is a property of data, which is said to be "properly
 * aligned" when its emplacement in memory is such that the data can
 * be optimally read by full words. This depends on the type of access;
 * basically, some hash functions will read data by 32-bit or 64-bit
 * words. <code>sphlib</code> does not mandate such alignment for input
 * data, but using aligned data can substantially improve performance.
 *
 * As a rule, it is best to input data by chunks whose length (in bytes)
 * is a multiple of eight, and which begins at "generally aligned"
 * addresses, such as the base address returned by a call to
 * <code>malloc()</code>.
 *
 * @section functions Implemented functions
 *
 * We give here the list of implemented functions. They are grouped by
 * family; to each family corresponds a specific header file. Each
 * individual function has its associated "short name". Please refer to
 * the documentation for that header file to get details on the hash
 * function denomination and provenance.
 *
 * Note: the functions marked with a '(64)' in the list below are
 * available only if the C compiler provides an integer type of length
 * 64 bits or more. Such a type is mandatory in the latest C standard
 * (ISO 9899:1999, aka "C99") and is present in several older compilers
 * as well, so chances are that such a type is available.
 *
 * - HAVAL family: file <code>sph_haval.h</code>
 *   - HAVAL-128/3 (128-bit, 3 passes): short name: <code>haval128_3</code>
 *   - HAVAL-128/4 (128-bit, 4 passes): short name: <code>haval128_4</code>
 *   - HAVAL-128/5 (128-bit, 5 passes): short name: <code>haval128_5</code>
 *   - HAVAL-160/3 (160-bit, 3 passes): short name: <code>haval160_3</code>
 *   - HAVAL-160/4 (160-bit, 4 passes): short name: <code>haval160_4</code>
 *   - HAVAL-160/5 (160-bit, 5 passes): short name: <code>haval160_5</code>
 *   - HAVAL-192/3 (192-bit, 3 passes): short name: <code>haval192_3</code>
 *   - HAVAL-192/4 (192-bit, 4 passes): short name: <code>haval192_4</code>
 *   - HAVAL-192/5 (192-bit, 5 passes): short name: <code>haval192_5</code>
 *   - HAVAL-224/3 (224-bit, 3 passes): short name: <code>haval224_3</code>
 *   - HAVAL-224/4 (224-bit, 4 passes): short name: <code>haval224_4</code>
 *   - HAVAL-224/5 (224-bit, 5 passes): short name: <code>haval224_5</code>
 *   - HAVAL-256/3 (256-bit, 3 passes): short name: <code>haval256_3</code>
 *   - HAVAL-256/4 (256-bit, 4 passes): short name: <code>haval256_4</code>
 *   - HAVAL-256/5 (256-bit, 5 passes): short name: <code>haval256_5</code>
 * - MD2: file <code>sph_md2.h</code>, short name: <code>md2</code>
 * - MD4: file <code>sph_md4.h</code>, short name: <code>md4</code>
 * - MD5: file <code>sph_md5.h</code>, short name: <code>md5</code>
 * - PANAMA: file <code>sph_panama.h</code>, short name: <code>panama</code>
 * - RadioGatun family: file <code>sph_radiogatun.h</code>
 *   - RadioGatun[32]: short name: <code>radiogatun32</code>
 *   - RadioGatun[64]: short name: <code>radiogatun64</code> (64)
 * - RIPEMD family: file <code>sph_ripemd.h</code>
 *   - RIPEMD: short name: <code>ripemd</code>
 *   - RIPEMD-128: short name: <code>ripemd128</code>
 *   - RIPEMD-160: short name: <code>ripemd160</code>
 * - SHA-0: file <code>sph_sha0.h</code>, short name: <code>sha0</code>
 * - SHA-1: file <code>sph_sha1.h</code>, short name: <code>sha1</code>
 * - SHA-2 family, 32-bit hashes: file <code>sph_sha2.h</code>
 *   - SHA-224: short name: <code>sha224</code>
 *   - SHA-256: short name: <code>sha256</code>
 *   - SHA-384: short name: <code>sha384</code> (64)
 *   - SHA-512: short name: <code>sha512</code> (64)
 * - Tiger family: file <code>sph_tiger.h</code>
 *   - Tiger: short name: <code>tiger</code> (64)
 *   - Tiger2: short name: <code>tiger2</code> (64)
 * - WHIRLPOOL family: file <code>sph_whirlpool.h</code>
 *   - WHIRLPOOL-0: short name: <code>whirlpool0</code> (64)
 *   - WHIRLPOOL-1: short name: <code>whirlpool1</code> (64)
 *   - WHIRLPOOL: short name: <code>whirlpool</code> (64)
 *
 * The fourteen second-round SHA-3 candidates are also implemented;
 * when applicable, the implementations follow the "final" specifications
 * as published for the third round of the SHA-3 competition (BLAKE,
 * Groestl, JH, Keccak and Skein have been tweaked for third round).
 *
 * - BLAKE family: file <code>sph_blake.h</code>
 *   - BLAKE-224: short name: <code>blake224</code>
 *   - BLAKE-256: short name: <code>blake256</code>
 *   - BLAKE-384: short name: <code>blake384</code>
 *   - BLAKE-512: short name: <code>blake512</code>
 * - BMW (Blue Midnight Wish) family: file <code>sph_bmw.h</code>
 *   - BMW-224: short name: <code>bmw224</code>
 *   - BMW-256: short name: <code>bmw256</code>
 *   - BMW-384: short name: <code>bmw384</code> (64)
 *   - BMW-512: short name: <code>bmw512</code> (64)
 * - CubeHash family: file <code>sph_cubehash.h</code> (specified as
 *   CubeHash16/32 in the CubeHash specification)
 *   - CubeHash-224: short name: <code>cubehash224</code>
 *   - CubeHash-256: short name: <code>cubehash256</code>
 *   - CubeHash-384: short name: <code>cubehash384</code>
 *   - CubeHash-512: short name: <code>cubehash512</code>
 * - ECHO family: file <code>sph_echo.h</code>
 *   - ECHO-224: short name: <code>echo224</code>
 *   - ECHO-256: short name: <code>echo256</code>
 *   - ECHO-384: short name: <code>echo384</code>
 *   - ECHO-512: short name: <code>echo512</code>
 * - Fugue family: file <code>sph_fugue.h</code>
 *   - Fugue-224: short name: <code>fugue224</code>
 *   - Fugue-256: short name: <code>fugue256</code>
 *   - Fugue-384: short name: <code>fugue384</code>
 *   - Fugue-512: short name: <code>fugue512</code>
 * - Groestl family: file <code>sph_groestl.h</code>
 *   - Groestl-224: short name: <code>groestl224</code>
 *   - Groestl-256: short name: <code>groestl256</code>
 *   - Groestl-384: short name: <code>groestl384</code>
 *   - Groestl-512: short name: <code>groestl512</code>
 * - Hamsi family: file <code>sph_hamsi.h</code>
 *   - Hamsi-224: short name: <code>hamsi224</code>
 *   - Hamsi-256: short name: <code>hamsi256</code>
 *   - Hamsi-384: short name: <code>hamsi384</code>
 *   - Hamsi-512: short name: <code>hamsi512</code>
 * - JH family: file <code>sph_jh.h</code>
 *   - JH-224: short name: <code>jh224</code>
 *   - JH-256: short name: <code>jh256</code>
 *   - JH-384: short name: <code>jh384</code>
 *   - JH-512: short name: <code>jh512</code>
 * - Keccak family: file <code>sph_keccak.h</code>
 *   - Keccak-224: short name: <code>keccak224</code>
 *   - Keccak-256: short name: <code>keccak256</code>
 *   - Keccak-384: short name: <code>keccak384</code>
 *   - Keccak-512: short name: <code>keccak512</code>
 * - Luffa family: file <code>sph_luffa.h</code>
 *   - Luffa-224: short name: <code>luffa224</code>
 *   - Luffa-256: short name: <code>luffa256</code>
 *   - Luffa-384: short name: <code>luffa384</code>
 *   - Luffa-512: short name: <code>luffa512</code>
 * - Shabal family: file <code>sph_shabal.h</code>
 *   - Shabal-192: short name: <code>shabal192</code>
 *   - Shabal-224: short name: <code>shabal224</code>
 *   - Shabal-256: short name: <code>shabal256</code>
 *   - Shabal-384: short name: <code>shabal384</code>
 *   - Shabal-512: short name: <code>shabal512</code>
 * - SHAvite-3 family: file <code>sph_shavite.h</code>
 *   - SHAvite-224 (nominally "SHAvite-3 with 224-bit output"):
 *     short name: <code>shabal224</code>
 *   - SHAvite-256 (nominally "SHAvite-3 with 256-bit output"):
 *     short name: <code>shabal256</code>
 *   - SHAvite-384 (nominally "SHAvite-3 with 384-bit output"):
 *     short name: <code>shabal384</code>
 *   - SHAvite-512 (nominally "SHAvite-3 with 512-bit output"):
 *     short name: <code>shabal512</code>
 * - SIMD family: file <code>sph_simd.h</code>
 *   - SIMD-224: short name: <code>simd224</code>
 *   - SIMD-256: short name: <code>simd256</code>
 *   - SIMD-384: short name: <code>simd384</code>
 *   - SIMD-512: short name: <code>simd512</code>
 * - Skein family: file <code>sph_skein.h</code>
 *   - Skein-224 (nominally specified as Skein-512-224): short name:
 *     <code>skein224</code> (64)
 *   - Skein-256 (nominally specified as Skein-512-256): short name:
 *     <code>skein256</code> (64)
 *   - Skein-384 (nominally specified as Skein-512-384): short name:
 *     <code>skein384</code> (64)
 *   - Skein-512 (nominally specified as Skein-512-512): short name:
 *     <code>skein512</code> (64)
 *
 * For the second-round SHA-3 candidates, the functions are as specified
 * for round 2, i.e. with the "tweaks" that some candidates added
 * between round 1 and round 2. Also, some of the submitted packages for
 * round 2 contained errors, in the specification, reference code, or
 * both. <code>sphlib</code> implements the corrected versions.
 */

/** @hideinitializer
 * Unsigned integer type whose length is at least 32 bits; on most
 * architectures, it will have a width of exactly 32 bits. Unsigned C
 * types implement arithmetics modulo a power of 2; use the
 * <code>SPH_T32()</code> macro to ensure that the value is truncated
 * to exactly 32 bits. Unless otherwise specified, all macros and
 * functions which accept <code>sph_u32</code> values assume that these
 * values fit on 32 bits, i.e. do not exceed 2^32-1, even on architectures
 * where <code>sph_u32</code> is larger than that.
 */
typedef __arch_dependant__ sph_u32;

/** @hideinitializer
 * Signed integer type corresponding to <code>sph_u32</code>; it has
 * width 32 bits or more.
 */
typedef __arch_dependant__ sph_s32;

/** @hideinitializer
 * Unsigned integer type whose length is at least 64 bits; on most
 * architectures which feature such a type, it will have a width of
 * exactly 64 bits. C99-compliant platform will have this type; it
 * is also defined when the GNU compiler (gcc) is used, and on
 * platforms where <code>unsigned long</code> is large enough. If this
 * type is not available, then some hash functions which depends on
 * a 64-bit type will not be available (most notably SHA-384, SHA-512,
 * Tiger and WHIRLPOOL).
 */
typedef __arch_dependant__ sph_u64;

/** @hideinitializer
 * Signed integer type corresponding to <code>sph_u64</code>; it has
 * width 64 bits or more.
 */
typedef __arch_dependant__ sph_s64;

/**
 * This macro expands the token <code>x</code> into a suitable
 * constant expression of type <code>sph_u32</code>. Depending on
 * how this type is defined, a suffix such as <code>UL</code> may
 * be appended to the argument.
 *
 * @param x   the token to expand into a suitable constant expression
 */
#define SPH_C32(x)

/**
 * Truncate a 32-bit value to exactly 32 bits. On most systems, this is
 * a no-op, recognized as such by the compiler.
 *
 * @param x   the value to truncate (of type <code>sph_u32</code>)
 */
#define SPH_T32(x)

/**
 * Rotate a 32-bit value by a number of bits to the left. The rotate
 * count must reside between 1 and 31. This macro assumes that its
 * first argument fits in 32 bits (no extra bit allowed on machines where
 * <code>sph_u32</code> is wider); both arguments may be evaluated
 * several times.
 *
 * @param x   the value to rotate (of type <code>sph_u32</code>)
 * @param n   the rotation count (between 1 and 31, inclusive)
 */
#define SPH_ROTL32(x, n)

/**
 * Rotate a 32-bit value by a number of bits to the left. The rotate
 * count must reside between 1 and 31. This macro assumes that its
 * first argument fits in 32 bits (no extra bit allowed on machines where
 * <code>sph_u32</code> is wider); both arguments may be evaluated
 * several times.
 *
 * @param x   the value to rotate (of type <code>sph_u32</code>)
 * @param n   the rotation count (between 1 and 31, inclusive)
 */
#define SPH_ROTR32(x, n)

/**
 * This macro is defined on systems for which a 64-bit type has been
 * detected, and is used for <code>sph_u64</code>.
 */
#define SPH_64

/**
 * This macro is defined on systems for the "native" integer size is
 * 64 bits (64-bit values fit in one register).
 */
#define SPH_64_TRUE

/**
 * This macro expands the token <code>x</code> into a suitable
 * constant expression of type <code>sph_u64</code>. Depending on
 * how this type is defined, a suffix such as <code>ULL</code> may
 * be appended to the argument. This macro is defined only if a
 * 64-bit type was detected and used for <code>sph_u64</code>.
 *
 * @param x   the token to expand into a suitable constant expression
 */
#define SPH_C64(x)

/**
 * Truncate a 64-bit value to exactly 64 bits. On most systems, this is
 * a no-op, recognized as such by the compiler. This macro is defined only
 * if a 64-bit type was detected and used for <code>sph_u64</code>.
 *
 * @param x   the value to truncate (of type <code>sph_u64</code>)
 */
#define SPH_T64(x)

/**
 * Rotate a 64-bit value by a number of bits to the left. The rotate
 * count must reside between 1 and 63. This macro assumes that its
 * first argument fits in 64 bits (no extra bit allowed on machines where
 * <code>sph_u64</code> is wider); both arguments may be evaluated
 * several times. This macro is defined only if a 64-bit type was detected
 * and used for <code>sph_u64</code>.
 *
 * @param x   the value to rotate (of type <code>sph_u64</code>)
 * @param n   the rotation count (between 1 and 63, inclusive)
 */
#define SPH_ROTL64(x, n)

/**
 * Rotate a 64-bit value by a number of bits to the left. The rotate
 * count must reside between 1 and 63. This macro assumes that its
 * first argument fits in 64 bits (no extra bit allowed on machines where
 * <code>sph_u64</code> is wider); both arguments may be evaluated
 * several times. This macro is defined only if a 64-bit type was detected
 * and used for <code>sph_u64</code>.
 *
 * @param x   the value to rotate (of type <code>sph_u64</code>)
 * @param n   the rotation count (between 1 and 63, inclusive)
 */
#define SPH_ROTR64(x, n)

/**
 * This macro evaluates to <code>inline</code> or an equivalent construction,
 * if available on the compilation platform, or to nothing otherwise. This
 * is used to declare inline functions, for which the compiler should
 * endeavour to include the code directly in the caller. Inline functions
 * are typically defined in header files as replacement for macros.
 */
#define SPH_INLINE

/**
 * This macro is defined if the platform has been detected as using
 * little-endian convention. This implies that the <code>sph_u32</code>
 * type (and the <code>sph_u64</code> type also, if it is defined) has
 * an exact width (i.e. exactly 32-bit, respectively 64-bit).
 */
#define SPH_LITTLE_ENDIAN

/**
 * This macro is defined if the platform has been detected as using
 * big-endian convention. This implies that the <code>sph_u32</code>
 * type (and the <code>sph_u64</code> type also, if it is defined) has
 * an exact width (i.e. exactly 32-bit, respectively 64-bit).
 */
#define SPH_BIG_ENDIAN

/**
 * This macro is defined if 32-bit words (and 64-bit words, if defined)
 * can be read from and written to memory efficiently in little-endian
 * convention. This is the case for little-endian platforms, and also
 * for the big-endian platforms which have special little-endian access
 * opcodes (e.g. Ultrasparc).
 */
#define SPH_LITTLE_FAST

/**
 * This macro is defined if 32-bit words (and 64-bit words, if defined)
 * can be read from and written to memory efficiently in big-endian
 * convention. This is the case for little-endian platforms, and also
 * for the little-endian platforms which have special big-endian access
 * opcodes.
 */
#define SPH_BIG_FAST

/**
 * On some platforms, this macro is defined to an unsigned integer type
 * into which pointer values may be cast. The resulting value can then
 * be tested for being a multiple of 2, 4 or 8, indicating an aligned
 * pointer for, respectively, 16-bit, 32-bit or 64-bit memory accesses.
 */
#define SPH_UPTR

/**
 * When defined, this macro indicates that unaligned memory accesses
 * are possible with only a minor penalty, and thus should be prefered
 * over strategies which first copy data to an aligned buffer.
 */
#define SPH_UNALIGNED

/**
 * Byte-swap a 32-bit word (i.e. <code>0x12345678</code> becomes
 * <code>0x78563412</code>). This is an inline function which resorts
 * to inline assembly on some platforms, for better performance.
 *
 * @param x   the 32-bit value to byte-swap
 * @return  the byte-swapped value
 */
static inline sph_u32 sph_bswap32(sph_u32 x);

/**
 * Byte-swap a 64-bit word. This is an inline function which resorts
 * to inline assembly on some platforms, for better performance. This
 * function is defined only if a suitable 64-bit type was found for
 * <code>sph_u64</code>
 *
 * @param x   the 64-bit value to byte-swap
 * @return  the byte-swapped value
 */
static inline sph_u64 sph_bswap64(sph_u64 x);

/**
 * Decode a 16-bit unsigned value from memory, in little-endian convention
 * (least significant byte comes first).
 *
 * @param src   the source address
 * @return  the decoded value
 */
static inline unsigned sph_dec16le(const void *src);

/**
 * Encode a 16-bit unsigned value into memory, in little-endian convention
 * (least significant byte comes first).
 *
 * @param dst   the destination buffer
 * @param val   the value to encode
 */
static inline void sph_enc16le(void *dst, unsigned val);

/**
 * Decode a 16-bit unsigned value from memory, in big-endian convention
 * (most significant byte comes first).
 *
 * @param src   the source address
 * @return  the decoded value
 */
static inline unsigned sph_dec16be(const void *src);

/**
 * Encode a 16-bit unsigned value into memory, in big-endian convention
 * (most significant byte comes first).
 *
 * @param dst   the destination buffer
 * @param val   the value to encode
 */
static inline void sph_enc16be(void *dst, unsigned val);

/**
 * Decode a 32-bit unsigned value from memory, in little-endian convention
 * (least significant byte comes first).
 *
 * @param src   the source address
 * @return  the decoded value
 */
static inline sph_u32 sph_dec32le(const void *src);

/**
 * Decode a 32-bit unsigned value from memory, in little-endian convention
 * (least significant byte comes first). This function assumes that the
 * source address is suitably aligned for a direct access, if the platform
 * supports such things; it can thus be marginally faster than the generic
 * <code>sph_dec32le()</code> function.
 *
 * @param src   the source address
 * @return  the decoded value
 */
static inline sph_u32 sph_dec32le_aligned(const void *src);

/**
 * Encode a 32-bit unsigned value into memory, in little-endian convention
 * (least significant byte comes first).
 *
 * @param dst   the destination buffer
 * @param val   the value to encode
 */
static inline void sph_enc32le(void *dst, sph_u32 val);

/**
 * Encode a 32-bit unsigned value into memory, in little-endian convention
 * (least significant byte comes first). This function assumes that the
 * destination address is suitably aligned for a direct access, if the
 * platform supports such things; it can thus be marginally faster than
 * the generic <code>sph_enc32le()</code> function.
 *
 * @param dst   the destination buffer
 * @param val   the value to encode
 */
static inline void sph_enc32le_aligned(void *dst, sph_u32 val);

/**
 * Decode a 32-bit unsigned value from memory, in big-endian convention
 * (most significant byte comes first).
 *
 * @param src   the source address
 * @return  the decoded value
 */
static inline sph_u32 sph_dec32be(const void *src);

/**
 * Decode a 32-bit unsigned value from memory, in big-endian convention
 * (most significant byte comes first). This function assumes that the
 * source address is suitably aligned for a direct access, if the platform
 * supports such things; it can thus be marginally faster than the generic
 * <code>sph_dec32be()</code> function.
 *
 * @param src   the source address
 * @return  the decoded value
 */
static inline sph_u32 sph_dec32be_aligned(const void *src);

/**
 * Encode a 32-bit unsigned value into memory, in big-endian convention
 * (most significant byte comes first).
 *
 * @param dst   the destination buffer
 * @param val   the value to encode
 */
static inline void sph_enc32be(void *dst, sph_u32 val);

/**
 * Encode a 32-bit unsigned value into memory, in big-endian convention
 * (most significant byte comes first). This function assumes that the
 * destination address is suitably aligned for a direct access, if the
 * platform supports such things; it can thus be marginally faster than
 * the generic <code>sph_enc32be()</code> function.
 *
 * @param dst   the destination buffer
 * @param val   the value to encode
 */
static inline void sph_enc32be_aligned(void *dst, sph_u32 val);

/**
 * Decode a 64-bit unsigned value from memory, in little-endian convention
 * (least significant byte comes first). This function is defined only
 * if a suitable 64-bit type was detected and used for <code>sph_u64</code>.
 *
 * @param src   the source address
 * @return  the decoded value
 */
static inline sph_u64 sph_dec64le(const void *src);

/**
 * Decode a 64-bit unsigned value from memory, in little-endian convention
 * (least significant byte comes first). This function assumes that the
 * source address is suitably aligned for a direct access, if the platform
 * supports such things; it can thus be marginally faster than the generic
 * <code>sph_dec64le()</code> function. This function is defined only
 * if a suitable 64-bit type was detected and used for <code>sph_u64</code>.
 *
 * @param src   the source address
 * @return  the decoded value
 */
static inline sph_u64 sph_dec64le_aligned(const void *src);

/**
 * Encode a 64-bit unsigned value into memory, in little-endian convention
 * (least significant byte comes first). This function is defined only
 * if a suitable 64-bit type was detected and used for <code>sph_u64</code>.
 *
 * @param dst   the destination buffer
 * @param val   the value to encode
 */
static inline void sph_enc64le(void *dst, sph_u64 val);

/**
 * Encode a 64-bit unsigned value into memory, in little-endian convention
 * (least significant byte comes first). This function assumes that the
 * destination address is suitably aligned for a direct access, if the
 * platform supports such things; it can thus be marginally faster than
 * the generic <code>sph_enc64le()</code> function. This function is defined
 * only if a suitable 64-bit type was detected and used for
 * <code>sph_u64</code>.
 *
 * @param dst   the destination buffer
 * @param val   the value to encode
 */
static inline void sph_enc64le_aligned(void *dst, sph_u64 val);

/**
 * Decode a 64-bit unsigned value from memory, in big-endian convention
 * (most significant byte comes first). This function is defined only
 * if a suitable 64-bit type was detected and used for <code>sph_u64</code>.
 *
 * @param src   the source address
 * @return  the decoded value
 */
static inline sph_u64 sph_dec64be(const void *src);

/**
 * Decode a 64-bit unsigned value from memory, in big-endian convention
 * (most significant byte comes first). This function assumes that the
 * source address is suitably aligned for a direct access, if the platform
 * supports such things; it can thus be marginally faster than the generic
 * <code>sph_dec64be()</code> function. This function is defined only
 * if a suitable 64-bit type was detected and used for <code>sph_u64</code>.
 *
 * @param src   the source address
 * @return  the decoded value
 */
static inline sph_u64 sph_dec64be_aligned(const void *src);

/**
 * Encode a 64-bit unsigned value into memory, in big-endian convention
 * (most significant byte comes first). This function is defined only
 * if a suitable 64-bit type was detected and used for <code>sph_u64</code>.
 *
 * @param dst   the destination buffer
 * @param val   the value to encode
 */
static inline void sph_enc64be(void *dst, sph_u64 val);

/**
 * Encode a 64-bit unsigned value into memory, in big-endian convention
 * (most significant byte comes first). This function assumes that the
 * destination address is suitably aligned for a direct access, if the
 * platform supports such things; it can thus be marginally faster than
 * the generic <code>sph_enc64be()</code> function. This function is defined
 * only if a suitable 64-bit type was detected and used for
 * <code>sph_u64</code>.
 *
 * @param dst   the destination buffer
 * @param val   the value to encode
 */
static inline void sph_enc64be_aligned(void *dst, sph_u64 val);

#endif

/* ============== END documentation block for Doxygen ============= */

#ifndef DOXYGEN_IGNORE

/*
 * We want to define the types "sph_u32" and "sph_u64" which hold
 * unsigned values of at least, respectively, 32 and 64 bits. These
 * tests should select appropriate types for most platforms. The
 * macro "SPH_64" is defined if the 64-bit is supported.
 */

#undef SPH_64
#undef SPH_64_TRUE

#if defined __STDC__ && __STDC_VERSION__ >= 199901L

/*
 * On C99 implementations, we can use <stdint.h> to get an exact 64-bit
 * type, if any, or otherwise use a wider type (which must exist, for
 * C99 conformance).
 */

#include <stdint.h>

#ifdef UINT32_MAX
typedef uint32_t sph_u32;
typedef int32_t sph_s32;
#else
typedef uint_fast32_t sph_u32;
typedef int_fast32_t sph_s32;
#endif
#if !SPH_NO_64
#ifdef UINT64_MAX
typedef uint64_t sph_u64;
typedef int64_t sph_s64;
#else
typedef uint_fast64_t sph_u64;
typedef int_fast64_t sph_s64;
#endif
#endif

#define SPH_C32(x)    ((sph_u32)(x))
#if !SPH_NO_64
#define SPH_C64(x)    ((sph_u64)(x))
#define SPH_64  1
#endif

#else

/*
 * On non-C99 systems, we use "unsigned int" if it is wide enough,
 * "unsigned long" otherwise. This supports all "reasonable" architectures.
 * We have to be cautious: pre-C99 preprocessors handle constants
 * differently in '#if' expressions. Hence the shifts to test UINT_MAX.
 */

#if ((UINT_MAX >> 11) >> 11) >= 0x3FF

typedef unsigned int sph_u32;
typedef int sph_s32;

#define SPH_C32(x)    ((sph_u32)(x ## U))

#else

typedef unsigned long sph_u32;
typedef long sph_s32;

#define SPH_C32(x)    ((sph_u32)(x ## UL))

#endif

#if !SPH_NO_64

/*
 * We want a 64-bit type. We use "unsigned long" if it is wide enough (as
 * is common on 64-bit architectures such as AMD64, Alpha or Sparcv9),
 * "unsigned long long" otherwise, if available. We use ULLONG_MAX to
 * test whether "unsigned long long" is available; we also know that
 * gcc features this type, even if the libc header do not know it.
 */

#if ((ULONG_MAX >> 31) >> 31) >= 3

typedef unsigned long sph_u64;
typedef long sph_s64;

#define SPH_C64(x)    ((sph_u64)(x ## UL))

#define SPH_64  1

#elif ((ULLONG_MAX >> 31) >> 31) >= 3 || defined __GNUC__

typedef unsigned long long sph_u64;
typedef long long sph_s64;

#define SPH_C64(x)    ((sph_u64)(x ## ULL))

#define SPH_64  1

#else

/*
 * No 64-bit type...
 */

#endif

#endif

#endif

/*
 * If the "unsigned long" type has length 64 bits or more, then this is
 * a "true" 64-bit architectures. This is also true with Visual C on
 * amd64, even though the "long" type is limited to 32 bits.
 */
#if SPH_64 && (((ULONG_MAX >> 31) >> 31) >= 3 || defined _M_X64)
#define SPH_64_TRUE   1
#endif

/*
 * Implementation note: some processors have specific opcodes to perform
 * a rotation. Recent versions of gcc recognize the expression above and
 * use the relevant opcodes, when appropriate.
 */

#define SPH_T32(x)    ((x) & SPH_C32(0xFFFFFFFF))
#define SPH_ROTL32(x, n)   SPH_T32(((x) << (n)) | ((x) >> (32 - (n))))
#define SPH_ROTR32(x, n)   SPH_ROTL32(x, (32 - (n)))

#if SPH_64

#define SPH_T64(x)    ((x) & SPH_C64(0xFFFFFFFFFFFFFFFF))
#define SPH_ROTL64(x, n)   SPH_T64(((x) << (n)) | ((x) >> (64 - (n))))
#define SPH_ROTR64(x, n)   SPH_ROTL64(x, (64 - (n)))

#endif

#ifndef DOXYGEN_IGNORE
/*
 * Define SPH_INLINE to be an "inline" qualifier, if available. We define
 * some small macro-like functions which benefit greatly from being inlined.
 */
#if (defined __STDC__ && __STDC_VERSION__ >= 199901L) || defined __GNUC__
#define SPH_INLINE inline
#elif defined _MSC_VER
#define SPH_INLINE __inline
#else
#define SPH_INLINE
#endif
#endif

/*
 * We define some macros which qualify the architecture. These macros
 * may be explicit set externally (e.g. as compiler parameters). The
 * code below sets those macros if they are not already defined.
 *
 * Most macros are boolean, thus evaluate to either zero or non-zero.
 * The SPH_UPTR macro is special, in that it evaluates to a C type,
 * or is not defined.
 *
 * SPH_UPTR             if defined: unsigned type to cast pointers into
 *
 * SPH_UNALIGNED        non-zero if unaligned accesses are efficient
 * SPH_LITTLE_ENDIAN    non-zero if architecture is known to be little-endian
 * SPH_BIG_ENDIAN       non-zero if architecture is known to be big-endian
 * SPH_LITTLE_FAST      non-zero if little-endian decoding is fast
 * SPH_BIG_FAST         non-zero if big-endian decoding is fast
 *
 * If SPH_UPTR is defined, then encoding and decoding of 32-bit and 64-bit
 * values will try to be "smart". Either SPH_LITTLE_ENDIAN or SPH_BIG_ENDIAN
 * _must_ be non-zero in those situations. The 32-bit and 64-bit types
 * _must_ also have an exact width.
 *
 * SPH_SPARCV9_GCC_32   UltraSPARC-compatible with gcc, 32-bit mode
 * SPH_SPARCV9_GCC_64   UltraSPARC-compatible with gcc, 64-bit mode
 * SPH_SPARCV9_GCC      UltraSPARC-compatible with gcc
 * SPH_I386_GCC         x86-compatible (32-bit) with gcc
 * SPH_I386_MSVC        x86-compatible (32-bit) with Microsoft Visual C
 * SPH_AMD64_GCC        x86-compatible (64-bit) with gcc
 * SPH_AMD64_MSVC       x86-compatible (64-bit) with Microsoft Visual C
 * SPH_PPC32_GCC        PowerPC, 32-bit, with gcc
 * SPH_PPC64_GCC        PowerPC, 64-bit, with gcc
 *
 * TODO: enhance automatic detection, for more architectures and compilers.
 * Endianness is the most important. SPH_UNALIGNED and SPH_UPTR help with
 * some very fast functions (e.g. MD4) when using unaligned input data.
 * The CPU-specific-with-GCC macros are useful only for inline assembly,
 * normally restrained to this header file.
 */

/*
 * 32-bit x86, aka "i386 compatible".
 */
#if defined __i386__ || defined _M_IX86

#define SPH_DETECT_UNALIGNED         1
#define SPH_DETECT_LITTLE_ENDIAN     1
#define SPH_DETECT_UPTR              sph_u32
#ifdef __GNUC__
#define SPH_DETECT_I386_GCC          1
#endif
#ifdef _MSC_VER
#define SPH_DETECT_I386_MSVC         1
#endif

/*
 * 64-bit x86, hereafter known as "amd64".
 */
#elif defined __x86_64 || defined _M_X64

#define SPH_DETECT_UNALIGNED         1
#define SPH_DETECT_LITTLE_ENDIAN     1
#define SPH_DETECT_UPTR              sph_u64
#ifdef __GNUC__
#define SPH_DETECT_AMD64_GCC         1
#endif
#ifdef _MSC_VER
#define SPH_DETECT_AMD64_MSVC        1
#endif

/*
 * 64-bit Sparc architecture (implies v9).
 */
#elif ((defined __sparc__ || defined __sparc) && defined __arch64__) \
	|| defined __sparcv9

#define SPH_DETECT_BIG_ENDIAN        1
#define SPH_DETECT_UPTR              sph_u64
#ifdef __GNUC__
#define SPH_DETECT_SPARCV9_GCC_64    1
#define SPH_DETECT_LITTLE_FAST       1
#endif

/*
 * 32-bit Sparc.
 */
#elif (defined __sparc__ || defined __sparc) \
	&& !(defined __sparcv9 || defined __arch64__)

#define SPH_DETECT_BIG_ENDIAN        1
#define SPH_DETECT_UPTR              sph_u32
#if defined __GNUC__ && defined __sparc_v9__
#define SPH_DETECT_SPARCV9_GCC_32    1
#define SPH_DETECT_LITTLE_FAST       1
#endif

/*
 * ARM, little-endian.
 */
#elif defined __arm__ && __ARMEL__

#define SPH_DETECT_LITTLE_ENDIAN     1

/*
 * MIPS, little-endian.
 */
#elif MIPSEL || _MIPSEL || __MIPSEL || __MIPSEL__

#define SPH_DETECT_LITTLE_ENDIAN     1

/*
 * MIPS, big-endian.
 */
#elif MIPSEB || _MIPSEB || __MIPSEB || __MIPSEB__

#define SPH_DETECT_BIG_ENDIAN        1

/*
 * PowerPC.
 */
#elif defined __powerpc__ || defined __POWERPC__ || defined __ppc__ \
	|| defined _ARCH_PPC

/*
 * Note: we do not declare cross-endian access to be "fast": even if
 * using inline assembly, implementation should still assume that
 * keeping the decoded word in a temporary is faster than decoding
 * it again.
 */
#if defined __GNUC__
#if SPH_64_TRUE
#define SPH_DETECT_PPC64_GCC         1
#else
#define SPH_DETECT_PPC32_GCC         1
#endif
#endif

#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
#define SPH_DETECT_BIG_ENDIAN        1
#elif defined __LITTLE_ENDIAN__ || defined _LITTLE_ENDIAN
#define SPH_DETECT_LITTLE_ENDIAN     1
#endif

/*
 * Itanium, 64-bit.
 */
#elif defined __ia64 || defined __ia64__ \
	|| defined __itanium__ || defined _M_IA64

#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
#define SPH_DETECT_BIG_ENDIAN        1
#else
#define SPH_DETECT_LITTLE_ENDIAN     1
#endif
#if defined __LP64__ || defined _LP64
#define SPH_DETECT_UPTR              sph_u64
#else
#define SPH_DETECT_UPTR              sph_u32
#endif

#endif

#if defined SPH_DETECT_SPARCV9_GCC_32 || defined SPH_DETECT_SPARCV9_GCC_64
#define SPH_DETECT_SPARCV9_GCC       1
#endif

#if defined SPH_DETECT_UNALIGNED && !defined SPH_UNALIGNED
#define SPH_UNALIGNED         SPH_DETECT_UNALIGNED
#endif
#if defined SPH_DETECT_UPTR && !defined SPH_UPTR
#define SPH_UPTR              SPH_DETECT_UPTR
#endif
#if defined SPH_DETECT_LITTLE_ENDIAN && !defined SPH_LITTLE_ENDIAN
#define SPH_LITTLE_ENDIAN     SPH_DETECT_LITTLE_ENDIAN
#endif
#if defined SPH_DETECT_BIG_ENDIAN && !defined SPH_BIG_ENDIAN
#define SPH_BIG_ENDIAN        SPH_DETECT_BIG_ENDIAN
#endif
#if defined SPH_DETECT_LITTLE_FAST && !defined SPH_LITTLE_FAST
#define SPH_LITTLE_FAST       SPH_DETECT_LITTLE_FAST
#endif
#if defined SPH_DETECT_BIG_FAST && !defined SPH_BIG_FAST
#define SPH_BIG_FAST    SPH_DETECT_BIG_FAST
#endif
#if defined SPH_DETECT_SPARCV9_GCC_32 && !defined SPH_SPARCV9_GCC_32
#define SPH_SPARCV9_GCC_32    SPH_DETECT_SPARCV9_GCC_32
#endif
#if defined SPH_DETECT_SPARCV9_GCC_64 && !defined SPH_SPARCV9_GCC_64
#define SPH_SPARCV9_GCC_64    SPH_DETECT_SPARCV9_GCC_64
#endif
#if defined SPH_DETECT_SPARCV9_GCC && !defined SPH_SPARCV9_GCC
#define SPH_SPARCV9_GCC       SPH_DETECT_SPARCV9_GCC
#endif
#if defined SPH_DETECT_I386_GCC && !defined SPH_I386_GCC
#define SPH_I386_GCC          SPH_DETECT_I386_GCC
#endif
#if defined SPH_DETECT_I386_MSVC && !defined SPH_I386_MSVC
#define SPH_I386_MSVC         SPH_DETECT_I386_MSVC
#endif
#if defined SPH_DETECT_AMD64_GCC && !defined SPH_AMD64_GCC
#define SPH_AMD64_GCC         SPH_DETECT_AMD64_GCC
#endif
#if defined SPH_DETECT_AMD64_MSVC && !defined SPH_AMD64_MSVC
#define SPH_AMD64_MSVC        SPH_DETECT_AMD64_MSVC
#endif
#if defined SPH_DETECT_PPC32_GCC && !defined SPH_PPC32_GCC
#define SPH_PPC32_GCC         SPH_DETECT_PPC32_GCC
#endif
#if defined SPH_DETECT_PPC64_GCC && !defined SPH_PPC64_GCC
#define SPH_PPC64_GCC         SPH_DETECT_PPC64_GCC
#endif

#if SPH_LITTLE_ENDIAN && !defined SPH_LITTLE_FAST
#define SPH_LITTLE_FAST              1
#endif
#if SPH_BIG_ENDIAN && !defined SPH_BIG_FAST
#define SPH_BIG_FAST                 1
#endif

#if defined SPH_UPTR && !(SPH_LITTLE_ENDIAN || SPH_BIG_ENDIAN)
#error SPH_UPTR defined, but endianness is not known.
#endif

#if SPH_I386_GCC && !SPH_NO_ASM

/*
 * On x86 32-bit, with gcc, we use the bswapl opcode to byte-swap 32-bit
 * values.
 */

static SPH_INLINE sph_u32
sph_bswap32(sph_u32 x)
{
	__asm__ __volatile__ ("bswapl %0" : "=r" (x) : "0" (x));
	return x;
}

#if SPH_64

static SPH_INLINE sph_u64
sph_bswap64(sph_u64 x)
{
	return ((sph_u64)sph_bswap32((sph_u32)x) << 32)
		| (sph_u64)sph_bswap32((sph_u32)(x >> 32));
}

#endif

#elif SPH_AMD64_GCC && !SPH_NO_ASM

/*
 * On x86 64-bit, with gcc, we use the bswapl opcode to byte-swap 32-bit
 * and 64-bit values.
 */

static SPH_INLINE sph_u32
sph_bswap32(sph_u32 x)
{
	__asm__ __volatile__ ("bswapl %0" : "=r" (x) : "0" (x));
	return x;
}

#if SPH_64

static SPH_INLINE sph_u64
sph_bswap64(sph_u64 x)
{
	__asm__ __volatile__ ("bswapq %0" : "=r" (x) : "0" (x));
	return x;
}

#endif

/*
 * Disabled code. Apparently, Microsoft Visual C 2005 is smart enough
 * to generate proper opcodes for endianness swapping with the pure C
 * implementation below.
 *

#elif SPH_I386_MSVC && !SPH_NO_ASM

static __inline sph_u32 __declspec(naked) __fastcall
sph_bswap32(sph_u32 x)
{
	__asm {
		bswap  ecx
		mov    eax,ecx
		ret
	}
}

#if SPH_64

static SPH_INLINE sph_u64
sph_bswap64(sph_u64 x)
{
	return ((sph_u64)sph_bswap32((sph_u32)x) << 32)
		| (sph_u64)sph_bswap32((sph_u32)(x >> 32));
}

#endif

 *
 * [end of disabled code]
 */

#else

static SPH_INLINE sph_u32
sph_bswap32(sph_u32 x)
{
	x = SPH_T32((x << 16) | (x >> 16));
	x = ((x & SPH_C32(0xFF00FF00)) >> 8)
		| ((x & SPH_C32(0x00FF00FF)) << 8);
	return x;
}

#if SPH_64

/**
 * Byte-swap a 64-bit value.
 *
 * @param x   the input value
 * @return  the byte-swapped value
 */
static SPH_INLINE sph_u64
sph_bswap64(sph_u64 x)
{
	x = SPH_T64((x << 32) | (x >> 32));
	x = ((x & SPH_C64(0xFFFF0000FFFF0000)) >> 16)
		| ((x & SPH_C64(0x0000FFFF0000FFFF)) << 16);
	x = ((x & SPH_C64(0xFF00FF00FF00FF00)) >> 8)
		| ((x & SPH_C64(0x00FF00FF00FF00FF)) << 8);
	return x;
}

#endif

#endif

#if SPH_SPARCV9_GCC && !SPH_NO_ASM

/*
 * On UltraSPARC systems, native ordering is big-endian, but it is
 * possible to perform little-endian read accesses by specifying the
 * address space 0x88 (ASI_PRIMARY_LITTLE). Basically, either we use
 * the opcode "lda [%reg]0x88,%dst", where %reg is the register which
 * contains the source address and %dst is the destination register,
 * or we use "lda [%reg+imm]%asi,%dst", which uses the %asi register
 * to get the address space name. The latter format is better since it
 * combines an addition and the actual access in a single opcode; but
 * it requires the setting (and subsequent resetting) of %asi, which is
 * slow. Some operations (i.e. MD5 compression function) combine many
 * successive little-endian read accesses, which may share the same
 * %asi setting. The macros below contain the appropriate inline
 * assembly.
 */

#define SPH_SPARCV9_SET_ASI   \
	sph_u32 sph_sparcv9_asi; \
	__asm__ __volatile__ ( \
		"rd %%asi,%0\n\twr %%g0,0x88,%%asi" : "=r" (sph_sparcv9_asi));

#define SPH_SPARCV9_RESET_ASI  \
	__asm__ __volatile__ ("wr %%g0,%0,%%asi" : : "r" (sph_sparcv9_asi));

#define SPH_SPARCV9_DEC32LE(base, idx)   ({ \
		sph_u32 sph_sparcv9_tmp; \
		__asm__ __volatile__ ("lda [%1+" #idx "*4]%%asi,%0" \
			: "=r" (sph_sparcv9_tmp) : "r" (base)); \
		sph_sparcv9_tmp; \
	})

#endif

static SPH_INLINE void
sph_enc16be(void *dst, unsigned val)
{
	((unsigned char *)dst)[0] = (val >> 8);
	((unsigned char *)dst)[1] = val;
}

static SPH_INLINE unsigned
sph_dec16be(const void *src)
{
	return ((unsigned)(((const unsigned char *)src)[0]) << 8)
		| (unsigned)(((const unsigned char *)src)[1]);
}

static SPH_INLINE void
sph_enc16le(void *dst, unsigned val)
{
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = val >> 8;
}

static SPH_INLINE unsigned
sph_dec16le(const void *src)
{
	return (unsigned)(((const unsigned char *)src)[0])
		| ((unsigned)(((const unsigned char *)src)[1]) << 8);
}

/**
 * Encode a 32-bit value into the provided buffer (big endian convention).
 *
 * @param dst   the destination buffer
 * @param val   the 32-bit value to encode
 */
static SPH_INLINE void
sph_enc32be(void *dst, sph_u32 val)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_LITTLE_ENDIAN
	val = sph_bswap32(val);
#endif
	*(sph_u32 *)dst = val;
#else
	if (((SPH_UPTR)dst & 3) == 0) {
#if SPH_LITTLE_ENDIAN
		val = sph_bswap32(val);
#endif
		*(sph_u32 *)dst = val;
	} else {
		((unsigned char *)dst)[0] = (val >> 24);
		((unsigned char *)dst)[1] = (val >> 16);
		((unsigned char *)dst)[2] = (val >> 8);
		((unsigned char *)dst)[3] = val;
	}
#endif
#else
	((unsigned char *)dst)[0] = (val >> 24);
	((unsigned char *)dst)[1] = (val >> 16);
	((unsigned char *)dst)[2] = (val >> 8);
	((unsigned char *)dst)[3] = val;
#endif
}

/**
 * Encode a 32-bit value into the provided buffer (big endian convention).
 * The destination buffer must be properly aligned.
 *
 * @param dst   the destination buffer (32-bit aligned)
 * @param val   the value to encode
 */
static SPH_INLINE void
sph_enc32be_aligned(void *dst, sph_u32 val)
{
#if SPH_LITTLE_ENDIAN
	*(sph_u32 *)dst = sph_bswap32(val);
#elif SPH_BIG_ENDIAN
	*(sph_u32 *)dst = val;
#else
	((unsigned char *)dst)[0] = (val >> 24);
	((unsigned char *)dst)[1] = (val >> 16);
	((unsigned char *)dst)[2] = (val >> 8);
	((unsigned char *)dst)[3] = val;
#endif
}

/**
 * Decode a 32-bit value from the provided buffer (big endian convention).
 *
 * @param src   the source buffer
 * @return  the decoded value
 */
static SPH_INLINE sph_u32
sph_dec32be(const void *src)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_LITTLE_ENDIAN
	return sph_bswap32(*(const sph_u32 *)src);
#else
	return *(const sph_u32 *)src;
#endif
#else
	if (((SPH_UPTR)src & 3) == 0) {
#if SPH_LITTLE_ENDIAN
		return sph_bswap32(*(const sph_u32 *)src);
#else
		return *(const sph_u32 *)src;
#endif
	} else {
		return ((sph_u32)(((const unsigned char *)src)[0]) << 24)
			| ((sph_u32)(((const unsigned char *)src)[1]) << 16)
			| ((sph_u32)(((const unsigned char *)src)[2]) << 8)
			| (sph_u32)(((const unsigned char *)src)[3]);
	}
#endif
#else
	return ((sph_u32)(((const unsigned char *)src)[0]) << 24)
		| ((sph_u32)(((const unsigned char *)src)[1]) << 16)
		| ((sph_u32)(((const unsigned char *)src)[2]) << 8)
		| (sph_u32)(((const unsigned char *)src)[3]);
#endif
}

/**
 * Decode a 32-bit value from the provided buffer (big endian convention).
 * The source buffer must be properly aligned.
 *
 * @param src   the source buffer (32-bit aligned)
 * @return  the decoded value
 */
static SPH_INLINE sph_u32
sph_dec32be_aligned(const void *src)
{
#if SPH_LITTLE_ENDIAN
	return sph_bswap32(*(const sph_u32 *)src);
#elif SPH_BIG_ENDIAN
	return *(const sph_u32 *)src;
#else
	return ((sph_u32)(((const unsigned char *)src)[0]) << 24)
		| ((sph_u32)(((const unsigned char *)src)[1]) << 16)
		| ((sph_u32)(((const unsigned char *)src)[2]) << 8)
		| (sph_u32)(((const unsigned char *)src)[3]);
#endif
}

/**
 * Encode a 32-bit value into the provided buffer (little endian convention).
 *
 * @param dst   the destination buffer
 * @param val   the 32-bit value to encode
 */
static SPH_INLINE void
sph_enc32le(void *dst, sph_u32 val)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_BIG_ENDIAN
	val = sph_bswap32(val);
#endif
	*(sph_u32 *)dst = val;
#else
	if (((SPH_UPTR)dst & 3) == 0) {
#if SPH_BIG_ENDIAN
		val = sph_bswap32(val);
#endif
		*(sph_u32 *)dst = val;
	} else {
		((unsigned char *)dst)[0] = val;
		((unsigned char *)dst)[1] = (val >> 8);
		((unsigned char *)dst)[2] = (val >> 16);
		((unsigned char *)dst)[3] = (val >> 24);
	}
#endif
#else
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = (val >> 8);
	((unsigned char *)dst)[2] = (val >> 16);
	((unsigned char *)dst)[3] = (val >> 24);
#endif
}

/**
 * Encode a 32-bit value into the provided buffer (little endian convention).
 * The destination buffer must be properly aligned.
 *
 * @param dst   the destination buffer (32-bit aligned)
 * @param val   the value to encode
 */
static SPH_INLINE void
sph_enc32le_aligned(void *dst, sph_u32 val)
{
#if SPH_LITTLE_ENDIAN
	*(sph_u32 *)dst = val;
#elif SPH_BIG_ENDIAN
	*(sph_u32 *)dst = sph_bswap32(val);
#else
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = (val >> 8);
	((unsigned char *)dst)[2] = (val >> 16);
	((unsigned char *)dst)[3] = (val >> 24);
#endif
}

/**
 * Decode a 32-bit value from the provided buffer (little endian convention).
 *
 * @param src   the source buffer
 * @return  the decoded value
 */
static SPH_INLINE sph_u32
sph_dec32le(const void *src)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_BIG_ENDIAN
	return sph_bswap32(*(const sph_u32 *)src);
#else
	return *(const sph_u32 *)src;
#endif
#else
	if (((SPH_UPTR)src & 3) == 0) {
#if SPH_BIG_ENDIAN
#if SPH_SPARCV9_GCC && !SPH_NO_ASM
		sph_u32 tmp;

		/*
		 * "__volatile__" is needed here because without it,
		 * gcc-3.4.3 miscompiles the code and performs the
		 * access before the test on the address, thus triggering
		 * a bus error...
		 */
		__asm__ __volatile__ (
			"lda [%1]0x88,%0" : "=r" (tmp) : "r" (src));
		return tmp;
/*
 * On PowerPC, this turns out not to be worth the effort: the inline
 * assembly makes GCC optimizer uncomfortable, which tends to nullify
 * the decoding gains.
 *
 * For most hash functions, using this inline assembly trick changes
 * hashing speed by less than 5% and often _reduces_ it. The biggest
 * gains are for MD4 (+11%) and CubeHash (+30%). For all others, it is
 * less then 10%. The speed gain on CubeHash is probably due to the
 * chronic shortage of registers that CubeHash endures; for the other
 * functions, the generic code appears to be efficient enough already.
 *
#elif (SPH_PPC32_GCC || SPH_PPC64_GCC) && !SPH_NO_ASM
		sph_u32 tmp;

		__asm__ __volatile__ (
			"lwbrx %0,0,%1" : "=r" (tmp) : "r" (src));
		return tmp;
 */
#else
		return sph_bswap32(*(const sph_u32 *)src);
#endif
#else
		return *(const sph_u32 *)src;
#endif
	} else {
		return (sph_u32)(((const unsigned char *)src)[0])
			| ((sph_u32)(((const unsigned char *)src)[1]) << 8)
			| ((sph_u32)(((const unsigned char *)src)[2]) << 16)
			| ((sph_u32)(((const unsigned char *)src)[3]) << 24);
	}
#endif
#else
	return (sph_u32)(((const unsigned char *)src)[0])
		| ((sph_u32)(((const unsigned char *)src)[1]) << 8)
		| ((sph_u32)(((const unsigned char *)src)[2]) << 16)
		| ((sph_u32)(((const unsigned char *)src)[3]) << 24);
#endif
}

/**
 * Decode a 32-bit value from the provided buffer (little endian convention).
 * The source buffer must be properly aligned.
 *
 * @param src   the source buffer (32-bit aligned)
 * @return  the decoded value
 */
static SPH_INLINE sph_u32
sph_dec32le_aligned(const void *src)
{
#if SPH_LITTLE_ENDIAN
	return *(const sph_u32 *)src;
#elif SPH_BIG_ENDIAN
#if SPH_SPARCV9_GCC && !SPH_NO_ASM
	sph_u32 tmp;

	__asm__ __volatile__ ("lda [%1]0x88,%0" : "=r" (tmp) : "r" (src));
	return tmp;
/*
 * Not worth it generally.
 *
#elif (SPH_PPC32_GCC || SPH_PPC64_GCC) && !SPH_NO_ASM
	sph_u32 tmp;

	__asm__ __volatile__ ("lwbrx %0,0,%1" : "=r" (tmp) : "r" (src));
	return tmp;
 */
#else
	return sph_bswap32(*(const sph_u32 *)src);
#endif
#else
	return (sph_u32)(((const unsigned char *)src)[0])
		| ((sph_u32)(((const unsigned char *)src)[1]) << 8)
		| ((sph_u32)(((const unsigned char *)src)[2]) << 16)
		| ((sph_u32)(((const unsigned char *)src)[3]) << 24);
#endif
}

#if SPH_64

/**
 * Encode a 64-bit value into the provided buffer (big endian convention).
 *
 * @param dst   the destination buffer
 * @param val   the 64-bit value to encode
 */
static SPH_INLINE void
sph_enc64be(void *dst, sph_u64 val)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_LITTLE_ENDIAN
	val = sph_bswap64(val);
#endif
	*(sph_u64 *)dst = val;
#else
	if (((SPH_UPTR)dst & 7) == 0) {
#if SPH_LITTLE_ENDIAN
		val = sph_bswap64(val);
#endif
		*(sph_u64 *)dst = val;
	} else {
		((unsigned char *)dst)[0] = (val >> 56);
		((unsigned char *)dst)[1] = (val >> 48);
		((unsigned char *)dst)[2] = (val >> 40);
		((unsigned char *)dst)[3] = (val >> 32);
		((unsigned char *)dst)[4] = (val >> 24);
		((unsigned char *)dst)[5] = (val >> 16);
		((unsigned char *)dst)[6] = (val >> 8);
		((unsigned char *)dst)[7] = val;
	}
#endif
#else
	((unsigned char *)dst)[0] = (val >> 56);
	((unsigned char *)dst)[1] = (val >> 48);
	((unsigned char *)dst)[2] = (val >> 40);
	((unsigned char *)dst)[3] = (val >> 32);
	((unsigned char *)dst)[4] = (val >> 24);
	((unsigned char *)dst)[5] = (val >> 16);
	((unsigned char *)dst)[6] = (val >> 8);
	((unsigned char *)dst)[7] = val;
#endif
}

/**
 * Encode a 64-bit value into the provided buffer (big endian convention).
 * The destination buffer must be properly aligned.
 *
 * @param dst   the destination buffer (64-bit aligned)
 * @param val   the value to encode
 */
static SPH_INLINE void
sph_enc64be_aligned(void *dst, sph_u64 val)
{
#if SPH_LITTLE_ENDIAN
	*(sph_u64 *)dst = sph_bswap64(val);
#elif SPH_BIG_ENDIAN
	*(sph_u64 *)dst = val;
#else
	((unsigned char *)dst)[0] = (val >> 56);
	((unsigned char *)dst)[1] = (val >> 48);
	((unsigned char *)dst)[2] = (val >> 40);
	((unsigned char *)dst)[3] = (val >> 32);
	((unsigned char *)dst)[4] = (val >> 24);
	((unsigned char *)dst)[5] = (val >> 16);
	((unsigned char *)dst)[6] = (val >> 8);
	((unsigned char *)dst)[7] = val;
#endif
}

/**
 * Decode a 64-bit value from the provided buffer (big endian convention).
 *
 * @param src   the source buffer
 * @return  the decoded value
 */
static SPH_INLINE sph_u64
sph_dec64be(const void *src)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_LITTLE_ENDIAN
	return sph_bswap64(*(const sph_u64 *)src);
#else
	return *(const sph_u64 *)src;
#endif
#else
	if (((SPH_UPTR)src & 7) == 0) {
#if SPH_LITTLE_ENDIAN
		return sph_bswap64(*(const sph_u64 *)src);
#else
		return *(const sph_u64 *)src;
#endif
	} else {
		return ((sph_u64)(((const unsigned char *)src)[0]) << 56)
			| ((sph_u64)(((const unsigned char *)src)[1]) << 48)
			| ((sph_u64)(((const unsigned char *)src)[2]) << 40)
			| ((sph_u64)(((const unsigned char *)src)[3]) << 32)
			| ((sph_u64)(((const unsigned char *)src)[4]) << 24)
			| ((sph_u64)(((const unsigned char *)src)[5]) << 16)
			| ((sph_u64)(((const unsigned char *)src)[6]) << 8)
			| (sph_u64)(((const unsigned char *)src)[7]);
	}
#endif
#else
	return ((sph_u64)(((const unsigned char *)src)[0]) << 56)
		| ((sph_u64)(((const unsigned char *)src)[1]) << 48)
		| ((sph_u64)(((const unsigned char *)src)[2]) << 40)
		| ((sph_u64)(((const unsigned char *)src)[3]) << 32)
		| ((sph_u64)(((const unsigned char *)src)[4]) << 24)
		| ((sph_u64)(((const unsigned char *)src)[5]) << 16)
		| ((sph_u64)(((const unsigned char *)src)[6]) << 8)
		| (sph_u64)(((const unsigned char *)src)[7]);
#endif
}

/**
 * Decode a 64-bit value from the provided buffer (big endian convention).
 * The source buffer must be properly aligned.
 *
 * @param src   the source buffer (64-bit aligned)
 * @return  the decoded value
 */
static SPH_INLINE sph_u64
sph_dec64be_aligned(const void *src)
{
#if SPH_LITTLE_ENDIAN
	return sph_bswap64(*(const sph_u64 *)src);
#elif SPH_BIG_ENDIAN
	return *(const sph_u64 *)src;
#else
	return ((sph_u64)(((const unsigned char *)src)[0]) << 56)
		| ((sph_u64)(((const unsigned char *)src)[1]) << 48)
		| ((sph_u64)(((const unsigned char *)src)[2]) << 40)
		| ((sph_u64)(((const unsigned char *)src)[3]) << 32)
		| ((sph_u64)(((const unsigned char *)src)[4]) << 24)
		| ((sph_u64)(((const unsigned char *)src)[5]) << 16)
		| ((sph_u64)(((const unsigned char *)src)[6]) << 8)
		| (sph_u64)(((const unsigned char *)src)[7]);
#endif
}

/**
 * Encode a 64-bit value into the provided buffer (little endian convention).
 *
 * @param dst   the destination buffer
 * @param val   the 64-bit value to encode
 */
static SPH_INLINE void
sph_enc64le(void *dst, sph_u64 val)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_BIG_ENDIAN
	val = sph_bswap64(val);
#endif
	*(sph_u64 *)dst = val;
#else
	if (((SPH_UPTR)dst & 7) == 0) {
#if SPH_BIG_ENDIAN
		val = sph_bswap64(val);
#endif
		*(sph_u64 *)dst = val;
	} else {
		((unsigned char *)dst)[0] = val;
		((unsigned char *)dst)[1] = (val >> 8);
		((unsigned char *)dst)[2] = (val >> 16);
		((unsigned char *)dst)[3] = (val >> 24);
		((unsigned char *)dst)[4] = (val >> 32);
		((unsigned char *)dst)[5] = (val >> 40);
		((unsigned char *)dst)[6] = (val >> 48);
		((unsigned char *)dst)[7] = (val >> 56);
	}
#endif
#else
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = (val >> 8);
	((unsigned char *)dst)[2] = (val >> 16);
	((unsigned char *)dst)[3] = (val >> 24);
	((unsigned char *)dst)[4] = (val >> 32);
	((unsigned char *)dst)[5] = (val >> 40);
	((unsigned char *)dst)[6] = (val >> 48);
	((unsigned char *)dst)[7] = (val >> 56);
#endif
}

/**
 * Encode a 64-bit value into the provided buffer (little endian convention).
 * The destination buffer must be properly aligned.
 *
 * @param dst   the destination buffer (64-bit aligned)
 * @param val   the value to encode
 */
static SPH_INLINE void
sph_enc64le_aligned(void *dst, sph_u64 val)
{
#if SPH_LITTLE_ENDIAN
	*(sph_u64 *)dst = val;
#elif SPH_BIG_ENDIAN
	*(sph_u64 *)dst = sph_bswap64(val);
#else
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = (val >> 8);
	((unsigned char *)dst)[2] = (val >> 16);
	((unsigned char *)dst)[3] = (val >> 24);
	((unsigned char *)dst)[4] = (val >> 32);
	((unsigned char *)dst)[5] = (val >> 40);
	((unsigned char *)dst)[6] = (val >> 48);
	((unsigned char *)dst)[7] = (val >> 56);
#endif
}

/**
 * Decode a 64-bit value from the provided buffer (little endian convention).
 *
 * @param src   the source buffer
 * @return  the decoded value
 */
static SPH_INLINE sph_u64
sph_dec64le(const void *src)
{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_BIG_ENDIAN
	return sph_bswap64(*(const sph_u64 *)src);
#else
	return *(const sph_u64 *)src;
#endif
#else
	if (((SPH_UPTR)src & 7) == 0) {
#if SPH_BIG_ENDIAN
#if SPH_SPARCV9_GCC_64 && !SPH_NO_ASM
		sph_u64 tmp;

		__asm__ __volatile__ (
			"ldxa [%1]0x88,%0" : "=r" (tmp) : "r" (src));
		return tmp;
/*
 * Not worth it generally.
 *
#elif SPH_PPC32_GCC && !SPH_NO_ASM
		return (sph_u64)sph_dec32le_aligned(src)
			| ((sph_u64)sph_dec32le_aligned(
				(const char *)src + 4) << 32);
#elif SPH_PPC64_GCC && !SPH_NO_ASM
		sph_u64 tmp;

		__asm__ __volatile__ (
			"ldbrx %0,0,%1" : "=r" (tmp) : "r" (src));
		return tmp;
 */
#else
		return sph_bswap64(*(const sph_u64 *)src);
#endif
#else
		return *(const sph_u64 *)src;
#endif
	} else {
		return (sph_u64)(((const unsigned char *)src)[0])
			| ((sph_u64)(((const unsigned char *)src)[1]) << 8)
			| ((sph_u64)(((const unsigned char *)src)[2]) << 16)
			| ((sph_u64)(((const unsigned char *)src)[3]) << 24)
			| ((sph_u64)(((const unsigned char *)src)[4]) << 32)
			| ((sph_u64)(((const unsigned char *)src)[5]) << 40)
			| ((sph_u64)(((const unsigned char *)src)[6]) << 48)
			| ((sph_u64)(((const unsigned char *)src)[7]) << 56);
	}
#endif
#else
	return (sph_u64)(((const unsigned char *)src)[0])
		| ((sph_u64)(((const unsigned char *)src)[1]) << 8)
		| ((sph_u64)(((const unsigned char *)src)[2]) << 16)
		| ((sph_u64)(((const unsigned char *)src)[3]) << 24)
		| ((sph_u64)(((const unsigned char *)src)[4]) << 32)
		| ((sph_u64)(((const unsigned char *)src)[5]) << 40)
		| ((sph_u64)(((const unsigned char *)src)[6]) << 48)
		| ((sph_u64)(((const unsigned char *)src)[7]) << 56);
#endif
}

/**
 * Decode a 64-bit value from the provided buffer (little endian convention).
 * The source buffer must be properly aligned.
 *
 * @param src   the source buffer (64-bit aligned)
 * @return  the decoded value
 */
static SPH_INLINE sph_u64
sph_dec64le_aligned(const void *src)
{
#if SPH_LITTLE_ENDIAN
	return *(const sph_u64 *)src;
#elif SPH_BIG_ENDIAN
#if SPH_SPARCV9_GCC_64 && !SPH_NO_ASM
	sph_u64 tmp;

	__asm__ __volatile__ ("ldxa [%1]0x88,%0" : "=r" (tmp) : "r" (src));
	return tmp;
/*
 * Not worth it generally.
 *
#elif SPH_PPC32_GCC && !SPH_NO_ASM
	return (sph_u64)sph_dec32le_aligned(src)
		| ((sph_u64)sph_dec32le_aligned((const char *)src + 4) << 32);
#elif SPH_PPC64_GCC && !SPH_NO_ASM
	sph_u64 tmp;

	__asm__ __volatile__ ("ldbrx %0,0,%1" : "=r" (tmp) : "r" (src));
	return tmp;
 */
#else
	return sph_bswap64(*(const sph_u64 *)src);
#endif
#else
	return (sph_u64)(((const unsigned char *)src)[0])
		| ((sph_u64)(((const unsigned char *)src)[1]) << 8)
		| ((sph_u64)(((const unsigned char *)src)[2]) << 16)
		| ((sph_u64)(((const unsigned char *)src)[3]) << 24)
		| ((sph_u64)(((const unsigned char *)src)[4]) << 32)
		| ((sph_u64)(((const unsigned char *)src)[5]) << 40)
		| ((sph_u64)(((const unsigned char *)src)[6]) << 48)
		| ((sph_u64)(((const unsigned char *)src)[7]) << 56);
#endif
}

#endif

#endif /* Doxygen excluded block */

#endif
