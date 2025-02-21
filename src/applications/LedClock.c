#include "applications.h"
#include <rtthread.h>
#include "port_utils.h"
#define PRIO_FIRST (RT_THREAD_PRIORITY_MAX / 3)
#define __CLK_DEBUG
#ifdef __CLK_DEBUG
#define CLK_DEBUG(x) rt_kprintf x
#else
#define CLK_DEBUG(x)
#endif
#define CLOCK_YIELD
#ifdef CLOCK_YIELD
#define CLOCK_DELAY(x) rt_thread_yield()
#else
#define CLOCK_DELAY(x) rt_thread_delay(x)
#endif
#define CLK_POS_0 1
#define CLK_POS_1 100
#define CLK_POS_2 10000
// Modes
#define CLOCK_SEC 0

// TODO implement
// show day time without seconds
#define CLOCK_DAY 1
// show calendar time
#define CALENDAR 2
// set multiple alarms
#define ALARM_CLOCK 3

// counter timer
#define COUNT_UP 4
#define COUNT_DOWN 5

// display mode
static u08_t clock_mode = CLOCK_SEC;

// TODO implement 
// 24h/12h format
#define FULL_DAY 0
#define HALF_DAY 1
static u08_t time_format = FULL_DAY;

typedef struct __LedClock
{
    rt_int16_t year;
    rt_int8_t month;
    rt_int8_t day;
    rt_int8_t hour;
    rt_int8_t min;
    rt_int8_t sec;
    rt_tick_t tick_pre;
    rt_tick_t extra_ticks;
} LedClock;

typedef struct __LedCounter
{
    rt_int8_t hour;
    rt_int8_t min;
    rt_int8_t sec;
    // us refers to 10ms actully
    rt_int8_t us;
    rt_tick_t tick_pre;
    rt_int8_t running;
} LedCounter;

rt_tick_t ticks_per_alarm;
rt_tick_t ticks_next_alarm;
LedCounter sys_count_up_counter;
LedCounter sys_count_down_counter;
static rt_int8_t sys_clock_running = 1;
LedClock sys_led_clock;
const static rt_tick_t ticks_per_sec = RT_TICK_PER_SECOND / 4;
const static rt_tick_t ticks_per_10ms = ticks_per_sec / 100;

ALIGN(RT_ALIGN_SIZE)
static char led_alarm_thread_stack[1024];
static struct rt_thread led_alarm_thread;
static rt_thread_t led_alarm_thread_tid = &led_alarm_thread;
static char sys_clock_control_stack[1024];
static struct rt_thread sys_clock_control_thread;
static rt_thread_t sys_clock_control_tid = &sys_clock_control_thread;
static char display_thread_stack[1024];
static struct rt_thread display_thread;
static rt_thread_t display_thread_tid = &display_thread;
static char count_up_thread_stack[1024];
static struct rt_thread count_up_thread;
static rt_thread_t count_up_thread_tid = &count_up_thread;
static char count_down_thread_stack[1024];
static struct rt_thread count_down_thread;
static rt_thread_t count_down_thread_tid = &count_down_thread;

static rt_int8_t alarming = 0;
static void alarm_entry(void* parameter)
{
    rt_kprintf("ALARM\n");
    for(;;){
        if(!alarming){
            rt_thread_suspend(led_alarm_thread_tid);
            rt_schedule();
        }
        alarm_using_leds();
        // refresh rate would be too high to read
        // when using CLOCK_DELAY
        rt_thread_delay(20);
        rt_thread_yield();
    }
}

void update_counter_down(LedCounter* counter)
{
    if(counter -> min == 0)
    {
        counter -> min += 60;
        counter -> hour --;
    }

    if(counter -> sec == 0)
    {
        counter -> sec += 60;
        counter -> min --;
    }
}

void update_counter_up(LedCounter* counter)
{
    if(counter -> us >= 100)
    {
        counter -> us -= 100;
        counter -> sec ++;
    }

    if(counter -> sec >= 60)
    {
        counter -> sec -= 60;
        counter -> min ++;
    }
}

void update_clock(LedClock* clock)
{
    if(clock -> sec >= 60)
    {
        clock -> sec -= 60;
        clock -> min ++;
    }
    if(clock -> min >= 60)
    {
        clock -> min -= 60;
        clock -> hour ++;
    }
    if(clock -> hour >= 24)
    {
        clock -> hour -= 24;
        // TODO increment of day, month and year counter
    }
}

