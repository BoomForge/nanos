#include <kernel/platform.h>
#include <kernel/time.h>
#include <kernel/types.h>

static uint32_t time_ticks_per_second;

void kernel_time_init(uint32_t ticks_per_second)
{
    if (ticks_per_second == 0u) {
        ticks_per_second = 100u;
    }

    time_ticks_per_second = ticks_per_second;
    platform_timer_init(ticks_per_second);
}

uint32_t kernel_time_ticks(void)
{
    return platform_timer_ticks();
}

uint32_t kernel_time_ticks_per_second(void)
{
    if (time_ticks_per_second == 0u) {
        return 1u;
    }

    return time_ticks_per_second;
}

uint32_t kernel_time_seconds(void)
{
    return kernel_time_ticks() / kernel_time_ticks_per_second();
}
