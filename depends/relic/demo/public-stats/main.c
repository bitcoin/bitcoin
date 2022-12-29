/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2020 RELIC Authors
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
 * @file
 *
 * Benchmarks for cryptographic protocols.
 *
 * @version $Id$
 * @ingroup bench
 */

#include <stdio.h>

#include "relic.h"
#include "csv.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define STATES 		19
#define GROUPS		3
#define DAYS 		90
#define FACTOR		(1000000)
#define FIXED		((uint64_t)100000)
#define DATABASE	"COVID19-Spain"
#define BEG_2018	"27/03/2018"
#define END_2018	"25/06/2018"
#define BEG_2019	"27/03/2019"
#define END_2019	"25/06/2019"
#define BEG_2020	"2020-03-27"
#define END_2020	"2020-06-25"

/* First value is population in each of the autonomous communities in 2020. */
uint64_t populations[STATES] = {
	8405294, 1316064, 1024381, 1176627, 2188626, 580997, 2410819,
	2030807, 7516544, 4948411, 1067272, 2699299, 6587711, 1479098, 646197,
	2172591, 312719, 84913, 84667
};

/* Total population per age group in 2019. */
uint64_t pyramid[GROUPS] = { 37643844, 4482743, 4566276 };

char *acronyms[STATES] = {
	"AN", "AR", "AS", "IB", "CN", "CB", "CL", "CM", "CT", "VC",
	"EX", "GA", "MD", "MC", "NC", "PV", "RI", "CE", "ML"
};

char *acs[STATES] = {
	"Andalusia", "Aragón", "Asturias", "Balearics", "Canary Islands",
	"Cantabria", "Castile & León", "Castile-La Mancha", "Catalonia",
	"Valencia", "Extremadura", "Galicia", "Madrid", "Murcia",
	"Navarre", "Basque Country", "La Rioja", "Ceuta", "Melilla"
};

/* Population pyramids for autonomous communities, taken from countryeconomy.com */
double pyramids[STATES][GROUPS] = {
	{15.86 + 66.98, 9.06, 17.16 - 9.06},
	{14.12 + 64.23, 10.26, 21.65 - 10.26},
	{10.97 + 63.37, 12.82, 25.66 - 12.82},
	{14.89 + 69.29, 8.62, 15.82 - 8.62},
	{13.20 + 70.57, 8.91, 16.22 - 8.91},
	{13.29 + 64.81, 11.11, 21.90 - 11.11},
	{11.94 + 62.83, 11.41, 25.23 - 11.41},
	{15.11 + 65.91, 8.80, 18.99 - 8.80},
	{15.53 + 65.36, 9.69, 19.12 - 9.69},
	{14.87 + 65.62, 10.15, 19.51 - 10.15},
	{13.66 + 65.70, 9.78, 20.64 - 9.78},
	{11.87 + 62.96, 11.90, 25.16 - 11.90},
	{15.48 + 66.66, 9.13, 18.86 - 9.13},
	{17.18 + 67.04, 8.19, 15.78 - 8.19},
	{15.51 + 64.69, 9.88, 19.80 - 9.88},
	{13.20 + 70.57, 8.91, 16.22 - 8.91},
	{11.87 + 62.96, 11.90, 25.16 - 11.90},
	{20.42 + 67.57, 6.58, 12.02 - 6.58},
	{15.48 + 66.66, 9.13, 17.86 - 9.13},
	//{80.55, 9.59, 9.77} //Spain
};

/* Read data from CSV in a given time interval. */
void read_region(g1_t s[], char *l[], bn_t m[], int *counter,
		uint64_t metric[3], const char *file, int region, char *start,
		char *end, bn_t sk) {
	FILE *stream = fopen(file, "r");
	int found = 0;
	char line[1024];
	char str[3];
	char label[100] = { 0 };
	dig_t n;

	found = 0;
	sprintf(str, "%d", region);
	while (fgets(line, 1024, stream)) {
		if (strstr(line, start) != NULL) {
			found = 1;
		}
		if (strstr(line, end) != NULL) {
			found = 0;
		}
		char **tmp = parse_csv(line);
		char **ptr = tmp;

		if (found && !strcmp(ptr[2], str) && !strcmp(ptr[5], "todos") &&
				strcmp(ptr[7], "todos")) {
			n = atoi(ptr[9]);
			//printf("%s\n", line);
			if (strcmp(ptr[6], "menos_65") == 0) {
				//printf("< 65 = %s\n", ptr[9]);
				metric[0] += n;
			}
			if (strcmp(ptr[6], "65_74") == 0) {
				//printf("65-74 = %s\n", ptr[9]);
				metric[1] += n;
			}
			if (strcmp(ptr[6], "mas_74") == 0) {
				//printf("> 74 = %s\n", ptr[9]);
				metric[2] += n;
			}

			bn_set_dig(m[*counter], n);
			l[*counter] = strdup(ptr[8]);
			cp_mklhs_sig(s[*counter], m[*counter], DATABASE, acs[region - 1],
				l[*counter], sk);
			(*counter)++;
		}

		free_csv_line(tmp);
	}
	fclose(stream);
}

