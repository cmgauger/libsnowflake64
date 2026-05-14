/*
 * Copyright (c) 2025 Christian Gauger-Cosgrove
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
 * \file	snowflake64.c
 * \copyright	MIT
 * \date	2025
 * \author	Christian Gauger-Cosgrove
 * \version	0.0.1
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(__linux__)
#	include <bsd/sys/tree.h>
#else
#	include <sys/tree.h>
#endif

//#include <cryptsim/date_time.h>
//#include <cryptsim/util.h>
#include <snowflake64.h>

/** \cond */
#define	WORKER_MAX	1024
#define	SEQUENCE_MAX	4096
/** \endcond */

/**
 * \brief
 *
 */
struct worker_node {
	int worker_number;
	/**<
	 * worker/thread/machine ID number
	 */
	RB_ENTRY(worker_node) node;
	/**<
	 * node pointer for the reb-black tree
	 */
};

/** \cond */
RB_HEAD(worker_tree, worker_node);
/** \endcond */

/**
 * \brief Snowflake ID generator state
 *
 */
struct snowflake64_context_s {
	struct worker_tree workers;
	/**<
	 * red-black tree containing all worker numbers assigned
	 */
	uint64_t (*timefunc)(void *);
	/**<
	 * function which returns the current time in milliseconds since the
	 *     UNIX epoch
	 */
	void *timefunc_state;
	/**<
	 * optional state variable which is passed to \c timefunc
	 */
	uint64_t base;
	/**<
	 * epoch timestamp of the Snowflake ID
	 */
	uint32_t sequence;
	/**<
	 * worker/thread/machine ID number issuing sequence selection
	 */
	int next_worker;
	/**<
	 * next worker/thread/machine ID number to be issued
	 */
};

/**
 * \brief 
 *
 */
struct snowflake64_s {
	uint64_t (*timefunc)(void *);
	/**<
	 * function which returns the current time in milliseconds since the
	 *     UNIX epoch
	 */
	void *timefunc_state;
	/**<
	 * state variable which is passed to \c timefunc
	 */
	uint64_t base;
	/**<
	 * epoch timestamp of the Snowflake ID
	 */
	uint64_t stored;
	/**<
	 * last generated timestamp
	 */
	uint16_t worker;
	/**<
	 * worker/thread/machine ID number of the Snowflake ID generator
	 */
	uint16_t sequence;
	/**<
	 * number of Snowflake IDs issued for the current timestamp
	 */
};

/** \cond */
RB_PROTOTYPE(worker_tree, worker_node, node, worker_cmp)
/** \endcond */

static uint64_t get_base(struct tm, int);
static uint32_t permute(uint32_t, uint32_t, uint32_t);
static int worker_cmp(struct worker_node *, struct worker_node *);
static int add_worker(struct snowflake64_context_s *, int);
static int delete_worker(struct snowflake64_context_s *, int);
static int count_workers(struct snowflake64_context_s *);
static int flr(int, int);
static int mod(int, int);
static int leap(int);
static int valid_date(struct tm);

/**
 * \brief Snowflake ID maximum number of workers
 *
 * This function returns the maximum worker number that can be assigned.
 *
 *
 *
 * \return maximum worker number
 */
int
snowflake64_max_worker(void) {
	return WORKER_MAX;
}

/**
 * \brief number of Snowflake ID generators created from context
 *
 * Queries the overall number of generators created under the supplied Snowflake
 * ID context.
 *
 *
 *
 * \return total number of generators created on success; -1 on failure
 *
 * \param [in] context  Snowflake ID context variable
 */
int
snowflake64_max_generators(snowflake64_context_h context) {
	struct snowflake64_context_s *ctx;

	if (context == NULL)
		return -1;
	ctx = context;

	return ctx -> next_worker;
}

/**
 * \brief number of Snowflake ID generators still active in context
 *
 * Queries the number of Snowflake ID generators which are still active, meaning
 * generators that have not been destroyed, under the supplied Snowflake ID
 * context.
 *
 *
 *
 * \return number of active generators on success; -1 on failure
 *
 * \param [in] context  Snowflake ID context variable
 */
