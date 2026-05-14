/*
 * Copyright (c) 2026 Christian Gauger-Cosgrove
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/**
 * \file	test.h
 * \copyright	MIT
 * \date	2026
 * \author	Christian Gauger-Cosgrove
 * \version	0.0.1
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <argtable3.h>
#include <logger.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>

#include "snowflake64_test.h"

#define	TEST_VER_MAJ	0
#define	TEST_VER_MIN	0
#define	TEST_VER_PAT	1

enum MODE {
	MODE_BATCH,
	MODE_INTERACTIVE
};

enum LEVEL {
	LEVEL_QUIET,
	LEVEL_NORMAL,
	LEVEL_VERBOSE
};

struct test_setup_function {
	CU_ErrorCode (*function)(void);
	const char *name;
};

struct test_suite {
	void (*test)(void);
	const char *name;
};

static void	 version(char *);
static void	 usage(void *, void *, void *, char *);
static int	 test(enum MODE, enum LEVEL);
static CU_ErrorCode	 setup_snowflake64_suite(void);

int
main(int argc, char *argv[]) {
	struct arg_lit *verbose_batch, *verbose_interactive, *quiet_batch,
	    *quiet_interactive, *help, *ver;
	struct arg_rex *interactive, *batch;
	struct arg_end *end_batch, *end_interactive, *end_misc;
	void *argtable_batch[] = {
		batch = arg_rex1(NULL, NULL, "batch", NULL, ARG_REX_ICASE, NULL),
		quiet_batch = arg_litn("q", "quiet", 0, 1,
		    "run tests with minimal messages"),
		verbose_batch = arg_litn("v", "verbose", 0, 1,
		    "run tests with maximal messages"),
		end_batch = arg_end(20)
	};
	void *argtable_interactive[] = {
		interactive = arg_rex1(NULL, NULL, "interactive", NULL,
		    ARG_REX_ICASE, NULL),
		quiet_interactive = arg_litn("q", "quiet", 0, 1,
		    "run tests with minimal messages"),
		verbose_interactive = arg_litn("v", "verbose", 0, 1,
		    "run tests with maximal messages"),
		end_interactive = arg_end(20)
	};
	void *argtable_misc[] = {
		help = arg_litn(NULL, "help", 0, 1,
		    "display this help and exit"),
		ver = arg_litn(NULL, "version", 0, 1,
		    "display version info and exit"),
		end_misc = arg_end(20)
	};
	int nerr_batch, nerr_interactive, nerr_misc, rc;

	/* Pre-initializations */
	rc = 0;

	/* Initialize C-LOGGER */
	logger_initConsoleLogger(stderr);
	logger_setLevel(LogLevel_TRACE);

	/* Verify all of the argtable[] entries were allocated */
	if (arg_nullcheck(argtable_batch) != 0 ||
	    arg_nullcheck(argtable_interactive) != 0 ||
	    arg_nullcheck(argtable_misc)) {
		/* Something didn't get initialized, so we've OOM'd */
		LOG_FATAL("insufficient memory", NULL);
		rc = 1;
		goto MAIN_EXIT;
	}

	/* We've defined multiple argtables for each command line syntax, let's
	 * go ahead and parse the command line.
	 */
	nerr_batch = arg_parse(argc, argv, argtable_batch);
	nerr_interactive = arg_parse(argc, argv, argtable_interactive);
	nerr_misc = arg_parse(argc, argv, argtable_misc);

	/* Determine which of the three syntaxes was selected */
	if (nerr_batch == 0) {
		if (verbose_batch -> count != 0) {
			rc = test(MODE_BATCH, LEVEL_VERBOSE);
		} else if (quiet_batch -> count != 0) {
			rc = test(MODE_BATCH, LEVEL_QUIET);
		} else {
			rc = test(MODE_BATCH, LEVEL_NORMAL);
		}
	} else if (nerr_interactive == 0) {
		if (verbose_interactive -> count != 0) {
			rc = test(MODE_INTERACTIVE, LEVEL_VERBOSE);
		} else if (quiet_interactive -> count != 0) {
			rc = test(MODE_INTERACTIVE, LEVEL_QUIET);
		} else {
			rc = test(MODE_INTERACTIVE, LEVEL_NORMAL);
		}
	} else if (nerr_misc == 0) {
		if (ver -> count != 0) {
			version(argv[0]);
			rc = 0;
		} else {
			usage(argtable_batch, argtable_interactive,
			    argtable_misc, argv[0]);
			rc = 1;
		}
	} else {
		if (batch -> count != 0) {
			arg_print_errors(stderr, end_batch, argv[0]);
			fprintf(stdout, "usage: %s ", argv[0]);
			arg_print_syntax(stdout, argtable_batch, "\n");
			rc = 1;
		} else if (interactive -> count != 0) {
			arg_print_errors(stderr, end_interactive, argv[0]);
			fprintf(stdout, "usage: %s ", argv[0]);
			arg_print_syntax(stdout, argtable_interactive, "\n");
			rc = 1;
		} else {
			fprintf(stderr, "%s: missing <batch|interactive|curses>"
			    " command.\n", argv[0]);
			usage(argtable_batch, argtable_interactive,
			    argtable_misc, argv[0]);
			rc = 1;
		}
	}

	/* Cleanup the argtables */
	arg_freetable(argtable_batch, sizeof(argtable_batch) /
	    sizeof(argtable_batch[0]));
	arg_freetable(argtable_interactive, sizeof(argtable_interactive) /
	    sizeof(argtable_interactive[0]));
	arg_freetable(argtable_misc, sizeof(argtable_misc) /
	    sizeof(argtable_misc[0]));