void update_clock_attr(LedClock* clock, rt_int8_t hour, rt_int8_t min, rt_int8_t sec)
{
    if(hour < 24)
        clock -> hour = hour;
    if(min < 60)
        clock -> min = min;
    if(sec < 60)
        clock -> sec = sec;
    clock -> tick_pre = rt_tick_get();
    clock -> extra_ticks = 0;
}

void init_clock(LedClock* clock)
{
    clock -> year = 2024;
    clock -> month = 5;
    clock -> day = 1;
    clock -> hour = 0;
    clock -> min = 0;
    clock -> sec = 0;
    clock -> tick_pre = rt_tick_get();
    clock -> extra_ticks = 0;
}

static void sys_clock_control(void* args)
{
    rt_kprintf("CLOCK\n");
    init_clock(&sys_led_clock);
    rt_tick_t tick_cur = rt_tick_get();
    rt_tick_t extra_ticks;
    rt_int8_t check_flag = 0;
    for(;;)
    {
        if(!sys_clock_running){
            rt_thread_suspend(sys_clock_control_tid);
            CLK_DEBUG(("Clock control suspend self\n"));
            rt_schedule();
        }
        tick_cur = rt_tick_get();
        //CLK_DEBUG(("tick %d\n", tick_cur));
        check_flag = 60 - sys_led_clock.sec;
        extra_ticks = sys_led_clock.extra_ticks + tick_cur - sys_led_clock.tick_pre;
        while(extra_ticks > ticks_per_sec && check_flag){
            extra_ticks -= ticks_per_sec;
            sys_led_clock.sec ++;
            check_flag --;
        }
        if(!check_flag)
            update_clock(&sys_led_clock);
        sys_led_clock.extra_ticks = extra_ticks;
        sys_led_clock.tick_pre = tick_cur;
        CLOCK_DELAY(ticks_per_sec);
    }
}

static u32_t disp_delay = ticks_per_sec;
static void display_current(void* parameter)
{
    rt_kprintf("DISPLAY\n");
    u32_t to_display_old = 0xFFFFFF;
    u32_t to_display = 0xFFFFFF;
    for(;;)
    {
        switch(clock_mode)
        {
            case CLOCK_SEC:
                // AAHH:MM.SS
                to_display = sys_led_clock.hour;
                if(time_format == HALF_DAY && to_display > 12)
                    to_display -= 12;
                to_display = convert_decimal_to_hex(to_display * CLK_POS_2 
                                                + sys_led_clock.min * CLK_POS_1 
                                                + sys_led_clock.sec * CLK_POS_0);
                to_display += 0xAA000000;
                break;
            case COUNT_UP:
                // BBMM:SS.UU
                to_display = convert_decimal_to_hex(sys_count_up_counter.min * CLK_POS_2
                                                + sys_count_up_counter.sec * CLK_POS_1
                                                + sys_count_up_counter.us * CLK_POS_0);
                to_display += 0xBB000000;
                break;
            case COUNT_DOWN:
                // CCHH:MM.SS
                to_display = convert_decimal_to_hex(sys_count_down_counter.hour * CLK_POS_2
                                                + sys_count_down_counter.min * CLK_POS_1
                                                + sys_count_down_counter.sec * CLK_POS_0);
                to_display += 0xCC000000;
                break;
            default:
                break;
        }
        if(to_display ^ to_display_old)
        {
            to_display_old = to_display;
            //CLK_DEBUG(("disp %x\n", to_display));
            digit_display_num(to_display);
        }
        CLOCK_DELAY(disp_delay);
    }
}

static void count_up_handler(void* args)
{
    rt_kprintf("COUNT UP\n");
    rt_tick_t tick_cur = rt_tick_get();
    int n_us = 0;
    for(;;)
    {
        if(!sys_count_up_counter.running){
            rt_thread_suspend(count_up_thread_tid);
            CLK_DEBUG(("Count up suspend self\n"));
            rt_schedule();
        }
        tick_cur = rt_tick_get();
        n_us = (tick_cur - sys_count_up_counter.tick_pre) / ticks_per_10ms;
        // variable n_us would be 0 which indicates the ticks should
        // be stored for next time
        sys_count_up_counter.tick_pre += n_us * ticks_per_10ms;
        n_us += sys_count_up_counter.us;
        if(n_us > 100){
            for(; n_us >= 100; n_us -= 100){
                sys_count_up_counter.sec ++;
                if(sys_count_up_counter.sec >= 60)
                    update_counter_up(&sys_count_up_counter);
            }
        }
        sys_count_up_counter.us = n_us;
        CLOCK_DELAY(ticks_per_10ms);
    }
}