int
snowflake64_active_generators(snowflake64_context_h context) {
	struct snowflake64_context_s *ctx;

	/* Check the validity of the context parameter */
	if (context == NULL)
		return -1;
	ctx = context;

	return count_workers(ctx);
}

/**
 * \brief Snowflake ID context initializer
 *
 * Allocates and initializes a context variable from which to derive Snowflake
 * ID generator state variables.  The Snowflake ID epoch is required, and the
 * context initializer will sanity check the supplied <tt>struct tm</tt> and
 * millisecond count; further the initializer will also check that the function
 * to get the current time in milliseconds is present.  The time-function state
 * variable is optional, and can be NULL if it is unneeded.
 *
 * The time function must provide the current time in milliseconds since the
 * UNIX epoch (January 1, 1970 at 00:00:00).  The following code exceprt is a
 * genericized, in that it will run on Windows and Linux systems, implementation
 * of the time function, if the reader wishes for a starting point for their own
 * implementation:
 * \code{.c}
uint64_t
get_current_time(void *state) {
#if defined(_MSC_VER) && (defined(_WIN32) || defined(_WIN64))
	FILETIME f;
#elif defined(_HAVE_CLOCK_GETTIME)
	struct timestamp s;
#else
	struct timeval v;
#endif
	uint64_t x;

#if defined(_MSC_VER) && (defined(_WIN32) || defined(_WIN64))
	// The clock_gettime() function is unavailable in Visual Studio, thus
	// use the WinAPI GetSystemTimeAsFileTime() function instead.
	GetSystemTimeAsFileTime((LPFILETIME) &f);

	// GetSystemTimeAsFileTime() is actually more precise than we need for a
	// SnowflakeID, so after loading the high- and low-order 32-bit words
	// into a 64-bit word, we reduce the precision from the native range of
	// an NT timestamp (100 ns increments) to the millisecond range.
	x = ((uint64_t) f.dwHighDateTime) << 32 | ((uint64_t) f.dwLowDateTime);
	x = (x - (x % 10000)) / 10000;
#elif defined(_HAVE_CLOCK_GETTIME)
	// If our system provides the clock_gettime() function, we will use it.
	clock_gettime(CLOCK_REALTIME, &s);

	// Similar to the Windows branch using GetSystemTimeAsFileTime() above,
	// clock_gettime() is overly precise, so we reduce the precision from
	// the nanosecond range down to the millisecond range.
	x = (s.tv_sec * 1000) + ((s.tv_nsec - (s.tv_nsec % 1000000)) / 1000000);
#else
	// If all else fails, we will use the gettimeofday() function.
	gettimeofday(&v, NULL);

	// Again we need to reduce the range of the resulting time, this time we
	// reduce a microsecond precision timestamp, again to millisecond range.
	x = (v.tv_sec * 1000) + ((v.tv_usec - (v.tv_usec % 1000))/ 1000);
#endif

	return x;
}
 * \endcode
 * Note the use of C++ style comments is only to prevent issues with the C
 * preprocessor and documentation system, please replace them with proper C
 * style comments.
 *
 * The sequence variable chooses a sequence to issue worker ID numbers in, using
 * Andrew Kensler's \ref permute function.
 *
 * Once operations with the context are finished, the memory can be free using
 * the \ref snowflake64_context_destroy function.
 *
 *
 *
 * \return Snowfake ID context variable on success; NULL on failure
 *
 * \param [in] epoch  the <tt>struct tm</tt> with the date and time which
 *     defines the epoch of the Snowflake ID
 * \param [in] epoch_millis  optional (if not required, set to zero) number of
 *     milliseconds which is added to the epoch time
 * \param [in] sequence  defines the sequence in which the worker/thread/machine
 *     IDs will be issued to generators derived from the context
 * \param [in] timefunc  function which returns the current time in milliseconds
 *     since the UNIX epoch
 * \param [in,out] timefunc_state  optional (if not required, set to NULL) state
 *     variable which is passed to \c timefunc
 */