MAIN_EXIT:
	/* Leave */
	exit((rc ? EXIT_FAILURE : EXIT_SUCCESS));
}

static void
version(char *pname) {
	fprintf(stdout, "%s %d.%d.%d\n\n\n", pname, TEST_VER_MAJ, TEST_VER_MIN,
	    TEST_VER_PAT);
	fprintf(stdout, "Copyright (c) 2026 Christian Gauger-Cosgrove\n\n"
	    "Permission is hereby granted, free of charge, to any person "
	    "obtaining a copy of this software and associated documentation "
	    "files (the \"Software\"), to deal in the Software without "
	    "restriction, including without limitation the rights to use, "
	    "copy, modify, merge, publish, distribute, sublicense, and/or sell "
	    "copies of the Software, and to permit persons to whom the "
	    "Software is furnished to do so, subject to the following "
	    "conditions:\n\n"
	    "The above copyright notice and this permission notice (including "
	    "the next paragraph) shall be included in all copies or "
	    "substantial portions of the Software.\n\n"
	    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, "
	    "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES "
	    "OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND "
	    "NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT "
	    "HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, "
	    "WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING "
	    "FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR "
	    "OTHER DEALINGS IN THE SOFTWARE.\n");

	return;
}

static void
usage(void *table_batch, void *table_interactive, void *table_misc,
    char *pname) {
	fprintf(stdout, "usage: %s ", pname);
	arg_print_syntax(stdout, table_batch, "\n");
	fprintf(stdout, "       %s ", pname);
	arg_print_syntax(stdout, table_interactive, "\n");
	fprintf(stdout, "       %s ", pname);
	arg_print_syntax(stdout, table_misc, "\n");

	fprintf(stdout, "This program tests the functions of the CryptSim "
	    "library of functions.\n");

	arg_print_glossary(stdout, table_batch, "      %-20s %s\n");
	arg_print_glossary(stdout, table_interactive, "      %-20s %s\n");
	arg_print_glossary(stdout, table_misc, "      %-20s %s\n");

	return;
}

