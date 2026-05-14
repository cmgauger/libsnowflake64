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
 * \file	snowflake64_test.c
 * \copyright	MIT
 * \date	2026
 * \author	Christian Gauger-Cosgrove
 * \version	0.0.1
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CUnit/CUnit.h>

#include <snowflake64.h>
#include "snowflake64_test.h"

/** \cond */
#define	TIMESTAMPS	7
#define	TIMESTAMP_TESTS	8
#define	WORKER_MAX	1024
#define	WORKER_NUMBER	378
#define	SEQUENCE_MAX	4096
/** \endcond */

/**
 * \brief
 */
struct test_state {
	int number;
	int iteration;
};

struct {
	uint64_t timestamp;
	uint64_t flake[TIMESTAMP_TESTS];
} snowflake64_test_data[TIMESTAMPS] = {
	{UINT64_C(0x0000018011E9B839),
	 {UINT64_C(0x14FF572D1E17A1F7), UINT64_C(0x14FF572D1E17A242),
	  UINT64_C(0x14FF572D1E17A2A8), UINT64_C(0x14FF572D1E17A31F),
	  UINT64_C(0x14FF572D1E17A354), UINT64_C(0x14FF572D1E17A4EF),
	  UINT64_C(0x14FF572D1E17AB0F), UINT64_C(0x14FF572D1E17AFF8)}},
	{UINT64_C(0x0000017426A969FC),
	 {UINT64_C(0x000000000017A000), UINT64_C(0x000000000017A000),
	  UINT64_C(0x000000000017A000), UINT64_C(0x000000000017A000),
	  UINT64_C(0x000000000017A000), UINT64_C(0x000000000017A000),
	  UINT64_C(0x000000000017A000), UINT64_C(0x000000000017A000)}},
	{UINT64_C(0x0000018183D55546),
	 {UINT64_C(0x155BD2146157A0F0), UINT64_C(0x155BD2146157A253),
	  UINT64_C(0x155BD2146157A7B3), UINT64_C(0x155BD2146157A9A9),
	  UINT64_C(0x155BD2146157AB31), UINT64_C(0x155BD2146157AD0C),
	  UINT64_C(0x155BD2146157AF7F), UINT64_C(0x155BD2146157AFCE)}},
	{UINT64_C(0x00000181AB118149),
	 {UINT64_C(0x1565A11F6217A1D0), UINT64_C(0x1565A11F6217A536),
	  UINT64_C(0x1565A11F6217A655), UINT64_C(0x1565A11F6217A67C),
	  UINT64_C(0x1565A11F6217A73E), UINT64_C(0x1565A11F6217AC50),
	  UINT64_C(0x1565A11F6217ACE3), UINT64_C(0x1565A11F6217AD19)}},
	{UINT64_C(0x00000182A17A6826),
	 {UINT64_C(0x15A33B591957A527), UINT64_C(0x15A33B591957A665),
	  UINT64_C(0x15A33B591957A733), UINT64_C(0x15A33B591957A76C),
	  UINT64_C(0x15A33B591957AB0A), UINT64_C(0x15A33B591957ADC1),
	  UINT64_C(0x15A33B591957AF7C), UINT64_C(0x15A33B591957AFB0)}},
	{UINT64_C(0x00000183E59E8586),
	 {UINT64_C(0x15F444607157A28F), UINT64_C(0x15F444607157A385),
	  UINT64_C(0x15F444607157A925), UINT64_C(0x15F444607157A9E2),
	  UINT64_C(0x15F444607157AA2B), UINT64_C(0x15F444607157AAF8),
	  UINT64_C(0x15F444607157AE8C), UINT64_C(0x15F444607157AEB0)}},
	{UINT64_C(0x00000184F029A3E5),
	 {UINT64_C(0x1636E7280917A3DB), UINT64_C(0x1636E7280917A792),
	  UINT64_C(0x1636E7280917A991), UINT64_C(0x1636E7280917AD82),
	  UINT64_C(0x1636E7280917AE0E), UINT64_C(0x1636E7280917AE3B),
	  UINT64_C(0x1636E7280917AF3D), UINT64_C(0x1636E7280917AF9E)}}
};

/**
 * \brief
 */
