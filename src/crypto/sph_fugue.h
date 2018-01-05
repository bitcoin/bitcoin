#ifndef SPH_FUGUE_H__
#define SPH_FUGUE_H__

#include <stddef.h>
#include "sph_types.h"

#ifdef __cplusplus
extern "C"{
#endif

#define SPH_SIZE_fugue224   224

#define SPH_SIZE_fugue256   256

#define SPH_SIZE_fugue384   384

#define SPH_SIZE_fugue512   512

typedef struct {
#ifndef DOXYGEN_IGNORE
	sph_u32 partial;
	unsigned partial_len;
	unsigned round_shift;
	sph_u32 S[36];
#if SPH_64
	sph_u64 bit_count;
#else
	sph_u32 bit_count_high, bit_count_low;
#endif
#endif
} sph_fugue_context;

typedef sph_fugue_context sph_fugue224_context;

typedef sph_fugue_context sph_fugue256_context;

typedef sph_fugue_context sph_fugue384_context;

typedef sph_fugue_context sph_fugue512_context;

void sph_fugue224_init(void *cc);

void sph_fugue224(void *cc, const void *data, size_t len);

void sph_fugue224_close(void *cc, void *dst);

void sph_fugue224_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

void sph_fugue256_init(void *cc);

void sph_fugue256(void *cc, const void *data, size_t len);

void sph_fugue256_close(void *cc, void *dst);

void sph_fugue256_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

void sph_fugue384_init(void *cc);

void sph_fugue384(void *cc, const void *data, size_t len);

void sph_fugue384_close(void *cc, void *dst);

void sph_fugue384_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

void sph_fugue512_init(void *cc);

void sph_fugue512(void *cc, const void *data, size_t len);

void sph_fugue512_close(void *cc, void *dst);

void sph_fugue512_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

#ifdef __cplusplus
}
#endif

#endif
