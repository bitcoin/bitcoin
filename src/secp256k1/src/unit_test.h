/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_UNIT_TEST_H
#define SECP256K1_UNIT_TEST_H

/* --------------------------------------------------------- */
/* Configurable constants                                    */
/* --------------------------------------------------------- */

/* Maximum number of command-line arguments.
 * Must be at least as large as the total number of tests
 * to allow specifying all tests individually. */
#define MAX_ARGS 150
/* Maximum number of parallel jobs */
#define MAX_SUBPROCESSES 16

/* --------------------------------------------------------- */
/* Test Framework Registry Macros                            */
/* --------------------------------------------------------- */

#define CASE(name) { #name, run_##name }
#define CASE1(name) { #name, name }

#define MAKE_TEST_MODULE(name) { \
    #name, \
    tests_##name, \
    sizeof(tests_##name) / sizeof(tests_##name[0]) \
}

/* Macro to wrap a test internal function with a COUNT loop (iterations number) */
#define REPEAT_TEST(fn) REPEAT_TEST_MULT(fn, 1)
#define REPEAT_TEST_MULT(fn, multiplier)            \
    static void fn(void) {                          \
        int i;                                      \
        int repeat = COUNT * (multiplier);          \
        for (i = 0; i < repeat; i++)                \
            fn##_internal();                        \
    }



/* --------------------------------------------------------- */
/* Test Framework API                                        */
/* --------------------------------------------------------- */

typedef void (*test_fn)(void);

struct tf_test_entry {
    const char* name;
    test_fn func;
};

struct tf_test_module {
    const char* name;
    const struct tf_test_entry* data;
    int size;
};

typedef int (*setup_ctx_fn)(void);
typedef int (*teardown_fn)(void);
typedef void (*run_test_fn)(const struct tf_test_entry*);

struct tf_targets {
    /* Target tests indexes */
    const struct tf_test_entry* slots[MAX_ARGS];
    /* Next available slot */
    int size;
};

/* --- Command-line args --- */
struct tf_args {
    /* 0 => sequential; 1..MAX_SUBPROCESSES => parallel workers */
    int num_processes;
    /* Specific RNG seed */
    const char* custom_seed;
    /* Whether to print the help msg */
    int help;
    /* Whether to print the tests list msg */
    int list_tests;
    /* Target tests indexes */
    struct tf_targets targets;
    /* Enable test execution logging */
    int logging;
};

/* --------------------------------------------------------- */
/* Public API                                                */
/* --------------------------------------------------------- */

struct tf_framework {
    /* Command-line args */
    struct tf_args args;
    /* Test modules registry */
    const struct tf_test_module* registry_modules;
    /* Num of modules */
    int num_modules;
    /* Registry for tests that require no RNG init */
    const struct tf_test_module* registry_no_rng;
    /* Specific context setup and teardown functions */
    setup_ctx_fn fn_setup;
    teardown_fn fn_teardown;
    /* Test runner function (can be customized) */
    run_test_fn fn_run_test;
};

/*
 * Initialize the test framework.
 *
 * Must be called before tf_run() and before any output is performed to
 * stdout or stderr, because this function disables buffering on both
 * streams to ensure reliable diagnostic output.
 *
 * Parses command-line arguments and configures the framework context.
 * The caller must initialize the following members of 'tf' before calling:
 *   - tf->registry_modules
 *   - tf->num_modules
 *
 * Side effects:
 *   - stdout and stderr are set to unbuffered mode via setbuf().
 *     This allows immediate flushing of diagnostic messages but may
 *     affect performance for other output operations.
 *
 * Returns:
 *   EXIT_SUCCESS (0) on success,
 *   EXIT_FAILURE (non-zero) on error.
 */
static int tf_init(struct tf_framework* tf, int argc, char** argv);

/*
 * Run tests based on the provided test framework context.
 *
 * This function uses the configuration stored in the tf_framework
 * (targets, number of processes, iteration count, etc.) to determine
 * which tests to execute and how to execute them.
 *
 * Returns:
 *   EXIT_SUCCESS (0) if all tests passed,
 *   EXIT_FAILURE (non-zero) otherwise.
 */
static int tf_run(struct tf_framework* tf);

#endif /* SECP256K1_UNIT_TEST_H */