snowflake64_context_h
snowflake64_context_init(struct tm epoch, int epoch_millis,
    uint32_t sequence, uint64_t (*timefunc)(void *), void *timefunc_state) {
	struct snowflake64_context_s *ctx;

	/* The following conditions must be met to be able to allocate amd
	 * create a Snowflake ID context:
	 *  1. The timestamp function must be defined;
	 *  2. The epoch must not be earlier than January 1, 1970;
	 *  3. The epoch must represent a valid date;
	 *  4. The epoch millisecond count cannot be less than 0; and,
	 *  5. The epoch millisecond count cannot be greater than 999.
	 * If any condition is not met, immediately bail.
	 *
	 * Note that there is no need to verify if the timestamp function state
	 * variable is not NULL, as it is wholly possible to provide a timestamp
	 * function which does not require state.
	 */
	if ((timefunc == NULL) || !valid_date(epoch) || (epoch.tm_year < 70) ||
	    (epoch_millis < 0) || (epoch_millis > 999))
		return NULL;

	/* Allocate and zeroize a block of memory for the structure. */
	ctx = malloc(sizeof(struct snowflake64_context_s));
	if (ctx == NULL)
		return NULL;
	memset(ctx, 0, sizeof(struct snowflake64_context_s));

	/* Populate the context data structure */
	RB_INIT(&(ctx -> workers));
	ctx -> timefunc_state = timefunc_state;
	ctx -> timefunc = timefunc;
	ctx -> base = get_base(epoch, epoch_millis);
	ctx -> next_worker = 0;
	ctx -> sequence = sequence;

	return ctx;
}

/**
 * \brief Snowflake ID context destructor
 *
 * Destroys the Snowflake ID context that was initialized by a call to the
 * \ref snowflake64_init_context function.  It is not possible to destroy a
 * context that has un-destroyed generator state variables.
 *
 *
 *
 * \return 0 on success; 1 on failure
 *
 * \param [in,out] context pointer to the Snowflake ID context variable to be
 *     destroyed
 */
int
snowflake64_context_destroy(snowflake64_context_h *context) {
	struct snowflake64_context_s *ctx;

	/* Check that the context is valid */
	if ((context == NULL) || (*context == NULL))
		return 1;

	ctx = *context;

	/* If generators still exist, we can't delete the context */
	if (!RB_EMPTY(&(ctx -> workers)))
		return 1;

	/* We can now obliterate the context */
	memset(ctx, 0, sizeof(struct snowflake64_context_s));
	free(ctx);
	ctx = NULL;
	
	*context = NULL;

	return 0;
}

/**
 * \brief Get Snowflake ID generator state from Snowflake ID context
 *
 * Creates a new Snowflake ID generator state from the supplied Snowflake ID
 * context.  All of the required information for the generator is taken from, or
 * derived from the context.
 *
 * When no longer needed, the memory allocated to the generator state variable
 * is to be freed using the \ref snowflake64_generator_destroy function.
 *
 *
 *
 * \return Snowflake ID generator state if success; NULL if failure
 *
 * \param [in,out] context  Snowflake ID context variable from which the
 *     generator will be derived
 */
snowflake64_h
snowflake64_generator_init(snowflake64_context_h context) {
	struct snowflake64_context_s *ctx;
	struct snowflake64_s *st;
	int w;

	/* Check the validity of the context parameter */
	if (context == NULL)
		return NULL;
	ctx = context;

	/* Get the next worker number */
	w = ctx -> next_worker;

	/* If we've used up all the worker slots, we abort */
	if (w >= WORKER_MAX)
		return NULL;

	/* Allocate the generator state structure */
	st = malloc(sizeof(struct snowflake64_s));
	if (st == NULL)
		return NULL;
	memset(st, 0, sizeof(struct snowflake64_s));

	/* Add the generator to the tree */
	if (add_worker(context, (int) permute((uint32_t) w, WORKER_MAX,
	    ctx -> sequence))) {
		/* Since add_worker failed, we need to cleanup before bailing */
		free(st);
		return NULL;
	}

	/* Initialize the structure values. */
	st -> timefunc_state = ctx -> timefunc_state;
	st -> timefunc = ctx -> timefunc;
	st -> base = ctx -> base;
	st -> stored = st -> base - 1;
	st -> worker = (uint16_t) permute((uint32_t) w, WORKER_MAX,
	    ctx -> sequence);
	st -> sequence = 0;

	return st;
}

