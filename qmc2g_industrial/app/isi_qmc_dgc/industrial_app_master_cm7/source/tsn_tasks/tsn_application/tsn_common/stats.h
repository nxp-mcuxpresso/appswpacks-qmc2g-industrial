/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_STATS_H_
#define _COMMON_STATS_H_

#include "stdint.h"
#include "log.h"

struct stats {
    uint32_t log2_size;
    uint32_t current_count;

    int32_t current_min;
    int32_t current_max;
    int64_t current_mean;
    uint64_t current_ms;

    /* Stats snapshot */
    int32_t min;
    int32_t max;
    int32_t mean;
    uint64_t ms;
    uint64_t variance;

    /* absolute min/max (never reset) */
    int32_t abs_min;
    int32_t abs_max;

    void *priv;
    void (*func)(struct stats *s);
};

#define MAX_SLOTS 256

struct hist {
    uint32_t slots[MAX_SLOTS];
    int n_slots;
    int slot_size;
};

void stats_reset(struct stats *s);
void stats_print(struct stats *s);
void stats_update(struct stats *s, int32_t val);
void stats_compute(struct stats *s);

int hist_init(struct hist *hist, unsigned int n_slots, unsigned slot_size);
void hist_update(struct hist *hist, unsigned int value);
void hist_print(struct hist *hist);


/*	stats_init
 *
 *  place of execution: flash
 *
 *  description: Initialize a stats structure.
 *
 *  params:     s: Pointer to structure to be initialized
 * 				log2_size: Set size to be reached before statistics are computed, expressed as a power of 2
 *              @priv: private field for use by func
 *              @func: pointer to the function to be called when stats are computed
 */
static inline void stats_init(struct stats *s, unsigned int log2_size, void *priv, void (*func)(struct stats *s))
{
    s->log2_size = log2_size;
    s->priv = priv;
    s->func = func;

    s->abs_min = 0x7fffffff;
    s->abs_max = -0x7fffffff;

    stats_reset(s);
}

#endif /* _COMMON_STATS_H_ */