static uint64_t
test_timefunc(void *test_state) {
	struct test_state *state;
	uint64_t ret_value;

	state = (struct test_state *) test_state;

	ret_value = snowflake64_test_data[state -> number].timestamp;

	if (state -> number == 1) {
		state -> number++;
	} else {
		if (state -> iteration == 4096) {
			state -> number++;
			state -> iteration = 0;
		} else {
			state -> iteration++;
		}
	}

	return ret_value;
}

/**
 * \brief
 */
static void
settm(struct tm *t, int yy, int mm, int dd, int hh, int nn, int ss) {
	/* Clear the struct */
	memset(t, 0, sizeof(struct tm));

	/* Set date */
	t -> tm_year = yy - 1900;
	t -> tm_mon  = mm - 1;
	t -> tm_mday = dd;

	/* Set time */
	t -> tm_hour = hh;
	t -> tm_min  = nn;
	t -> tm_sec  = ss;

	return;
}

/**
 * \brief
 */
int
init_snowflake64_suite(void) {
	return 0;
}

/**
 * \brief
 */
int
clean_snowflake64_suite(void) {
	return 0;
}

/**
 * \bref
 */
void
test_snowflake64_init_size_destroy(void) {
	snowflake64_h *sf_mass;
	snowflake64_context_h sf_ctx;
	snowflake64_h sf_gen;
	struct tm test_base;
	int i;

	/* Test the return value of snowflake64_max_worker() */
	CU_ASSERT_EQUAL(snowflake64_max_worker(), 1024);

	/* Test the error handling of the context initializer function; per the
	 * internal comments of the snowflake64_context_init() function, these
	 * error conditions are:
	 *   1. current-time function being NULL
	 *   2. Epoch before the UNIX epoch
	 *   3. Epoch not being a valid date
	 *   4. Milliseconds out of range
	 *     a. Milliseconds less than 0
	 *     b. Milliseconds greater than 999
	 * Given that the date validity is checked with the tested-elsewhere
	 * valid_date() library function, only one invalid date is needed.
	 */
	settm(&test_base, 2009,  7, 10, 19, 17, 10);
	CU_ASSERT_PTR_NULL(sf_ctx = snowflake64_context_init(test_base,  236,
	    UINT32_C(0x238E1F29), NULL, NULL));

	settm(&test_base, 1969,  7, 21,  2, 56, 15);
	CU_ASSERT_PTR_NULL(sf_ctx = snowflake64_context_init(test_base,  763,
	    UINT32_C(0x79E2A9E3), test_timefunc, NULL));

	settm(&test_base, 1975, 14, 79, 96, 85, 75);
	CU_ASSERT_PTR_NULL(sf_ctx = snowflake64_context_init(test_base,  427,
	    UINT32_C(0x79838CB2), test_timefunc, NULL));

	settm(&test_base, 1983,  7, 28, 20, 20, 17);
	CU_ASSERT_PTR_NULL(sf_ctx = snowflake64_context_init(test_base,  -14,
	    UINT32_C(0x02901D82), test_timefunc, NULL));

	settm(&test_base, 2009,  2, 24, 10, 28, 43);
	CU_ASSERT_PTR_NULL(sf_ctx = snowflake64_context_init(test_base, 6031,
	    UINT32_C(0x153EA438), test_timefunc, NULL));

	/* Test the first part of the error handling for the context destructor 
	 * function; context pointer handling.
	 */
	CU_ASSERT_EQUAL(snowflake64_context_destroy(NULL), 1);
	CU_ASSERT_EQUAL(snowflake64_context_destroy(&sf_ctx), 1);

	/* Test the context error handling part of the generator initializer
	 * function.
	 */
	CU_ASSERT_PTR_NULL(sf_gen = snowflake64_generator_init(sf_ctx));

	/* Test the error handling of the "raw" generator initializer function.
	 * Similar to the context initializer, there are multiple errors to be
	 * checked:
	 *   1. current-time function being NULL
	 *   2. Epoch before the UNIX epoch
	 *   3. Epoch not being a valid date
	 *   4. Milliseconds out of range
	 *     a. Milliseconds less than 0
	 *     b. Milliseconds greater than 999
	 *   5. Worker/thread/machine ID out of range
	 *     a. Worker/thread/machine ID less than 0
	 *     b. Worker/thread/machine ID greater than 1023
	 * Similar to the test for the context initializer, only one invalid
	 * date is necessary as the function makes use of the valid_date()
	 * library function to ensure that the passed epoch date is valid.
	 */
	settm(&test_base, 1998, 11, 13, 16, 59, 59);
	CU_ASSERT_PTR_NULL(sf_gen = snowflake64_generator_raw(test_base,  562,
	     175, NULL, NULL));

	settm(&test_base, 1966,  9,  6, 19, 30,  0);
	CU_ASSERT_PTR_NULL(sf_gen = snowflake64_generator_raw(test_base,   26,
	     254, test_timefunc, NULL));

	settm(&test_base, 2012, 49, 32, 72, 93, -4);
	CU_ASSERT_PTR_NULL(sf_gen = snowflake64_generator_raw(test_base,  316,
	     892, test_timefunc, NULL));

	settm(&test_base, 1990,  6, 25,  1,  9, 57);
	CU_ASSERT_PTR_NULL(sf_gen = snowflake64_generator_raw(test_base, -860,
	     699, test_timefunc, NULL));

	settm(&test_base, 2010, 10, 16, 21, 49, 43);
	CU_ASSERT_PTR_NULL(sf_gen = snowflake64_generator_raw(test_base, 9457,
	     179, test_timefunc, NULL));

	settm(&test_base, 1975,  8,  1,  9, 33, 58);
	CU_ASSERT_PTR_NULL(sf_gen = snowflake64_generator_raw(test_base,  741,
	    -970, test_timefunc, NULL));

	settm(&test_base, 1981, 12, 17,  8,  7, 36);
	CU_ASSERT_PTR_NULL(sf_gen = snowflake64_generator_raw(test_base,  701,
	    4639, test_timefunc, NULL));

	/* Test the first part of the error handling for the state destructor
	 * function; state pointer handling.
	 */
	CU_ASSERT_EQUAL(snowflake64_generator_destroy(NULL, NULL), 1);
	CU_ASSERT_EQUAL(snowflake64_generator_destroy(&sf_gen, NULL), 1);

	/* This should create a context, without errors */
	settm(&test_base, 2005,  9, 29,  4,  5, 20);
	CU_ASSERT_PTR_NOT_NULL(sf_ctx = snowflake64_context_init(test_base, 883,
	    UINT32_C(0x42963E5A), test_timefunc, NULL));

	/* This should create a generator, without errors */
	CU_ASSERT_PTR_NOT_NULL(sf_gen = snowflake64_generator_init(sf_ctx));

	/* Test the second part of the error handling for the context
	 * destructor function; the inability to delete a context with active
	 * generators.
	 */
	CU_ASSERT_EQUAL(snowflake64_context_destroy(&sf_ctx), 1);
	CU_ASSERT_PTR_NOT_NULL(sf_ctx);

	/* Test the second part of the error handling for the state destructor
	 * function: inability to delete a context-derived state with an invalid
	 * context.
	 */
	CU_ASSERT_EQUAL(snowflake64_generator_destroy(&sf_gen, NULL), 1);
	CU_ASSERT_PTR_NOT_NULL(sf_gen);

	/* The generator should deallocate successfully */
	CU_ASSERT_EQUAL(snowflake64_generator_destroy(&sf_gen, sf_ctx), 0);

	/* The context should deallocate successfully */
	CU_ASSERT_EQUAL(snowflake64_context_destroy(&sf_ctx), 0);

	/* We need 1024 generator handles… */
	sf_mass = malloc(WORKER_MAX * sizeof(snowflake64_h));
	CU_ASSERT_FATAL(sf_mass != NULL);
	memset(sf_mass, 0, WORKER_MAX * sizeof(snowflake64_h));

	/* Allocate a context for the next test set */
	sf_ctx = snowflake64_context_init(test_base, 883, UINT32_C(0x42963E5A),
	    test_timefunc, NULL);

	/* Test the return value of snowflake64_active_generators() */
	CU_ASSERT_EQUAL(snowflake64_active_generators(sf_ctx), 0);

	/* Test the return value of snowflake64_max_generators() */
	CU_ASSERT_EQUAL(snowflake64_max_generators(sf_ctx), 0);

	/* …to allocate 1024 generators */
	for (i = 0; i < WORKER_MAX; ++i) {
		/* Test the return values of snowflake64_active_generators() and
		 * snowflake64_max_generators().
		 */
		switch (i) {
		case  31:
		case 322:
		case 348:
		case 450:
		case 673:
		case 706:
		case 778:
		case 847:
			CU_ASSERT_EQUAL(snowflake64_active_generators(sf_ctx),
			    i);
			CU_ASSERT_EQUAL(snowflake64_max_generators(sf_ctx), i);
			break;
		}
		sf_mass[i] = snowflake64_generator_init(sf_ctx);
	}

	/* Test that we cannot allocate any more generators */
	CU_ASSERT_PTR_NULL(sf_gen = snowflake64_generator_init(sf_ctx));

	/* Test the return value of snowflake64_active_generators() */
	CU_ASSERT_EQUAL(snowflake64_active_generators(sf_ctx), WORKER_MAX);

	/* Test the return value of snowflake64_max_generators() */
	CU_ASSERT_EQUAL(snowflake64_max_generators(sf_ctx), WORKER_MAX);

	/* Deallocate the massive pile of generators, and generator handles */
	for (i = WORKER_MAX; i > 0; --i) {
		switch (i) {
		case  11:
		case 132:
		case 372:
		case 445:
		case 618:
		case 665:
		case 713:
		case 861:
			CU_ASSERT_EQUAL(snowflake64_active_generators(sf_ctx),
			    i);
			break;
		}
		snowflake64_generator_destroy(&(sf_mass[i - 1]), sf_ctx);
	}
	free(sf_mass);

	/* Test the return value of snowflake64_active_generators() */
	CU_ASSERT_EQUAL(snowflake64_active_generators(sf_ctx), 0);

	/* Test the return value of snowflake64_max_generators() */
	CU_ASSERT_EQUAL(snowflake64_max_generators(sf_ctx), WORKER_MAX);

	/* Deallocate the context */
	snowflake64_context_destroy(&sf_ctx);

	/* This should create a generator, without errors */
	settm(&test_base, 2017, 12, 20, 13, 35, 16);
	CU_ASSERT_PTR_NOT_NULL(sf_gen = snowflake64_generator_raw(test_base,
	    584,   71, test_timefunc, NULL));

	/* The generator should deallocate successfully */
	CU_ASSERT_EQUAL(snowflake64_generator_destroy(&sf_gen, NULL), 0);
	CU_ASSERT_PTR_NULL(sf_gen);

	return;
}