/**
 * \brief Snowflake ID state variable initializer
 *
 * Creates a new, standalone, Snowflake ID generator from the parameters passed
 * in to the function; refer to \ref snowflake64_context_init for an
 * explanation of the <tt>epoch</tt>, <tt>epoch_millis</tt>, <tt>timefunc</tt>,
 * and <tt>timefucn_state</tt> parameters.  The <tt>worker</tt> parameter is the
 * manually specified worker/thread/machine ID number the generator is to use
 * (which in a generator created using \ref snowflake64_generator_init is derived
 * from the context variable).
 *
 * When no longer needed, the memory allocated to the generator state variable
 * is to be freed using the \ref snowflake64_generator_destroy function.
 *
 *
 *
 * \return Snowflake ID generator state if success; NULL if failure
 *
 * \param [in] epoch  the <tt>struct tm</tt> with the date and time which
 *     defines the epoch of the Snowflake ID
 * \param [in] epoch_millis  optional (if not required, set to zero) number of
 *     milliseconds which is added to the epoch time
 * \param [in] worker  worker/thread/machine ID number of the generator
 * \param [in] timefunc  function which returns the current time in milliseconds
 *     since the UNIX epoch
 * \param [in,out] timefunc_state  optional (if not required, set to NULL) state
 *     variable which is passed to \c timefunc
 */
snowflake64_h
snowflake64_generator_raw(struct tm epoch, int epoch_millis, int worker,
    uint64_t (*timefunc)(void *), void *timefunc_state) {
	struct snowflake64_s *st;

	/* The required conditions for the SNOWFLAKE64 generator to function are
	 * as follows:
	 *  1. The timestamp function must be defined;
	 *  2. The worker/thread/machine ID cannot be less than 0;
	 *  3. The worker/thread/machine ID cannot be greater than 1023;
	 *  4. The epoch must not be earlier than January 1, 1970;
	 *  5. The epoch must represent a valid date;
	 *  6. The epoch millisecond count cannot be less than 0; and,
	 *  7. The epoch millisecond count cannot be greater than 999.
	 * If any condition is not met, immediately bail.
	 *
	 * Note that there is no need to verify if the timestamp function state
	 * variable is not NULL, as it is wholly possible to provide a timestamp
	 * function which does not require state.
	 */
	if ((timefunc == NULL) || (worker < 0) || !(worker < WORKER_MAX) ||
	    !valid_date(epoch) || (epoch.tm_year < 70) || (epoch_millis < 0) ||
	    (epoch_millis > 999))
		return NULL;

	/* Allocate and zeroize a block of memory for the structure. */
	st = malloc(sizeof(struct snowflake64_s));
	if (st == NULL)
		return NULL;
	memset(st, 0, sizeof(struct snowflake64_s));

	/* Initialize the structure values. */
	st -> timefunc_state = timefunc_state;
	st -> timefunc = timefunc;
	st -> base = get_base(epoch, epoch_millis);
	st -> stored = st -> base - 1;
	st -> worker = ((uint16_t) worker) | UINT16_C(0x0400);
	st -> sequence = 0;

	return st;
}

/**
 * \brief Snowflake ID generator state destructor
 *
 * Destroys a Snowflake ID generator state that was created by calls to either
 * of the \ref snowflake64_generator_init or \ref snowflake64_generator_raw
 * functions.
 *
 * If attempting to destroy a Snowflake ID generator state that was derived from
 * a Snowflake ID context, you must provide the context as the second parameter
 * to the destructor function.  When destroying a "raw" generator state, set the
 * context variable to NULL.
 *
 *
 *
 * \return 0 on success; 1 on failure
 *
 * \param [in,out] state  pointer to the Snowflake ID state variable to be
 *     destroyed
 * \param [in,out] context  optional Snowflake ID context variable from which
 *     the generator was derived
 */