static void count_down_handler(void* args)
{
    rt_kprintf("COUNT DOWN\n");
    rt_tick_t tick_cur = rt_tick_get();
    int n_s = 0;
    for(;;)
    {
        if(!sys_count_down_counter.running){
            rt_thread_suspend(count_down_thread_tid);
            CLK_DEBUG(("Count down suspend self\n"));
            rt_schedule();
        }
        tick_cur = rt_tick_get();
        n_s = (tick_cur - sys_count_down_counter.tick_pre) / ticks_per_sec;
        sys_count_down_counter.tick_pre += n_s * ticks_per_sec;
        if(sys_count_down_counter.tick_pre < ticks_next_alarm){
            while(n_s >= sys_count_down_counter.sec)
            {
                n_s -= sys_count_down_counter.sec;
                sys_count_down_counter.sec = 0;
                update_counter_down(&sys_count_down_counter);
            }
            sys_count_down_counter.sec -= n_s;
        }else{
            sys_count_down_counter.hour = 0;
            sys_count_down_counter.min = 0;
            sys_count_down_counter.sec = 0;
            sys_count_down_counter.running = 0;
            alarming = 1;
            rt_thread_resume(led_alarm_thread_tid);
        }
        CLOCK_DELAY(ticks_per_sec);
    }
}

#ifdef CLOCK_YIELD
#define PRIO_CLOCK PRIO_FIRST
#else
#define PRIO_CLOCK (PRIO_FIRST - 1)
#endif
void clock_init(void)
{
    sys_count_up_counter.running = 0;
    if(!sys_count_up_counter.running)
    {
        sys_count_up_counter.hour = 0;
        sys_count_up_counter.min = 0;
        sys_count_up_counter.sec = 0;
        sys_count_up_counter.us = 0;
    }

    sys_count_down_counter.running = 0;
    if(!sys_count_down_counter.running)
    {
        sys_count_down_counter.hour = 0;
        sys_count_down_counter.min = 0;
        sys_count_down_counter.sec = 0;
        sys_count_down_counter.us = 0;
    }

    rt_thread_init(led_alarm_thread_tid,
                   "alarm",
                   alarm_entry,
                   RT_NULL,
                   led_alarm_thread_stack,
                   sizeof(led_alarm_thread_stack),
                   PRIO_CLOCK, 5);
    rt_thread_startup(led_alarm_thread_tid);

    rt_thread_init(sys_clock_control_tid,
                    "sys_clk",
                    sys_clock_control,
                    RT_NULL,
                    sys_clock_control_stack,
                    sizeof(sys_clock_control_stack),
                    PRIO_CLOCK, 50);
    rt_thread_startup(sys_clock_control_tid);

    rt_thread_init(display_thread_tid,
                    "disp_t",
                    display_current,
                    RT_NULL,
                    display_thread_stack,
                    sizeof(display_thread_stack),
                    PRIO_CLOCK, 50);
    rt_thread_startup(display_thread_tid);

    rt_thread_init(count_up_thread_tid,
                    "ct_up_t",
                    count_up_handler,
                    RT_NULL,
                    count_up_thread_stack,
                    sizeof(count_up_thread_stack),
                    PRIO_CLOCK, 50);
    rt_thread_startup(count_up_thread_tid);

    rt_thread_init(count_down_thread_tid,
                    "ct_dn_t",
                    count_down_handler,
                    RT_NULL,
                    count_down_thread_stack,
                    sizeof(count_down_thread_stack),
                    PRIO_CLOCK, 50);
    rt_thread_startup(count_down_thread_tid);
    CLK_DEBUG(("INIT END\n"));
}

static rt_int8_t* modifying_attr = NULL;

