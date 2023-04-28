/***********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_BENCH_H
#define SECP256K1_BENCH_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if (defined(_MSC_VER) && _MSC_VER >= 1900)
#  include <time.h>
#else
#  include "sys/time.h"
#endif

static int64_t gettime_i64(void) {
#if (defined(_MSC_VER) && _MSC_VER >= 1900)
    /* C11 way to get wallclock time */
    struct timespec tv;
    if (!timespec_get(&tv, TIME_UTC)) {
        fputs("timespec_get failed!", stderr);
        exit(1);
    }
    return (int64_t)tv.tv_nsec / 1000 + (int64_t)tv.tv_sec * 1000000LL;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_usec + (int64_t)tv.tv_sec * 1000000LL;
#endif
}

#define FP_EXP (6)
#define FP_MULT (1000000LL)

/* Format fixed point number. */
static void print_number(const int64_t x) {
    int64_t x_abs, y;
    int c, i, rounding, g; /* g = integer part size, c = fractional part size */
    size_t ptr;
    char buffer[30];

    if (x == INT64_MIN) {
        /* Prevent UB. */
        printf("ERR");
        return;
    }
    x_abs = x < 0 ? -x : x;

    /* Determine how many decimals we want to show (more than FP_EXP makes no
     * sense). */
    y = x_abs;
    c = 0;
    while (y > 0LL && y < 100LL * FP_MULT && c < FP_EXP) {
        y *= 10LL;
        c++;
    }

    /* Round to 'c' decimals. */
    y = x_abs;
    rounding = 0;
    for (i = c; i < FP_EXP; ++i) {
        rounding = (y % 10) >= 5;
        y /= 10;
    }
    y += rounding;

    /* Format and print the number. */
    ptr = sizeof(buffer) - 1;
    buffer[ptr] = 0;
    g = 0;
    if (c != 0) { /* non zero fractional part */
        for (i = 0; i < c; ++i) {
            buffer[--ptr] = '0' + (y % 10);
            y /= 10;
        }
    } else if (c == 0) { /* fractional part is 0 */
        buffer[--ptr] = '0'; 
    }
    buffer[--ptr] = '.';
    do {
        buffer[--ptr] = '0' + (y % 10);
        y /= 10;
        g++;
    } while (y != 0);
    if (x < 0) {
        buffer[--ptr] = '-';
        g++;
    }
    printf("%5.*s", g, &buffer[ptr]); /* Prints integer part */
    printf("%-*s", FP_EXP, &buffer[ptr + g]); /* Prints fractional part */
}

static void run_benchmark(char *name, void (*benchmark)(void*, int), void (*setup)(void*), void (*teardown)(void*, int), void* data, int count, int iter) {
    int i;
    int64_t min = INT64_MAX;
    int64_t sum = 0;
    int64_t max = 0;
    for (i = 0; i < count; i++) {
        int64_t begin, total;
        if (setup != NULL) {
            setup(data);
        }
        begin = gettime_i64();
        benchmark(data, iter);
        total = gettime_i64() - begin;
        if (teardown != NULL) {
            teardown(data, iter);
        }
        if (total < min) {
            min = total;
        }
        if (total > max) {
            max = total;
        }
        sum += total;
    }
    /* ',' is used as a column delimiter */
    printf("%-30s, ", name);
    print_number(min * FP_MULT / iter);
    printf("   , ");
    print_number(((sum * FP_MULT) / count) / iter);
    printf("   , ");
    print_number(max * FP_MULT / iter);
    printf("\n");
}

static int have_flag(int argc, char** argv, char *flag) {
    char** argm = argv + argc;
    argv++;
    while (argv != argm) {
        if (strcmp(*argv, flag) == 0) {
            return 1;
        }
        argv++;
    }
    return 0;
}

/* takes an array containing the arguments that the user is allowed to enter on the command-line
   returns:
      - 1 if the user entered an invalid argument
      - 0 if all the user entered arguments are valid */
static int have_invalid_args(int argc, char** argv, char** valid_args, size_t n) {
    size_t i;
    int found_valid;
    char** argm = argv + argc;
    argv++;

    while (argv != argm) {
        found_valid = 0;
        for (i = 0; i < n; i++) {
            if (strcmp(*argv, valid_args[i]) == 0) {
                found_valid = 1; /* user entered a valid arg from the list */
                break;
            }
        }
        if (found_valid == 0) {
            return 1; /* invalid arg found */
        }
        argv++;
    }
    return 0;
}

static int get_iters(int default_iters) {
    char* env = getenv("SECP256K1_BENCH_ITERS");
    if (env) {
        return strtol(env, NULL, 0);
    } else {
        return default_iters;
    }
}

static void print_output_table_header_row(void) {
    char* bench_str = "Benchmark";     /* left justified */
    char* min_str = "    Min(us)    "; /* center alignment */
    char* avg_str = "    Avg(us)    ";
    char* max_str = "    Max(us)    ";
    printf("%-30s,%-15s,%-15s,%-15s\n", bench_str, min_str, avg_str, max_str);
    printf("\n");
}

#endif /* SECP256K1_BENCH_H */