static int
test(enum MODE mode, enum LEVEL level) {
	const struct test_setup_function setup[] = {
		{setup_snowflake64_suite, "snowflake64"},
		{NULL, NULL}
	};
	CU_ErrorCode ec;
	int i;

	/* Pre-initializations */
	ec = CUE_SUCCESS;
	i = 0;

	/* Set the correct logging mode */
	switch (level) {
	case LEVEL_QUIET:
		logger_setLevel(LogLevel_ERROR);
		break;
	case LEVEL_NORMAL:
		logger_setLevel(LogLevel_WARN);
		break;
	case LEVEL_VERBOSE:
		logger_setLevel(LogLevel_TRACE);
		break;
	default:
		logger_setLevel(LogLevel_WARN);
	}

	/* Initialize the test registry */
	LOG_INFO("initializing test registry", NULL);
	if (CUE_SUCCESS != CU_initialize_registry()) {
		LOG_FATAL("failure initializing test registry: %s",
		    CU_get_error_msg());
		ec = CU_get_error();
		goto TEST_RETURN;
	}

	/* Setup the test suites */
	LOG_INFO("setting up test suites", NULL);
	while (setup[i].function != NULL) {
		ec = setup[i].function();
		if (ec != CUE_SUCCESS) {
			LOG_ERROR("failure in '%s' setup", setup[i].name);
			goto TEST_CLEANUP;
		}

		++i;
	}
// +++---1---+++---2---+++---3---+++---4---+++---5---+++---6---+++---7---+++---8

	/* Run the tests */
	LOG_INFO("running CUnit test harness", NULL);
	switch (mode) {
	case MODE_BATCH:
		/* Set the verbosity */
		switch (level) {
		case LEVEL_QUIET:
			CU_basic_set_mode(CU_BRM_SILENT);
			break;
		case LEVEL_NORMAL:
			CU_basic_set_mode(CU_BRM_NORMAL);
			break;
		case LEVEL_VERBOSE:
			CU_basic_set_mode(CU_BRM_VERBOSE);
			break;
		default:
			CU_basic_set_mode(CU_BRM_NORMAL);
		}

		/* Run under the CUnit/Basic interface */
		CU_basic_run_tests();

		break;
	case MODE_INTERACTIVE:
		/* Run under the CUnit/Console interface */
		CU_console_run_tests();

		break;
	default:
		CU_basic_set_mode(CU_BRM_NORMAL);
		CU_basic_run_tests();
	}

	/* Cleanup the test registry */
TEST_CLEANUP:
	LOG_INFO("cleaning up the test registry", NULL);
	CU_cleanup_registry();

	/* Leave */
TEST_RETURN:
	return (ec != CUE_SUCCESS);
}

static CU_ErrorCode
setup_snowflake64_suite(void) {
	const struct test_suite test[] = {
		{test_snowflake64_init_size_destroy, "Snowflake ID "
		    "Initializers, Destructors, & Count/Limit Test"},
		{test_snowflake64_generate, "Snowflake ID Generator Test"},
		{NULL, NULL}
	};
	CU_pSuite snowflake64_suite;
	CU_ErrorCode ec;
	int i;

	/* Value pre-initialization */
	ec = CUE_SUCCESS;
	i = 0;

	/* Initialize the test suite */
	LOG_INFO("adding 'snowflake64' test suite", NULL);
	snowflake64_suite = CU_add_suite("snowflake64 Suite",
	    init_snowflake64_suite, clean_snowflake64_suite);
	if (snowflake64_suite == NULL) {
		LOG_ERROR("failure adding 'snowflake64' test suite: %s",
		    CU_get_error_msg());
		ec = CU_get_error();
		goto SUITE_EXIT;
	}

	/* Add the tests */
	LOG_INFO("adding 'snowflake64' tests", NULL);
	while (test[i].test != NULL) {
		if (CU_add_test(snowflake64_suite, test[i].name,
		    test[i].test) == NULL) {
			LOG_ERROR("failure adding test: %s",
			    CU_get_error_msg());
			ec = CU_get_error();
			goto SUITE_EXIT;
		}

		++i;
	}

SUITE_EXIT:
	return ec;
}
