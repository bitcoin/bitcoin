/*-
 * Copyright 2014 Alexander Peslyak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "yescrypt.c"

#ifdef YESCRYPT_FLAGS
#undef YESCRYPT_FLAGS
#endif
#define YESCRYPT_FLAGS \
	(YESCRYPT_RW | YESCRYPT_PARALLEL_SMIX | YESCRYPT_PWXFORM)
#define YESCRYPT_BASE_N 8
#define YESCRYPT_R 8
#define YESCRYPT_P 1

#ifdef TEST
static
#endif
int PHS(void *out, size_t outlen, const void *in, size_t inlen,
    const void *salt, size_t saltlen, unsigned int t_cost, unsigned int m_cost)
{
	yescrypt_shared_t shared;
	yescrypt_local_t local;
	int retval;

	if (yescrypt_init_shared(&shared, NULL, 0,
	    0, 0, 0, YESCRYPT_SHARED_DEFAULTS, 0, NULL, 0))
		return -1;
	if (yescrypt_init_local(&local)) {
		yescrypt_free_shared(&shared);
		return -1;
	}
	retval = yescrypt_kdf(&shared, &local, in, inlen, salt, saltlen,
	    (uint64_t)YESCRYPT_BASE_N << m_cost, YESCRYPT_R, YESCRYPT_P,
	    t_cost, YESCRYPT_FLAGS, out, outlen);
	if (yescrypt_free_local(&local)) {
		yescrypt_free_shared(&shared);
		return -1;
	}
	if (yescrypt_free_shared(&shared))
		return -1;
	return retval;
}

#ifdef TEST
#include <stdio.h>
#include <unistd.h> /* for sysconf() */
#include <sys/times.h>

static void print_hex(const uint8_t *buf, size_t buflen, const char *sep)
{
	size_t i;

	putchar('"');
	for (i = 0; i < buflen; i++)
		printf("\\x%02x", buf[i]);
	printf("\"%s", sep);
}

static void print_PHS(const void *in, size_t inlen,
    const void *salt, size_t saltlen, unsigned int t_cost, unsigned int m_cost)
{
	uint8_t dk[32];

	printf("PHS(");
	print_hex(in, inlen, ", ");
	print_hex(salt, saltlen, ", ");
	printf("%u, %u) = ", t_cost, m_cost);

	if (PHS(dk, sizeof(dk), in, inlen, salt, saltlen, t_cost, m_cost)) {
		puts("FAILED");
		return;
	}

	print_hex(dk, sizeof(dk), "\n");
}

static void print_all_PHS(unsigned int t_cost, unsigned int m_cost)
{
	clock_t clk_tck = sysconf(_SC_CLK_TCK);
	struct tms start_tms, end_tms;
	clock_t start = times(&start_tms), end, start_v, end_v;
	const int count = 0x102;
	size_t inlen;
	int i, j;

	inlen = 0;
	for (i = 0; i < count; i++) {
		uint8_t in[128], salt[16];

		for (j = 0; j < inlen; j++)
			in[j] = (i + j) & 0xff;
		for (j = 0; j < sizeof(salt); j++)
			salt[j] = ~(i + j) & 0xff;

		print_PHS(in, inlen, salt, sizeof(salt), t_cost, m_cost);

		if (++inlen > sizeof(in))
			inlen = 0;
	}

	end = times(&end_tms);

	start_v = start_tms.tms_utime + start_tms.tms_stime +
	    start_tms.tms_cutime + start_tms.tms_cstime;
	end_v = end_tms.tms_utime + end_tms.tms_stime +
	    end_tms.tms_cutime + end_tms.tms_cstime;

	if (end == start)
		end++;
	if (end_v == start_v)
		end_v++;

	fprintf(stderr, "m_cost=%u (%.0f KiB), t_cost=%u\n"
	    "%llu c/s real, %llu c/s virtual (%llu hashes in %.2f seconds)\n",
	    m_cost, (YESCRYPT_BASE_N << m_cost) * YESCRYPT_R / 8.0, t_cost,
	    (unsigned long long)count * clk_tck / (end - start),
	    (unsigned long long)count * clk_tck / (end_v - start_v),
	    (unsigned long long)count, (double)(end - start) / clk_tck);
}

int main(int argc, char *argv[])
{
#if 0
	setvbuf(stdout, NULL, _IOLBF, 0);
#endif

	print_all_PHS(0, 0);
	print_all_PHS(0, 7);
	print_all_PHS(0, 8);
	print_all_PHS(1, 8);
	print_all_PHS(2, 8);
	print_all_PHS(3, 8);
	print_all_PHS(0, 11);

	return 0;
}
#endif