int
snowflake64_generator_destroy(snowflake64_h *state,
    snowflake64_context_h context) {
	struct snowflake64_context_s *ctx;
	struct snowflake64_s *st;
	int raw;

	/* Check the validity of the state parameter */
	if ((state == NULL) || (*state == NULL))
		return 1;
	st = *state;

	/* Now, using the valid state, check if we need a context, and if the
	 * said context is valid.
	 */
	raw = st -> worker & 0x0400;
	if (!raw && (context == NULL))
		return 1;
	ctx = context;

	/* The error checking is over and done with; firstly, if this is not a
	 * "raw" generator, delete this generator from the context.  If it is a
	 * "raw" generator then we skip this.
	 */
	if (!raw) {
		delete_worker(ctx, st -> worker);
	}

	/* We can now obliterate the state */
	memset(st, 0, sizeof(struct snowflake64_s));
	free(st);
	st = NULL;
	
	*state = NULL;

	return 0;
}

/**
 * \brief Snowflake ID generator
 *
 * Generates a Snowflake ID when called, this function is <em>not</em>
 * thread-safe if the generator state is used in multiple threads.  This is due
 * to both: (1) duplication of the worker/thread/machine ID number; and, (2) the
 * generator state variable does not include any mutex locking.
 *
 * If the generator function is unable to generate an ID -- which can only occur
 * if one has already generated 4096 Snowflake IDs in one millisecond, or if the
 * clock has been advanced backwards -- then the Snowflake ID will be set to
 * zero.  Given that -- theoretically -- a zero ID is a valid ID, the return
 * value of the generator function must <em>always</em> be checked to ensure the
 * validity of the ID.
 *
 *
 *
 * \return 0 on success, 1 on failure
 *
 * \param [out] snowflake  generated Snowflake ID
 * \param [in,out] state  Snowflake ID generator state
 */
int
snowflake64_generate(uint64_t *snowflake, snowflake64_h state) {
	uint64_t b, c, s, t, w;
	int cmp;

	/* Get the current time, and compare it with the stored timestamp */
	b = state -> stored;
	c = state -> timefunc(state -> timefunc_state);
	cmp = (b == c) ? 0 : ((b < c) ? +1 : -1);

	/* Set the Snowflake ID value to zero in case we bail out */
	*snowflake = 0;

	switch (cmp) {
	case 0:
		/*
		 * If the stored and current times are the same, first we check
		 * that the sequence number is less than 4095, otherwise we
		 * cannot generate a new ID -- if we can't generate an ID, error
		 * out.  If the sequence number is less than 4095, then we will
		 * increment the sequence number by 1.
		 */
		if (state -> sequence >= (SEQUENCE_MAX - 1)) {
			return 1;
		}
		
		state -> sequence += 1;
		break;
	case 1:
		/*
		 * If the stored and current times differ, we replace the stored
		 * time with the current time, we reset the sequence number to 0
		 * (as this is the first timestamp generated for this time).
		 */
		state -> stored = c;
		state -> sequence = 0;
		break;
	case -1:
	default:
		/*
		 * If the current time is \b BEFORE the stored time, or the
		 * comparator is any value other than -1, 0, or +1, something is
		 * quite wrong, so bail out immediately.
		 */
		return 1;
	}

	/* Convert the timestamps into the offset */
	t = (((state -> stored - state -> base) &
	    UINT64_C(0x000003FFFFFFFFFF)) << 22) &
	    UINT64_C(0x7FFFFFFFFFFFFFFF);
	
	/* Set the worker ID and sequence number temporary values */
	w = (((uint64_t) state -> worker) & UINT16_C(0x03FF)) << 12;
	s = state -> sequence;

	/* Combine the segments and set the ID */
	*snowflake = t | w | s;

	return 0;
}

/** \cond */
RB_GENERATE(worker_tree, worker_node, node, worker_cmp)
/** \endcond */

/**
 * \brief Converts the epoch <tt>struct tm</tt> and milliseconds to base value
 *
 * \return base value
 */
