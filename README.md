# Embedded C Software Timer Library
Software timer library for embedded (bare metal) projects. This provides the ability to create arbitrary numbers of software timers to track elapsed time, or to create expiration timers. It has no direct hardware dependencies, so it is portable to many platforms.

## Installation
This library can be installed into your project using [clib](https://github.com/clibs/clib):

```
$ clib install bradschl/embedded-c-soft-timer
```

## Example
```C
#include <project.h>
#include "stimer/stimer.h"

static uint32_t
get_current_time(void * hint)
{
    (void) hint;

    // Timer is 1us resolution. The hardware counts down from the maximum
    // value, so the raw value needs to be flipped to be compatible with the
    // software timer
    return Timer_1_INIT_PERIOD - Timer_1_ReadCounter();
}


int
main()
{
    // Create the timer context, 1000000ns per Timer_1 tick, maximum value
    // returned by get_current_timer is Timer_1_INIT_PERIOD
    struct stimer_ctx * timer_ctx = 
        stimer_alloc_context(NULL, 
                             get_current_time,
                             Timer_1_INIT_PERIOD,
                             1000000);
    ASSERT_NOT_NULL(timer_ctx);


    // Allocate a timer handle
    struct stimer * my_timer = stimer_alloc(timer_ctx);
    ASSERT_NOT_NULL(my_timer);

    // Set the timer to expire in 100ms from now
    stimer_expire_from_now_ms(my_timer, 100);


    for(;;) {
        // Periodically execute the timer context to drive the
        // internal timer rollover logic
        stimer_execute_context(timer_ctx);

        /* ...  */

        // Check if the timer expired, do stuff with it
        if(stimer_is_expired(my_timer)) {
            stimer_advance(my_timer);
            /* ...  */
        }
    }
}
```

## API
See [stimer.h](src/stimer/stimer.h) for the C API.

## Dependencies and Resources
This library uses heap when allocating structures. After initialization, additional allocations will not be made. This should be fine for an embedded target, since memory fragmentation only happens if memory is freed.

Compiled, this library is only a few kilobytes. Runtime memory footprint is very small, and is dependent on the number of timers allocated.

## License
MIT license for all files.