int main(int argc, char *argv[]) {
	uint64_t baseline[GROUPS] = { 0, 0, 0 };
	uint64_t mortality[GROUPS] = { 0, 0, 0 };
	uint64_t expected[GROUPS] = { 0, 0, 0 };
	uint64_t observed[STATES][GROUPS];
	uint64_t ratios[STATES][GROUPS];
	dig_t ft[STATES];
	bn_t res, t[STATES], sk[STATES], m[STATES][3 * GROUPS * DAYS];
	g1_t u, sig, sigs[STATES][3 * GROUPS * DAYS], cs[STATES];
	g2_t pk[STATES];
	char *l[STATES][3 * GROUPS * DAYS];
	dig_t *f[STATES];
	int flen[STATES];
	int counter;
	uint64_t total;
	uint64_t excess;

	if (core_init() != RLC_OK) {
		core_clean();
		return 1;
	}

	if (pc_param_set_any() != RLC_OK) {
		core_clean;
		return 1;
	}

	RLC_TRY {
		/* Initialize and generate keys for signers. */
		bn_null(res);
		bn_new(res);
		g1_null(u);
		g1_new(u);
		g1_null(sig);
		g1_new(sig);
		for (int i = 0; i < STATES; i++) {
			f[i] = RLC_ALLOCA(dig_t, 2 * GROUPS * DAYS);
			bn_null(t[i]);
			bn_new(t[i]);
			bn_null(sk[i]);
			bn_new(sk[i]);
			g1_null(cs[i]);
			g1_new(cs[i]);
			g2_null(sk[i]);
			g2_new(sk[i]);
			cp_mklhs_gen(sk[i], pk[i]);
			for (int j = 0; j < GROUPS; j++) {
				for (int k = 0; k < 3 * DAYS; k++) {
					bn_null(m[i][j * 3 * DAYS + k]);
					bn_new(m[i][j * 3 * DAYS + k]);
					g1_null(sigs[i][j * 3 * DAYS + k]);
					g1_new(sigs[i][j * 3 * DAYS + k]);
					l[i][j * 3 * DAYS + k] = NULL;
				}
			}
		}

		/* Compute current population of every age group in each autonomous community. */
		for (int i = 0; i < STATES; i++) {
			for (int j = 0; j < GROUPS; j++) {
				ratios[i][j] = pyramids[i][j] / 100.0 * populations[i];
			}
		}

		for (int i = 0; i < STATES; i++) {
			counter = 0;
			observed[i][0] = observed[i][1] = observed[i][2] = 0;
			read_region(sigs[i], l[i], m[i], &counter, baseline,
					"data_04_13.csv", i + 1, BEG_2018, END_2018, sk[i]);
			read_region(sigs[i], l[i], m[i], &counter, baseline,
					"data_04_13.csv", i + 1, BEG_2019, END_2019, sk[i]);
			read_region(sigs[i], l[i], m[i], &counter, observed[i], "data.csv",
					i + 1, BEG_2020, END_2020, sk[i]);
		}

		for (int j = 0; j < GROUPS; j++) {
			mortality[j] = FIXED * FACTOR / (2 * pyramid[j]) * baseline[j];
		}

		total = excess = 0;
		for (int i = 0; i < STATES; i++) {
			printf("%s -- %s:\n", acronyms[i], acs[i]);

			for (int j = 0; j < GROUPS; j++) {
				//expected[j] = (FIXED * ratios[i][j]/(2*pyramid[j])) * baseline[j];
				expected[j] = mortality[j] * ratios[i][j] / (FIXED * FACTOR);
			}

			printf("\texpected : %lu %lu %lu\n", expected[0], expected[1],
					expected[2]);
			printf("\tobserved : %lu %lu %lu\n", observed[i][0], observed[i][1],
					observed[i][2]);

			printf("\ttotal expected: %lu\n",
					(expected[0] + expected[1] + expected[2]) / FIXED);
			printf("\ttotal observed: %lu\n",
					observed[i][0] + observed[i][1] + observed[i][2]);

			total += (expected[0] + expected[1] + expected[2]);
			excess += (observed[i][0] + observed[i][1] + observed[i][2]);
		}

		util_banner("Plaintext computation:", 1);

		printf("Baseline : %6lu %6lu %6lu\n", baseline[0] / 2, baseline[1] / 2,
				baseline[2] / 2);
		printf("Demograph: %6lu %6lu %6lu\n", pyramid[0] / FACTOR,
				pyramid[1] / FACTOR, pyramid[2] / FACTOR);
		printf("Mortality: %6lu %6lu %6lu\n", mortality[0] / FIXED,
				mortality[1] / FIXED, mortality[2] / FIXED);
		printf("Total Expected: %6lu\n", total);
		printf("Total Observed: %6lu\n", excess);

		util_banner("Authenticated computation:", 1);

		bn_zero(res);
		g1_set_infty(u);
		g1_set_infty(sig);
		for (int i = 0; i < STATES; i++) {
			flen[i] = 2 * GROUPS * DAYS;
			for (int j = 0; j < GROUPS; j++) {
				total = 0;
				for (int k = 0; k < STATES; k++) {
					total += FIXED * ratios[k][j] / (2 * pyramid[j]);
				}
				for (int k = 0; k < DAYS; k++) {
					f[i][j * DAYS + k] = f[i][j * DAYS + GROUPS * DAYS + k] =
							total;
				}
			}
			cp_mklhs_fun(t[i], m[i], f[i], 2 * GROUPS * DAYS);
			cp_mklhs_evl(u, sigs[i], f[i], 2 * GROUPS * DAYS);
			bn_add(res, res, t[i]);
			g1_add(sig, sig, u);
		}
		g1_norm(sig, sig);
		assert(cp_mklhs_ver(sig, res, t, DATABASE, acs, l[0], f, flen,
			pk, STATES));

		printf("Total Expected: %6lu\n", res->dp[0] / FIXED);

		bn_zero(res);
		g1_set_infty(u);
		g1_set_infty(sig);
		for (int i = 0; i < STATES; i++) {
			flen[i] = GROUPS * DAYS;
			for (int j = 0; j < GROUPS; j++) {
				for (int k = 0; k < DAYS; k++) {
					f[i][j * DAYS + k] = 1;
				}
			}
			cp_mklhs_fun(t[i], &m[i][2 * GROUPS * DAYS], f[i], GROUPS * DAYS);
			cp_mklhs_evl(u, &sigs[i][2 * GROUPS * DAYS], f[i], GROUPS * DAYS);
			bn_add(res, res, t[i]);
			g1_add(sig, sig, u);
		}
		g1_norm(sig, sig);

		printf("Total Observed: %6lu\n", res->dp[0]);

		assert(cp_mklhs_ver(sig, res, t, DATABASE, acs,
				&l[0][2 * GROUPS * DAYS], f, flen, pk, STATES));
		BENCH_ONE("Time elapsed", cp_mklhs_ver(sig, res, t, DATABASE, acs,
				&l[0][2 * GROUPS * DAYS], f, flen, pk, STATES));

		cp_mklhs_off(cs, ft, acs, &l[0][2 * GROUPS * DAYS], f, flen, STATES);
		assert(cp_mklhs_onv(sig, res, t, DATABASE, acs, cs, ft, pk, STATES));
		BENCH_ONE("Time with precomputation", cp_mklhs_onv(sig, res, t,
			DATABASE, acs, cs, ft, pk, STATES));

	} RLC_CATCH_ANY {
		RLC_THROW(ERR_CAUGHT);
	} RLC_FINALLY {
		bn_free(res);
		g1_free(u);
		g1_free(sig);
		for (int i = 0; i < STATES; i++) {
			RLC_FREE(f[i]);
			bn_free(t[i]);
			bn_free(sk[i]);
			g1_free(cs[i]);
			g2_free(pk[i]);
			for (int j = 0; j < GROUPS; j++) {
				for (int k = 0; k < 3 * DAYS; k++) {
					bn_free(m[i][j * 3 * DAYS + k]);
					g1_free(sigs[i][j * 3 * DAYS + k]);
					free(l[i][j * 3 * DAYS + k]);
				}
			}
		}
	}

	core_clean();
	return 0;
}
