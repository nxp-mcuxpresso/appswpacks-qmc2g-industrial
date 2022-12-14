/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "stats.h"
#include "log.h"

/*	stats_reset
 *
 *  place of execution: ITCM
 *
 *  description: reset all the variables below in the passed structure
 *
 *  params:     stats *s - handler for the stats being reset
 */
void stats_reset(struct stats *s)
{
    s->current_count = 0;
    s->current_min = 0x7fffffff;
    s->current_mean = 0;
    s->current_max = -0x7fffffff;
    s->current_ms = 0;
}


/*	stats_print
 *
 *  place of execution: ITCM
 *
 *  description: Example function to be passed to stats_init
 *               This function expects the priv field to be a character string.
 *
 *  usage:      stats_init(s, log2_size, "your string", print_stats);
 *
 *  params:     stats *s - handler for the stats being print
 */
#if PRINT_LEVEL == VERBOSE_DEBUG
void stats_print(struct stats *s)
{
    INF("stats(%p) %s min %d mean %d max %d rms^2 %llu stddev^2 %llu absmin %d absmax %d\n",
        s, s->priv, s->min, s->mean, s->max, s->ms, s->variance, s->abs_min, s->abs_max);
}
#endif

/*	stats_update
 *
 *  place of execution: ITCM
 *
 *  description: Update stats with a given sample. This function adds a sample to the set.
 * 				 If the number of accumulated samples is equal to the requested size of the set,
 *               the following indicators will be computed:
 *                 . minimum observed value,
 *                 . maximum observed value,
 *                 . mean value,
 *                 . square of the RMS (i.e. mean of the squares)
 *                 . square of the standard deviation (i.e. variance)
 *
 *  params:     s: handler for the stats being monitored
 *              val: sample to be added
 */
void stats_update(struct stats *s, int32_t val)
{
    s->current_count++;

    s->current_mean += val;
    s->current_ms += (int64_t)val * val;

    if (val < s->current_min) {
        s->current_min = val;
        if (val < s->abs_min)
            s->abs_min = val;
    }

    if (val > s->current_max) {
        s->current_max = val;
        if (val > s->abs_max)
            s->abs_max = val;
    }

    if (s->current_count == (1U << s->log2_size)) {
        s->ms = s->current_ms >> s->log2_size;
        s->variance = s->ms - ((s->current_mean * s->current_mean) >> (2 * s->log2_size));
        s->mean = s->current_mean >> s->log2_size;

        s->min = s->current_min;
        s->max = s->current_max;

        if (s->func)
            s->func(s);

        stats_reset(s);
    }
}


/*	stats_compute
 *
 *  place of execution: ITCM
 *
 *  description: Compute current stats event if set size hasn't been reached yet.
 *  			 This function computes current statistics for the stats:
 *    				. minimum observed value,
 *    				. maximum observed value,
 *    				. mean value,
 *    				. square of the RMS (i.e. mean of the squares)
 *    				. square of the standard deviation (i.e. variance)
 *
 *  params:     s: handler for the stats being monitored
 *
 */
void stats_compute(struct stats *s)
{
    if (s->current_count) {
        s->ms = s->current_ms / s->current_count;
        s->variance = s->ms - (s->current_mean * s->current_mean) / ((int64_t)s->current_count * s->current_count);
        s->mean = s->current_mean / s->current_count;
    } else {
        s->mean = 0;
        s->ms = 0;
        s->variance = 0;
    }

    s->min = s->current_min;
    s->max = s->current_max;
}


/*	hist_init
 *
 *  place of execution: flash
 *
 *  description: initialize history for the statistics. Maximum number of slots
 *  			 is set by MAX_SLOTS macro
 *
 *  params:     *hist: handler for the history
 *  			n_slots: number of history slots
 *  			slot_size: size of the single slot
 *
 */
int hist_init(struct hist *hist, unsigned int n_slots, unsigned slot_size)
{
    /* One extra slot for last bucket. */

    if ((n_slots + 1) > MAX_SLOTS)
        return -1;

    hist->n_slots = n_slots + 1;
    hist->slot_size = slot_size;

    return 0;
}


/*	hist_update
 *
 *  place of execution: ITCM
 *
 *  description: save the passed value into the history
 *
 *  params:     *hist: handler for the history
 *  			value: sample to be added
 *
 */
void hist_update(struct hist *hist, unsigned int value)
{
    unsigned int slot = value / hist->slot_size;

    if (slot >= hist->n_slots)
        slot = hist->n_slots - 1;

    hist->slots[slot]++;
}


/*	hist_print
 *
 *  place of execution: ITCM
 *
 *  description: used to print history
 *
 *  params:     *hist: handler for the history
 *
 */
#if PRINT_LEVEL == VERBOSE_DEBUG
void hist_print(struct hist *hist)
{
    int i;

    INF("n_slot %d slot_size %d \n", hist->n_slots, hist->slot_size);

    if (app_log_level >= VERBOSE_INFO) {
        for (i = 0; i < (hist->n_slots + 1); i++)
            PRINTF("%lu ", hist->slots[i]);
        PRINTF("\n");
    }
}
#endif