/**
 * \brief
 */
void
test_snowflake64_generate(void) {
	const int test_points[TIMESTAMPS][TIMESTAMP_TESTS + 1] = {
		{ 503,  578,  680,  799,  852, 1263, 2831, 4088, 0},
		{   0,    0,    0,    0,    0,    0,    0,    0, 0},
		{ 240,  595, 1971, 2473, 2865, 3340, 3967, 4046, 0},
		{ 464, 1334, 1621, 1660, 1854, 3152, 3299, 3353, 0},
		{1319, 1637, 1843, 1900, 2826, 3521, 3964, 4016, 0},
		{ 655,  901, 2341, 2530, 2603, 2808, 3724, 3760, 0},
		{ 987, 1938, 2449, 3458, 3598, 3643, 3901, 3998, 0}
	};
	snowflake64_h sf_gen;
	struct test_state state;
	struct tm test_base;
	uint64_t snowflake;
	int i, j, k, x;

	memset(&state, 0, sizeof(struct test_state));
	settm(&test_base, 2010, 11,  4,  1, 42, 54);
	CU_ASSERT_PTR_NOT_NULL(sf_gen = snowflake64_generator_raw(test_base,
	    657, WORKER_NUMBER, test_timefunc, &state));

	/* Test the generated time stamps */
	for (i = 0; i < TIMESTAMPS; ++i) {
		switch (i) {
		case 0:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			for (j = 0, k = 0; j <= SEQUENCE_MAX; ++j) {
				x = snowflake64_generate(&snowflake, sf_gen);
				if (j == test_points[i][k]) {
					CU_ASSERT_EQUAL(x, 0);
					CU_ASSERT_EQUAL(snowflake,
					    snowflake64_test_data[i].flake[k]);
					k++;
				}

				if (j == 4096) {
					CU_ASSERT_EQUAL(x, 1);
					CU_ASSERT_EQUAL(snowflake, 0);
				}
			}
			break;
		case 1:
			x = snowflake64_generate(&snowflake, sf_gen);
			CU_ASSERT_EQUAL(x, 1);
			CU_ASSERT_EQUAL(snowflake, 0);
			break;
		}
	}

	CU_ASSERT_EQUAL(snowflake64_generator_destroy(&sf_gen, NULL), 0);
	CU_ASSERT_PTR_NULL(sf_gen);
	
	return;
}
