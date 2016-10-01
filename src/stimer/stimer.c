/**
 * Copyright (c) 2016 Bradley Kim Schleusner < bradschl@gmail.com >
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stddef.h>
#include <stdlib.h>

#include "stimer.h"
#include "timermath/timermath.h"

// -------------------------------------------------------------- Private types

struct stimer {
    // Context
    struct stimer_ctx *                 ctx;


    // Linked list
    struct stimer *                     next;


    // Last get_time_fn checkpoint
    uint32_t                            checkpoint;


    // Expire period
    struct stimer_duration              expire_interval;


    // Elapsed time
    struct stimer_duration              elapsed;
    bool                                is_running;
};


struct stimer_ctx {
    // Timer linked list root
    struct stimer *                     root;


    // Timer math
    struct tm_math                      tm;
    uint32_t                            ns_per_count;


    // Time function
    stimer_get_time_fn                  get_time_fn;
    void *                              hint;
};


// ---------------------------------------------------------- Private functions

// ------------ Timer linking functions

static void
link_timer(struct stimer_ctx * ctx, struct stimer * ts)
{
    ts->ctx = ctx;

    if (NULL == ctx->root) {
        ts->next = NULL;
    } else {
        ts->next = ctx->root;
    }

    ctx->root = ts;
}


static void
unlink_timer(struct stimer * ts)
{
    struct stimer * next = ts->next;
    struct stimer_ctx * ctx = ts->ctx;
    ts->next = NULL;
    ts->ctx = NULL;

    if (NULL != ctx) {
        if (ctx->root == ts) {
            ctx->root = next;
        } else {
            struct stimer * prev = NULL;
            for (prev = ctx->root; NULL != prev; prev = prev->next) {
                if (ts == prev->next) {
                    prev->next = next;
                    break;
                }
            }
        }
    }
}


// --------------- Timer math functions

static inline void
advance_timer_duration(struct stimer_duration * td, uint32_t ns_advance)
{
    // Need to be really careful about overflows in here, hence strange math
    uint32_t headroom = 1000000000u - td->nanoseconds;

    if (ns_advance < headroom) {
        td->nanoseconds += ns_advance;
    } else {
        td->seconds += 1;
        td->nanoseconds = ns_advance - headroom;
    }
}


static inline void
advance_and_checkpoint(struct stimer * ts, struct tm_math * tm, uint32_t now)
{
    if (ts->is_running) {
        int32_t diff = tm_get_diff(tm, now, ts->checkpoint);
        if (diff > 0) {
            uint32_t ns_advance = diff * ts->ctx->ns_per_count;
            advance_timer_duration(&ts->elapsed, ns_advance);
            ts->checkpoint = now;
        }
    }
}


static inline void
advance_and_checkpoint_2(struct stimer * ts)
{
    if (NULL != ts->ctx) {
        struct stimer_ctx * ctx = ts->ctx;
        uint32_t now = ctx->get_time_fn(ctx->hint);
        advance_and_checkpoint(ts, &ctx->tm, now);
    }
}


// ------------- Expire timer functions

static inline void
start_and_checkpoint_timer(struct stimer * ts)
{
    if (NULL != ts->ctx) {
        ts->checkpoint = ts->ctx->get_time_fn(ts->ctx->hint);
        ts->is_running = true;

        ts->elapsed.seconds = 0;
        ts->elapsed.nanoseconds = 0;
    }
}


static inline bool
timer_duration_ge(struct stimer_duration * lhs, struct stimer_duration * rhs)
{
    bool is_ge = false;
    if (lhs->seconds > rhs->seconds) {
        is_ge = true;
    } else if (lhs->seconds == rhs->seconds) {
        if (lhs->nanoseconds >= rhs->nanoseconds) {
            is_ge = true;
        }
    }
    return is_ge;
}


// ----------------------------------------------------------- Public functions

// ---------------------- Timer context

struct stimer_ctx *
stimer_alloc_context(void * hint,
                     stimer_get_time_fn get_time_fn,
                     uint32_t max_time,
                     uint32_t ns_per_count)
{
    struct stimer_ctx * ctx = (struct stimer_ctx *) malloc(sizeof(struct stimer_ctx));

    if (NULL != ctx) {
        ctx->root = NULL;

        tm_initialize(&ctx->tm, max_time);

        ctx->ns_per_count = ns_per_count;
        ctx->get_time_fn = get_time_fn;
        ctx->hint = hint;
    }

    return ctx;
}


void
stimer_free_context(struct stimer_ctx * ctx)
{
    if (NULL != ctx) {
        while (NULL != ctx->root) {
            unlink_timer(ctx->root);
        }

        free(ctx);
    }
}


void
stimer_execute_context(struct stimer_ctx * ctx)
{
    if (NULL != ctx) {
        uint32_t now = ctx->get_time_fn(ctx->hint);

        struct stimer * ts;
        for (ts = ctx->root; NULL != ts; ts = ts->next) {
            advance_and_checkpoint(ts, &ctx->tm, now);
        }
    }
}


// ------------------------------ Timer

struct stimer *
stimer_alloc(struct stimer_ctx * ctx)
{
    struct stimer * ts = NULL;

    if (NULL != ctx) {
        ts = (struct stimer *) malloc(sizeof(struct stimer));
        if (NULL != ts) {
            ts->ctx = NULL;
            ts->next = NULL;

            ts->checkpoint = 0;

            ts->expire_interval.seconds = 0;
            ts->expire_interval.nanoseconds = 0;

            ts->elapsed.seconds = 0;
            ts->elapsed.nanoseconds = 0;
            ts->is_running = false;

            link_timer(ctx, ts);
        }
    }

    return ts;
}


void
stimer_free(struct stimer * ts)
{
    if (NULL != ts) {
        unlink_timer(ts);
        free(ts);
    }
}


// ------------ Elapsed timer functions

void
stimer_start(struct stimer * ts)
{
    if (NULL != ts) {
        start_and_checkpoint_timer(ts);
    }
}


void
stimer_stop(struct stimer * ts)
{
    if ((NULL != ts) && (NULL != ts->ctx)) {
        if (ts->is_running) {
            struct stimer_ctx * ctx = ts->ctx;
            uint32_t now = ctx->get_time_fn(ctx->hint);

            advance_and_checkpoint(ts, &ctx->tm, now);
            ts->is_running = false;
        }
    }
}


void
stimer_get_elapsed_time(struct stimer * ts, struct stimer_duration * t)
{
    if (NULL != ts) {
        if ((ts->is_running) && (NULL != ts->ctx)) {
            struct stimer_ctx * ctx = ts->ctx;
            uint32_t now = ctx->get_time_fn(ctx->hint);

            advance_and_checkpoint(ts, &ctx->tm, now);
        }

        *t = ts->elapsed;
    }
}


// ------------- Expire timer functions

void
stimer_expire_from_now(struct stimer * ts, struct stimer_duration * t)
{
    if ((NULL != ts) && (NULL != t)) {
        start_and_checkpoint_timer(ts);
        ts->expire_interval = *t;
    }
}


void
stimer_expire_from_now_s(struct stimer * ts, uint32_t s)
{
    if (NULL != ts) {
        start_and_checkpoint_timer(ts);
        ts->expire_interval.seconds = s;
        ts->expire_interval.nanoseconds = 0;
    }
}


void
stimer_expire_from_now_ms(struct stimer * ts, uint32_t ms)
{
    if (NULL != ts) {
        start_and_checkpoint_timer(ts);
        ts->expire_interval.seconds = ms / 1000;
        ts->expire_interval.nanoseconds = (ms % 1000) * 1000000;
    }
}


void
stimer_expire_from_now_us(struct stimer * ts, uint32_t us)
{
    if (NULL != ts) {
        start_and_checkpoint_timer(ts);
        ts->expire_interval.seconds = us / 1000000;
        ts->expire_interval.nanoseconds = (us % 1000000) * 1000;
    }
}


void
stimer_expire_from_now_ns(struct stimer * ts, uint32_t ns)
{
    if (NULL != ts) {
        start_and_checkpoint_timer(ts);
        ts->expire_interval.seconds = ns / 1000000000;
        ts->expire_interval.nanoseconds = ns % 1000000000;
    }
}


bool
stimer_is_expired(struct stimer * ts)
{
    bool expired = false;
    if (NULL != ts) {
        advance_and_checkpoint_2(ts);
        expired = timer_duration_ge(&ts->elapsed, &ts->expire_interval);
    }
    return expired;
}


void
stimer_restart_from_now(struct stimer * ts)
{
    if ((NULL != ts) && (ts->is_running)) {
        start_and_checkpoint_timer(ts);
    }
}


void
stimer_advance(struct stimer * ts)
{
    if ((NULL != ts) && (ts->is_running)) {
        advance_and_checkpoint_2(ts);

        if (timer_duration_ge(&ts->elapsed, &ts->expire_interval)) {
            ts->elapsed.seconds -= ts->expire_interval.seconds;
            if (ts->elapsed.nanoseconds >= ts->expire_interval.nanoseconds) {
                ts->elapsed.nanoseconds -= ts->expire_interval.nanoseconds;
            } else {
                ts->elapsed.seconds -= 1;
                ts->elapsed.nanoseconds += (1000000000 - ts->expire_interval.nanoseconds);
            }
        } else {
            ts->elapsed.seconds = 0;
            ts->elapsed.nanoseconds = 0;
        }
    }
}