void handle_ok(void)
{
    switch (clock_mode)
    {
    case CLOCK_SEC:
        if(modifying_attr != NULL)
        {
            if(modifying_attr == &(sys_led_clock.sec))
            {
                modifying_attr = NULL;
            }
            else if(modifying_attr == &(sys_led_clock.hour))
            {
                modifying_attr = &(sys_led_clock.min);
            }else if(modifying_attr == &(sys_led_clock.min))
            {
                modifying_attr = &(sys_led_clock.sec);
            }
        } else {
            if(sys_clock_running){
                sys_clock_running = 0;
            }else{
                update_clock_attr(&sys_led_clock, 100, 100, 100);
                sys_clock_running = 1;
                rt_thread_resume(sys_clock_control_tid);
            }
        }
        break; 
    case COUNT_UP:
        if(sys_count_up_counter.running){
            sys_count_up_counter.running = 0;
        }else{
            sys_count_up_counter.running = 1;
            sys_count_up_counter.tick_pre = rt_tick_get();
            rt_thread_resume(count_up_thread_tid);
        }
        break;
    case COUNT_DOWN:
        if(modifying_attr != NULL)
        {
            if(modifying_attr == &(sys_count_down_counter.sec)){
                modifying_attr = NULL;
            }else if(modifying_attr == &(sys_count_down_counter.hour)){
                modifying_attr = &(sys_count_down_counter.min);
            }else if(modifying_attr == &(sys_count_down_counter.min)){
                modifying_attr = &(sys_count_down_counter.sec);
            }
        }else{
            if(sys_count_down_counter.running){
                sys_count_down_counter.running = 0;
            }else{
                sys_count_down_counter.running = 1;
                sys_count_down_counter.tick_pre = rt_tick_get();
                ticks_next_alarm = sys_count_down_counter.hour * 3600 
                                    + sys_count_down_counter.min * 60 
                                    + sys_count_down_counter.sec; 
                ticks_next_alarm *= ticks_per_sec;
                ticks_next_alarm += sys_count_down_counter.tick_pre;
                rt_thread_resume(count_down_thread_tid);
            }
        }
        break;
    default:
        break;
    }
}

void handle_setting(void)
{
    /** 
     * Enter setting mode to set a specific time to the main clock and the count down
     * counter. The time is set from the hour field to the second field. If the input
     * is ok, the active field will be set to the next field.
     */
    switch (clock_mode)
    {
    case CLOCK_SEC:
        sys_clock_running = 0;
        modifying_attr = &(sys_led_clock.hour);
        break;
    case COUNT_DOWN:
        if(!sys_count_down_counter.running)
            modifying_attr = &(sys_count_down_counter.hour);
        break;
    default:
        break;
    }
}

#define CLOCK_CTL_OK 15
#define CLOCK_CTL_SETTING 14
#define CLOCK_CTL_UP 13
#define CLOCK_CTL_DOWN 12
void clock_demo(u32_t* ctl)
{
    parse_control(ctl);
    CLK_DEBUG(("CTL VAL %d\n", *ctl));
    switch (*ctl)
    {
    case CLOCK_CTL_OK:
    // exit the setting mode of current clock mode
        handle_ok();
        break;
    case CLOCK_CTL_SETTING:
    // enter the setting mode of current clock mode
        handle_setting();
        break;
    case CLOCK_CTL_UP:
    // increase the active field
        if(modifying_attr != NULL)
            (*modifying_attr) ++;
        break;
    case CLOCK_CTL_DOWN:
    // decrease the active field
        if(modifying_attr != NULL)
            (*modifying_attr) --;
        break;
    case COUNT_DOWN:
    // switch to the count down counter
    // ok: start/suspend/resume the timer
    // setting: enter the setting mode
        clock_mode = COUNT_DOWN;
        disp_delay = ticks_per_sec;
        if(!sys_count_down_counter.running)
        {
            sys_count_down_counter.hour = 0;
            sys_count_down_counter.min = 1;
            sys_count_down_counter.sec = 0;
            sys_count_down_counter.us = 0;
        }
        break;
    case COUNT_UP:
    // switch to the count up counter
    // ok: start/suspend/resume the stopwatch
        clock_mode = COUNT_UP;
        disp_delay = ticks_per_10ms;
        if(!sys_count_up_counter.running)
        {
            sys_count_up_counter.hour = 0;
            sys_count_up_counter.min = 0;
            sys_count_up_counter.sec = 0;
            sys_count_up_counter.us = 0;
        }
        break;
    case CLOCK_SEC:
    // switch to the system clock
    // ok: suspend/resume the clock
    // setting: enter the setting mode
        clock_mode = CLOCK_SEC;
        disp_delay = ticks_per_sec;
        break;
    default:
        break;
    }
}

void clock_main(void)
{
    init_position_map();
    clock_init();
    u32_t sw_pre = get_sw_16();
    u32_t sw_cur;
    u32_t sw_ctl;
    CLK_DEBUG(("ENTER MAIN LOOP\n"));
    for(;;)
    {
        sw_cur = get_sw_16();
        sw_ctl = sw_pre ^ sw_cur;
        if(sw_ctl)
        {
            sw_pre = sw_cur;
            CLK_DEBUG(("Switch ctrl %x\n", sw_ctl));
            if(alarming)
                alarming = 0;
            clock_demo(&sw_ctl);
        }
        rt_thread_yield();
    }
}
