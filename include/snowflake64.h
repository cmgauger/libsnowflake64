#ifndef SNOWFLAKE64_H
#define SNOWFLAKE64_H
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
 * \file	snowflake64.h
 * \copyright	MIT
 * \date	2025
 * \author	Christian Gauger-Cosgrove
 * \version	0.0.1
 */
#include <stdint.h>
#include <time.h>

/**
 * \brief Snowflake ID context handle
 *
 * Opaque handle for the Snowflake ID context.
 */
typedef struct snowflake64_context_s *snowflake64_context_h;

/**
 * \brief Snowflake ID generator state handle
 *
 * Opaque handle for the Snowflake ID generator state.
 */
typedef struct snowflake64_s *snowflake64_h;

snowflake64_context_h snowflake64_context_init(struct tm, int, uint32_t,
    uint64_t (*)(void *), void *);
snowflake64_h snowflake64_generator_init(snowflake64_context_h);
snowflake64_h snowflake64_generator_raw(struct tm, int, int,
    uint64_t (*)(void *), void *);
int snowflake64_max_worker(void);
int snowflake64_max_generators(snowflake64_context_h);
int snowflake64_active_generators(snowflake64_context_h);
int snowflake64_context_destroy(snowflake64_context_h *);
int snowflake64_generator_destroy(snowflake64_h *, snowflake64_context_h);
int snowflake64_generate(uint64_t *, snowflake64_h);

#endif /* SNOWFLAKE64_H */
