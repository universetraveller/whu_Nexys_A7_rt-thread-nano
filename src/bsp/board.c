/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-07-24     Tanek        the first version
 * 2018-11-12     Ernest Chen  modify copyright
 */
 
#include <stdint.h>
#include <rthw.h>
#include <rtthread.h>
#include "port_utils.h"

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
#define RT_HEAP_SIZE 1024
static uint32_t rt_heap[RT_HEAP_SIZE];     // heap default size: 4K(1024 * 4)
RT_WEAK void *rt_heap_begin_get(void)
{
    return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
    return rt_heap + RT_HEAP_SIZE;
}
#endif

FUNC_ADD_RET(init_uart, int, 0)
FUNC_ADD_RET(init_interrupts, int, 0)
FUNC_ADD_RET(register_timer_handler, int, 0)
FUNC_ADD_RET(disable_hw_timer_interrupts, int, 0)
FUNC_ADD_RET(init_timer_period, int, 0)
FUNC_ADD_RET(timer_setup_init, int, 0)

// Behaviour of INIT_BOARD_EXPORT macros is equivalent to the following function
// The registered functions seem to be called in reversed order
//  void init_directly(void)
//  {
//      init_uart();
//      init_interrupts();
//      register_timer_handler();
//      disable_hw_timer_interrupts();
//      init_timer_period();
//  }
INIT_BOARD_EXPORT(NAME_FUNC_ADD_RET(init_timer_period, 0));
INIT_BOARD_EXPORT(NAME_FUNC_ADD_RET(disable_hw_timer_interrupts, 0));
INIT_BOARD_EXPORT(NAME_FUNC_ADD_RET(register_timer_handler, 0));
INIT_BOARD_EXPORT(NAME_FUNC_ADD_RET(init_interrupts, 0));
INIT_BOARD_EXPORT(NAME_FUNC_ADD_RET(init_uart, 0));
INIT_DEVICE_EXPORT(NAME_FUNC_ADD_RET(timer_setup_init, 0));

/**
 * This function will initial your board.
 */
void rt_hw_board_init()
{
    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif
}

void hw_tick_handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

void rt_hw_console_output(const char *str)
{
    char* ctrl = (char*)str;
    for ( ; *ctrl; ctrl++) 
      hw_print_char(*ctrl);
}