static uint64_t
get_base(struct tm epoch, int millis) {
	uint64_t tt;
	int a, dd, mm, yy;

	if (!valid_date(epoch) ||
	    (epoch.tm_year > 5877590) || (epoch.tm_year < -5881388))
		return 0;

	yy = epoch.tm_year + 1899;
	mm = epoch.tm_mon;
	dd = epoch.tm_mday;

	a = (yy * 365) + flr(yy, 4) - flr(yy, 100) + flr(yy, 400) + dd +
	    (31 * mm) - ((mm > 1) ? ((23 + 4 * (mm + 1)) / 10) : 0) +
	    ((mm > 1) ? leap(yy + 1) : 0);

	tt  = (uint64_t) (a - 719163);
	tt *= 86400;
	tt += (uint64_t) (epoch.tm_hour * 3600 + epoch.tm_min * 60 +
	    epoch.tm_sec);
	tt *= 1000;
	tt += (uint64_t) millis;

	return tt;
}

/**
 * \brief
 *
 */
static uint32_t
permute(uint32_t index, uint32_t length, uint32_t seed) {
	uint32_t mask;

	mask = length -1;
	mask |= mask >> 1;
	mask |= mask >> 2;
	mask |= mask >> 4;
	mask |= mask >> 8;
	mask |= mask >> 16;

	do {
		index ^= seed;
		index *= UINT32_C(0xE170893D);
		index ^= seed >> 16;
		index ^= (index & mask) >> 4;
		index ^= seed >> 8;
		index *= UINT32_C(0x0929EB3F);
		index ^= seed >> 23;
		index ^= (index & mask) >> 1;
		index *= 1 | seed >> 27;
		index *= UINT32_C(0x6935FA69);
		index ^= (index & mask) >> 11;
		index *= UINT32_C(0x74DCB303);
		index ^= (index & mask) >> 2;
		index *= UINT32_C(0x9E501CC3);
		index ^= (index & mask) >> 2;
		index *= UINT32_C(0xC860A3DF);
		index &= mask;
		index ^= index >> 5;
	} while (index >= length);

	return (index + seed) % length;
}

/**
 * \brief integer floored division function
 *
 * Calculates the division of the numerator by the denominator and returns the
 * largest integer less than or equal to the result.  Two error conditions are
 * verified before the calculation is run, namely that of the denominator being
 * zero (to avoid division by zero) and that of the numerator being equal to
 * <tt>INT_MIN</tt> (to avoid overflow); if either error condition is valid, a
 * zero is returned.
 *
 * Results of running the Bounded Model Checker for C (CBMC) on this function:
 * \code{.unparsed}
** 0 of 509 failed (1 iterations)
VERIFICATION SUCCESSFUL
 * \endcode
 *
 * \ref rmq
 *
 *
 *
 * \return largest integer less than or equal to the numerator divided by the
 *     denominator; zero if an erroneous input is provided
 *
 * \param[in]  numerator  dividend
 * \param[in]  denominator  divisor
 */
static int
flr(int numerator, int denominator) {
	if ((denominator == 0) || (numerator == INT_MIN))
		return 0;

	return (numerator / denominator - (((numerator % denominator) != 0) &&
	    ((numerator ^ denominator) < 0)));
}

/**
 * \brief integer modulo function, Knuth's floored division type
 *
 * Calculates the modulo of the numerator divided by the denominator, using
 * Knuth's floored division algorithm, that is:
 * \f[
 *     x - y \left\lfloor \frac{x}{y} \right\rfloor
 * \f]
 * Two error conditions are verified before the calculation is run, namely: that
 * of the denominator being zero (to avoid division by zero), and that of the
 * numerator being equal to <tt>INT_MIN</tt> (to avoid overflow). If either
 * error condition is encountered, a zero is returned.
 *
 * Results of running the Bounded Model Checker for C (CBMC) on this function:
 * \code{.unparsed}
** 0 of 509 failed (1 iterations)
VERIFICATION SUCCESSFUL
 * \endcode
 *
 * \ref rmq
 *
 *
 *
 * \return modulo of the numerator divided by the denominator; zero if an
 *     erroneous input is provided
 *
 * \param[in]  numerator  dividend
 * \param[in]  denominator  divisor
 */
