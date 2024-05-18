#include <rtthread.h>
#include "port_utils.h"
extern int thread_sample(void);
int main(void)
{
#ifdef _MAIN_DBG_MACRO
    __debug_external_hardwares();
#endif
    thread_sample();
    u16_t __leds = 0x8000;
    for(;;)
    {
        __leds = __leds == 0x8000 ? 0x1 : __leds << 1;
        rt_kprintf("---");
        led_display_16(__leds);
        rt_thread_delay(10);
        digit_display_num(__leds);
        rt_thread_delay(10);
    }
}