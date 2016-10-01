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

#include "describe/describe.h"

#include "stimer/stimer.h"


static uint32_t
mock_get_time(void * hint)
{
    uint32_t t = 0;
    if(NULL != hint) {
        t = *((uint32_t *) hint);
    }
    return t;
}


int main(int argc, char const *argv[])
{
    (void) argc;
    (void) argv;

    describe("Timer context") {
        struct stimer_ctx * ctx = NULL;
        it("can be allocated") {
            ctx = stimer_alloc_context(NULL, mock_get_time, 0xFF, 1000000);
            assert_not_null(ctx);
        }

        struct stimer * t1 = NULL;
        struct stimer * t2 = NULL;
        it("can allocate timers") {
            t1 = stimer_alloc(ctx);
            assert_not_null(t1);

            t2 = stimer_alloc(ctx);
            assert_not_null(t2);
        }

        it("can free timers") {
            stimer_free(t1);
            stimer_free(t2);
        }

        it("can be deallocated") {
            stimer_free_context(ctx);
        }
    }

    describe("Timer elapse math") {
        struct stimer_ctx * ctx = NULL;
        uint32_t current_time = 0;

        struct stimer * t1 = NULL;
        struct stimer * t2 = NULL;

        it("test objects can be allocated") {
            ctx = stimer_alloc_context(&current_time, mock_get_time, 0xFF, 1000000);
            assert_not_null(ctx);

            t1 = stimer_alloc(ctx);
            assert_not_null(t1);

            t2 = stimer_alloc(ctx);
            assert_not_null(t2);
        }

        it("can track elapsed time") {
            stimer_start(t1);
            stimer_start(t2);

            struct stimer_duration td;

            stimer_get_elapsed_time(t1, &td);
            assert_equal(0, td.seconds);
            assert_equal(0, td.nanoseconds);

            int i;
            for (i = 0; i < 1001; ++i) {
                current_time += 1;
                stimer_execute_context(ctx);
            }

            stimer_get_elapsed_time(t1, &td);
            assert_equal(1, td.seconds);
            assert_equal(1000000, td.nanoseconds);

            stimer_stop(t1);

            for (i = 0; i < 999; ++i) {
                current_time += 1;
                stimer_execute_context(ctx);
            }

            stimer_get_elapsed_time(t1, &td);
            assert_equal(1, td.seconds);
            assert_equal(1000000, td.nanoseconds);

            stimer_get_elapsed_time(t2, &td);
            assert_equal(2, td.seconds);
            assert_equal(0, td.nanoseconds);
        }

        it("test objects can be deallocated") {
            stimer_free_context(ctx);
            stimer_free(t2);
            stimer_free(t1);
        }
    }

    describe("Timer expire math") {
        struct stimer_ctx * ctx = NULL;
        uint32_t current_time = 0;

        struct stimer * t1 = NULL;
        struct stimer * t2 = NULL;
        struct stimer * t3 = NULL;
        struct stimer * t4 = NULL;
        struct stimer * t5 = NULL;

        it("test objects can be allocated") {
            ctx = stimer_alloc_context(&current_time, mock_get_time, 0xFF, 1000000);
            assert_not_null(ctx);

            t1 = stimer_alloc(ctx);
            assert_not_null(t1);

            t2 = stimer_alloc(ctx);
            assert_not_null(t2);

            t3 = stimer_alloc(ctx);
            assert_not_null(t3);

            t4 = stimer_alloc(ctx);
            assert_not_null(t4);

            t5 = stimer_alloc(ctx);
            assert_not_null(t5);
        }

        it("can expire timers") {
            stimer_expire_from_now_us(t1, 2000);
            stimer_expire_from_now_ns(t2, 3000000);
            stimer_expire_from_now_ms(t3, 4);
            stimer_expire_from_now_s(t4, 1);
            {
                struct stimer_duration td;
                td.seconds = 1;
                td.nanoseconds = 1000000;
                stimer_expire_from_now(t5, &td);
            }

            assert_equal(false, stimer_is_expired(t1));
            assert_equal(false, stimer_is_expired(t2));
            assert_equal(false, stimer_is_expired(t3));
            assert_equal(false, stimer_is_expired(t4));
            assert_equal(false, stimer_is_expired(t5));

            current_time += 1;
            assert_equal(false, stimer_is_expired(t1));
            assert_equal(false, stimer_is_expired(t2));
            assert_equal(false, stimer_is_expired(t3));
            assert_equal(false, stimer_is_expired(t4));
            assert_equal(false, stimer_is_expired(t5));

            current_time += 1;
            assert_equal(true, stimer_is_expired(t1));
            assert_equal(false, stimer_is_expired(t2));
            assert_equal(false, stimer_is_expired(t3));
            assert_equal(false, stimer_is_expired(t4));
            assert_equal(false, stimer_is_expired(t5));

            current_time += 1;
            assert_equal(true, stimer_is_expired(t1));
            assert_equal(true, stimer_is_expired(t2));
            assert_equal(false, stimer_is_expired(t3));
            assert_equal(false, stimer_is_expired(t4));
            assert_equal(false, stimer_is_expired(t5));

            current_time += 1;
            assert_equal(true, stimer_is_expired(t1));
            assert_equal(true, stimer_is_expired(t2));
            assert_equal(true, stimer_is_expired(t3));
            assert_equal(false, stimer_is_expired(t4));
            assert_equal(false, stimer_is_expired(t5));

            int i;
            for(i = 0; i < 995; ++i) {
                current_time += 1;
                stimer_execute_context(ctx);
            }
            assert_equal(true, stimer_is_expired(t1));
            assert_equal(true, stimer_is_expired(t2));
            assert_equal(true, stimer_is_expired(t3));
            assert_equal(false, stimer_is_expired(t4));
            assert_equal(false, stimer_is_expired(t5));

            current_time += 1;
            assert_equal(true, stimer_is_expired(t1));
            assert_equal(true, stimer_is_expired(t2));
            assert_equal(true, stimer_is_expired(t3));
            assert_equal(true, stimer_is_expired(t4));
            assert_equal(false, stimer_is_expired(t5));

            current_time += 1;
            assert_equal(true, stimer_is_expired(t1));
            assert_equal(true, stimer_is_expired(t2));
            assert_equal(true, stimer_is_expired(t3));
            assert_equal(true, stimer_is_expired(t4));
            assert_equal(true, stimer_is_expired(t5));
        }

        it("test objects can be deallocated") {
            stimer_free(t5);
            stimer_free(t4);
            stimer_free(t3);
            stimer_free(t2);
            stimer_free(t1);
            stimer_free_context(ctx);
        }
    }


    describe("Timer reset") {
        struct stimer_ctx * ctx = NULL;
        uint32_t current_time = 0;

        struct stimer * t1 = NULL;
        struct stimer * t2 = NULL;

        it("test objects can be allocated") {
            ctx = stimer_alloc_context(&current_time, mock_get_time, 0xFF, 1000000);
            assert_not_null(ctx);

            t1 = stimer_alloc(ctx);
            assert_not_null(t1);

            t2 = stimer_alloc(ctx);
            assert_not_null(t2);
        }

        it("can advance and restart timers") {
            stimer_expire_from_now_ms(t1, 2);
            stimer_expire_from_now_ms(t2, 2);

            assert_equal(false, stimer_is_expired(t1));
            assert_equal(false, stimer_is_expired(t2));

            current_time += 1;
            assert_equal(false, stimer_is_expired(t1));
            assert_equal(false, stimer_is_expired(t2));

            current_time += 1;
            assert_equal(true, stimer_is_expired(t1));
            assert_equal(true, stimer_is_expired(t2));

            current_time += 1;
            assert_equal(true, stimer_is_expired(t1));
            assert_equal(true, stimer_is_expired(t2));
            stimer_advance(t1);
            stimer_restart_from_now(t2);
            assert_equal(false, stimer_is_expired(t1));
            assert_equal(false, stimer_is_expired(t2));

            current_time += 1;
            assert_equal(true, stimer_is_expired(t1));
            assert_equal(false, stimer_is_expired(t2));

            current_time += 1;
            assert_equal(true, stimer_is_expired(t1));
            assert_equal(true, stimer_is_expired(t2));
        }

        it("test objects can be deallocated") {
            stimer_free(t2);
            stimer_free(t1);
            stimer_free_context(ctx);
        }
    }


    return 0;
}