static int
mod(int numerator, int denominator) {
	int64_t n, d, m;
	int f;

	if ((denominator == 0) || (numerator == INT_MIN))
		return 0;

	n = numerator;
	d = denominator;
	f = flr(numerator, denominator);

	m = (n - (d * f));

	if ((m < INT_MIN) || (m > INT_MAX))
		return 0;

	return (int) m;
}

/**
 * \brief leap year test routine
 *
 * This routine tests if a specified year is a leap year in the proleptic
 * Gregorian calendar.
 *
 * Results of running the Bounded Model Checker for C (CBMC) on this function:
 * \code{.unparsed}
** 0 of 509 failed (1 iterations)
VERIFICATION SUCCESSFUL
 * \endcode
 *
 *
 *
 * \return 0 if regular year, 1 if leap year
 *
 * \param[in]  year  year, in the proleptic Gregorian calendar
 */
static int
leap(int year) {
	return ((mod(year, 4) == 0) && ((mod(year, 100) != 0) ||
	    (mod(year, 400) == 0)));
}

/**
 * \brief <tt>struct tm</tt> date validation routine
 *
 * This function validates that a date stored in a <tt>struct tm</tt> structure
 * is valid.  "Validity" being defined as having values in the correct ranges
 * for the year, month, and day variables.  Only the year, month, and day values
 * of the <tt>struct tm</tt> structure are verified, the remainder of the
 * structure is ignored.
 *
 * Results of running the Bounded Model Checker for C (CBMC) on this function:
 * \code{.unparsed}
** 0 of 509 failed (1 iterations)
VERIFICATION SUCCESSFUL
 * \endcode
 *
 *
 *
 * \return 1 if valid; 0 if not
 *
 * \param[in]  dt  date/time structure to be validated
 */
static int
valid_date(struct tm dt) {
	const int days_in_month[2][12] = {
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};
	int l;
	
	/* Verify that tm_year is within an acceptable range. */
	if (dt.tm_year > 2147481747)
		return 0;

	/* Establish if the year specified is a leap year Gregorian calendar. */
	l = leap(dt.tm_year + 1900);

	/* Check that the tm_mon, tm_mday values are all in valid ranges -- this
	 * is what we needed the leap year data for.  The correctness of the
	 * tm_year value was established earlier.
	 */
	if ((dt.tm_mon < 0) || (dt.tm_mon > 11) ||
	    (dt.tm_mday < 1) || (dt.tm_mday > days_in_month[l][dt.tm_mon]))
		return 0;

	return 1;
}

/**
 * \brief
 */
static int
worker_cmp(struct worker_node *a, struct worker_node *b) {
	return a -> worker_number - b -> worker_number;
}

/**
 * \brief
 */
static int
add_worker(struct snowflake64_context_s *context, int worker) {
	struct worker_node *node;

	node = malloc(sizeof(struct worker_node));
	if (node == NULL)
		return 1;
	memset(node, 0, sizeof(struct worker_node));

	node -> worker_number = worker;

	if (RB_INSERT(worker_tree, &(context -> workers), node) != NULL) {
		free(node);
		return 1;
	}

	context -> next_worker++;

	return 0;
}

/**
 * \brief
 */
static int
delete_worker(struct snowflake64_context_s *context, int worker) {
	struct worker_node *node;
	struct worker_node find;

	find.worker_number = worker;

	node = RB_FIND(worker_tree, &(context -> workers), &find);
	if (node == NULL)
		return 1;

	RB_REMOVE(worker_tree, &(context -> workers), node);

	memset(node, 0, sizeof(struct worker_node));
	free(node);

	return 0;
}

/**
 * \brief
 */
static int
count_workers(struct snowflake64_context_s *context) {
	struct worker_node *node;
	int worker_count;

	worker_count = 0;
	RB_FOREACH(node, worker_tree, &(context -> workers)) {
		worker_count++;
	}

	return worker_count;
}