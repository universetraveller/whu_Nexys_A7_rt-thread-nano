#include "psp_api.h"

#define SegEn_ADDR      0x80001038
#define SegDig_ADDR     0x8000103C

#define GPIO_SWs        0x80001400
#define GPIO_LEDs       0x80001404
#define GPIO_INOUT      0x80001408
#define RGPIO_INTE      0x8000140C
#define RGPIO_PTRIG     0x80001410
#define RGPIO_CTRL      0x80001418
#define RGPIO_INTS      0x8000141C

#define RPTC_CNTR       0x80001200
#define RPTC_HRC        0x80001204
#define RPTC_LRC        0x80001208
#define RPTC_CTRL       0x8000120c

#define Select_INT      0x80001018

#define CYCLES_PER_MS D_CLOCK_RATE/1000
#define TIMER_PERIOD_INIT_LOAD 1

#define NAME_FUNC_ADD_RET(f, v) f##_ret_##v
#define FUNC_ADD_RET(f, type, v) type NAME_FUNC_ADD_RET(f, v)(void) {f(); return v;}

#define STRINGFY(x) #x
#define STRINGFY_MACRO(x) STRINGFY(x)

#define PSP_HANDLER_WRAPPER_NAME(f) f##_psp_handler
#define PSP_HANDLER_WRAPPER(f) void PSP_HANDLER_WRAPPER_NAME(f)(void) {rt_interrupt_enter(); f(); rt_interrupt_leave(); rt_hw_interrupt_handled();}

extern void hw_tick_handler(void);
void notify_ecall(void);
#define PORT_USED_VECT_TABLE M_PSP_VECT_TABLE
#define PORT_TICK_INCREASE hw_tick_handler
#define PORT_ECALL_HANDLER notify_ecall
#define PORT_INT_REG_FUNC pspRegisterInterruptHandler
#define PORT_EXC_REG_FUNC pspRegisterExceptionHandler

void init_uart(void);

void hw_print_char(char);

void init_interrupts(void);

void enable_interrupts(void);

void disable_hw_timer_interrupts(void);

void enable_hw_timer_interrupts(void);

void init_timer_period(void);

void set_timer_period(u32_t);

u32_t get_timer_period_current(void);

void timer_setup(void);

void timer_setup_init(void);

void timer_interrupt_handler(void);

void port_register_interrupt_handler(pspInterruptHandler_t handler, u32_t cause);

void port_register_exception_handler(pspInterruptHandler_t handler, u32_t cause);

void register_timer_handler();

void delay(int*, double);

void notify_timer_hits(void);

void start_bsp_timer(void);

void notify_timer_hits_and_loop(void);

void init_bsp_timer(int duration_ms, pspNmiHandler_t callback);

void init_psp_timer(int cycles, pspInterruptHandler_t callback);

void digit_display_num(u32_t to_display);

void led_display_16(u16_t fmt);

u32_t psp_read_register(u32_t addr);

u32_t get_sw_32();

u32_t get_sw_16();

u32_t get_leds_16();

void __stall_to_next_switches_change(unsigned int* sw);

void __debug_external_hardwares();