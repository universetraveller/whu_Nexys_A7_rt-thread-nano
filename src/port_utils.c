#include "port_utils.h"
#include "psp_api.h"
#include "bsp_external_interrupts.h"
#include "bsp_timer.h"
#include "bsp_printf.h"
#include "bsp_mem_map.h"
#include "bsp_version.h"


extern D_PSP_DATA_SECTION D_PSP_ALIGNED(1024) pspInterruptHandler_t G_Ext_Interrupt_Handlers[8];


void DefaultInitialization(void)
{
  u32_t uiSourceId;

  /* Register interrupt vector */
  pspInterruptsSetVectorTableAddress(&PORT_USED_VECT_TABLE);

  /* Set external-interrupts vector-table address in MEIVT CSR */
  pspExternalInterruptSetVectorTableAddress(G_Ext_Interrupt_Handlers);

  /* Put the Generation-Register in its initial state (no external interrupts are generated) */
  bspInitializeGenerationRegister(D_PSP_EXT_INT_ACTIVE_HIGH);

  for (uiSourceId = D_BSP_FIRST_IRQ_NUM; uiSourceId <= D_BSP_LAST_IRQ_NUM; uiSourceId++)
  {
    /* Make sure the external-interrupt triggers are cleared */
    bspClearExtInterrupt(uiSourceId);
  }

  /* Set Standard priority order */
  pspExtInterruptSetPriorityOrder(D_PSP_EXT_INT_STANDARD_PRIORITY);

  /* Set interrupts threshold to minimal (== all interrupts should be served) */
  pspExtInterruptsSetThreshold(M_PSP_EXT_INT_THRESHOLD_UNMASK_ALL_VALUE);

  /* Set the nesting priority threshold to minimal (== all interrupts should be served) */
  pspExtInterruptsSetNestingPriorityThreshold(M_PSP_EXT_INT_THRESHOLD_UNMASK_ALL_VALUE);
}


void ExternalIntLine_Initialization(u32_t uiSourceId, u32_t priority, pspInterruptHandler_t pTestIsr)
{
  /* Set Gateway Interrupt type (Level) */
  pspExtInterruptSetType(uiSourceId, D_PSP_EXT_INT_LEVEL_TRIG_TYPE);

  /* Set gateway Polarity (Active high) */
  pspExtInterruptSetPolarity(uiSourceId, D_PSP_EXT_INT_ACTIVE_HIGH);

  /* Clear the gateway */
  pspExtInterruptClearPendingInt(uiSourceId);

  /* Set IRQ4 priority */
  pspExtInterruptSetPriority(uiSourceId, priority);
    
  /* Enable IRQ4 interrupts in the PIC */
  pspExternalInterruptEnableNumber(uiSourceId);

  /* Register ISR */
  G_Ext_Interrupt_Handlers[uiSourceId] = pTestIsr;
}


void GPIO_Initialization(void)
{
  /* Configure LEDs and Switches */
  M_PSP_WRITE_REGISTER_32(GPIO_INOUT, 0xFFFF);        /* GPIO_INOUT */
  M_PSP_WRITE_REGISTER_32(GPIO_LEDs, 0x0);            /* GPIO_LEDs */

  /* Configure GPIO interrupts */
  M_PSP_WRITE_REGISTER_32(RGPIO_INTE, 0x10000);       /* RGPIO_INTE */
  M_PSP_WRITE_REGISTER_32(RGPIO_PTRIG, 0x10000);      /* RGPIO_PTRIG */
  M_PSP_WRITE_REGISTER_32(RGPIO_INTS, 0x0);           /* RGPIO_INTS */
  M_PSP_WRITE_REGISTER_32(RGPIO_CTRL, 0x1);           /* RGPIO_CTRL */
}


void delay(int * i, double times) {
  if(i == NULL){
      int j;
      delay(&j, times);
      return;
  }
  for((*i)=0;(*i)<CYCLES_PER_MS * times;(*i)++);
}

void init_uart(void)
{
  swervolfVersion_t stSwervolfVersion;

  versionGetSwervolfVer(&stSwervolfVersion);

 /* Whisper bypass - force UART state to be "non-busy" (== 0) so print via UART will be displayed on console
  * when running with Whisper */
  u32_t* pUartState = (u32_t*)(D_UART_BASE_ADDRESS+0x8);
  *pUartState = 0 ;

  /* init uart */
  uartInit();


  printfNexys("------------------------------------------");
  printfNexys("SweRVolf version %d.%d%d (SHA %08x) (dirty %d)",
                   stSwervolfVersion.ucMajor,
                   stSwervolfVersion.ucMinor,
                   stSwervolfVersion.ucRev,
                   stSwervolfVersion.ucSha,
                   stSwervolfVersion.ucDirty);
  printfNexys("------------------------------------------");
}

#define M_UART_RD_REG_LSR()  (*((volatile unsigned int*)(D_UART_BASE_ADDRESS + (4*0x05) )))
#define D_UART_LSR_THRE_BIT    (0x20)
#define M_UART_WR_CH(_CHAR_) (*((volatile unsigned int*)(D_UART_BASE_ADDRESS + (0x00) )) = _CHAR_)
void hw_print_char(char ch)
{
    if (ch == '\n')
        hw_print_char('\r');

    /* Check for space in UART FIFO */
    while((M_UART_RD_REG_LSR() & D_UART_LSR_THRE_BIT) == 0);

    //write char
    M_UART_WR_CH(ch);
}

void init_interrupts(void)
{
  /* INITIALIZE THE INTERRUPT SYSTEM */
  DefaultInitialization();                            /* Default initialization */
  pspExtInterruptsSetThreshold(5);                    /* Set interrupts threshold to 5 */

  /* INITIALIZE INTERRUPT LINE IRQ4 */
  //ExternalIntLine_Initialization(4, 6, GPIO_ISR);     /* Initialize line IRQ4 with a priority of 6. Set GPIO_ISR as the Interrupt Service Routine */
  //M_PSP_WRITE_REGISTER_32(Select_INT, 0x1);           /* Connect the GPIO interrupt to the IRQ4 interrupt line */

  /* INITIALIZE THE PERIPHERALS */
  GPIO_Initialization();                              /* Initialize the GPIO */
  M_PSP_WRITE_REGISTER_32(SegEn_ADDR, 0x0);           /* Initialize the 7-Seg Displays */
}

void enable_interrupts(void)
{
    /* ENABLE INTERRUPTS */
    pspInterruptsEnable();                              /* Enable all interrupts in mstatus CSR */
    M_PSP_SET_CSR(D_PSP_MIE_NUM, D_PSP_MIE_MEIE_MASK);  /* Enable external interrupts in mie CSR */
}

void disable_hw_timer_interrupts(void)
{
    pspDisableInterruptNumberMachineLevel(D_PSP_INTERRUPTS_MACHINE_TIMER);
}

void enable_hw_timer_interrupts(void)
{
    pspEnableInterruptNumberMachineLevel(D_PSP_INTERRUPTS_MACHINE_TIMER);
}

u32_t _timer_period;
void set_timer_period(u32_t period)
{
    if(period == 0)
    {
        init_timer_period();
        return;
    }
    _timer_period = period;
}

void init_timer_period(void)
{
    set_timer_period((D_CLOCK_RATE * D_TICK_TIME_MS / D_PSP_MSEC));
}

u32_t get_timer_period_current(void)
{
    return _timer_period;
}

void timer_setup(void)
{
    //printfNexys("timer_setup");
    /* Enable timer interrupt */
    pspEnableInterruptNumberMachineLevel(D_PSP_INTERRUPTS_MACHINE_TIMER);

    /* Activates Core's timer with the calculated period */
    pspTimerCounterSetupAndRun(E_MACHINE_TIMER, _timer_period);
    //printfNexys("timer_setup end");
}

u08_t __RT_TIMER_SHOULD_INIT = 1;
void timer_setup_init(void)
{
    if(!__RT_TIMER_SHOULD_INIT)
        return;
    printfNexys("Init timer with tick: %d", _timer_period);
    _timer_period *= TIMER_PERIOD_INIT_LOAD;
    timer_setup();
    _timer_period /= TIMER_PERIOD_INIT_LOAD;
    __RT_TIMER_SHOULD_INIT = 0;
}

void timer_interrupt_handler(void)
{
    /* Disable Machine-Timer interrupt */
    pspDisableInterruptNumberMachineLevel(D_PSP_INTERRUPTS_MACHINE_TIMER);

    /* Increment the RTOS tick. */
    PORT_TICK_INCREASE();

    /* Setup the Timer for next round */
    timer_setup();
}

void notify_ecall(void)
{
    printfNexys("ECALL exception occurs");
}

void port_register_exception_handler(pspInterruptHandler_t handler, u32_t cause)
{
    PORT_EXC_REG_FUNC(handler, cause);
}

void port_register_interrupt_handler(pspInterruptHandler_t handler, u32_t cause)
{
    PORT_INT_REG_FUNC(handler, cause);
}

void register_timer_handler()
{
    // Ecall is unnecessary for context switch on this cpu
    port_register_exception_handler(PORT_ECALL_HANDLER, E_EXC_ENVIRONMENT_CALL_FROM_MMODE);
    /* register timer interrupt handler */
    port_register_interrupt_handler(timer_interrupt_handler, E_MACHINE_TIMER_CAUSE);
}

void notify_timer_hits(void)
{
    pspDisableInterruptNumberMachineLevel(D_PSP_INTERRUPTS_MACHINE_TIMER);
    printfNexys("TIMER COUNT ENDS");
    pspEnableInterruptNumberMachineLevel(D_PSP_INTERRUPTS_MACHINE_TIMER);
}

void start_bsp_timer(void)
{
    bspStartTimer();
}

void notify_timer_hits_and_loop(void)
{
    notify_timer_hits();
    start_bsp_timer();
}

void init_bsp_timer(int duration_ms, pspNmiHandler_t callback)
{
    /* Register the initial NMI handler in nmi_vec register */
    pspNmiSetVec(D_NMI_VEC_ADDRESSS, pspNmiHandlerSelector);

    /* Register External-Pin-Asserted NMI handler function */
    pspNmiRegisterHandler(callback, D_PSP_NMI_EXT_PIN_ASSERTION);

    /* Rout timer to NMI pin assertion - i.e. when the timer expires, an NMI will be asserted */
    bspRoutTimer(E_TIMER_TO_NMI);
    
    /* Initialize Timer (at its expiration, it will create an NMI) */
    bspSetTimerDurationMsec(duration_ms);
}

void init_psp_timer(int cycles, pspInterruptHandler_t callback)
{
    pspRegisterInterruptHandler(callback, E_MACHINE_TIMER_CAUSE);
    pspEnableInterruptNumberMachineLevel(D_PSP_INTERRUPTS_MACHINE_TIMER);
    pspTimerCounterSetupAndRun(E_MACHINE_TIMER, cycles);
}

void digit_display_num(u32_t to_display)
{
    M_PSP_WRITE_REGISTER_32(SegDig_ADDR, to_display);
}

void led_display_16(u16_t fmt)
{
    M_PSP_WRITE_REGISTER_32(GPIO_LEDs, fmt);             
}

u32_t psp_read_register(u32_t addr)
{
    return M_PSP_READ_REGISTER_32(addr);
}

u32_t get_sw_32()
{
    return psp_read_register(GPIO_SWs);
}

u32_t get_sw_16()
{
    return psp_read_register(GPIO_SWs) >> 0x10;
}

u32_t get_leds_16()
{
    return psp_read_register(GPIO_LEDs);
}
#define SWITCHES_MASK 0xFFFF0000

void __stall_to_next_switches_change(unsigned int* sw)
{
    if(sw == NULL)
    {
        unsigned int sw_n = M_PSP_READ_REGISTER_32(GPIO_SWs) & SWITCHES_MASK;
        __stall_to_next_switches_change(&sw_n);
        return;
    }
    unsigned int _sw;
    int i;
    unsigned int sw_o = (*sw) & SWITCHES_MASK;
    for(;;)
    {
        _sw = M_PSP_READ_REGISTER_32(GPIO_SWs) & SWITCHES_MASK;
        if(_sw ^ sw_o)
        {
            *sw = _sw;
            break;
        }
        delay(&i, 1);
        continue;
    }
}

void __debug_external_hardwares(void)
{
    unsigned int sw;
    printfNexys("---\nTrap into __debug_external_hardwares");
    sw = M_PSP_READ_REGISTER_32(GPIO_SWs);
    u16_t count = 0x8000;

    printfNexys("DEBUG digit_display_num(0x%x)", count);
    digit_display_num(count);
    __stall_to_next_switches_change(&sw);

    printfNexys("DEBUG led_display_16(0x%x)", count);
    led_display_16(count);
    __stall_to_next_switches_change(&sw);

    printfNexys("DEBUG get_leds_16()");
    printfNexys("get_leds_16() is 0x%x", get_leds_16());
    __stall_to_next_switches_change(&sw);

    count = count << 1;
    printfNexys("count << 1 is 0x%x", count);

    printfNexys("DEBUG digit_display_num(0x%x)", count);
    digit_display_num(count);
    __stall_to_next_switches_change(&sw);

    printfNexys("DEBUG led_display_16(0x%x)", count);
    led_display_16(count);
    __stall_to_next_switches_change(&sw);

    printfNexys("DEBUG get_leds_16()");
    printfNexys("get_leds_16() is 0x%x", get_leds_16());
    __stall_to_next_switches_change(&sw);

    count = 0x123;

    printfNexys("DEBUG digit_display_num(0x%x)", count);
    digit_display_num(count);
    __stall_to_next_switches_change(&sw);

    printfNexys("DEBUG led_display_16(0x%x)", count);
    led_display_16(count);
    __stall_to_next_switches_change(&sw);

    printfNexys("DEBUG get_leds_16()");
    printfNexys("get_leds_16() is 0x%x", get_leds_16());
    __stall_to_next_switches_change(&sw);

    printfNexys("DEBUG get_sw_[32,16]()");
    for(count = 0; count < 4; ++ count)
    {
        printfNexys("Change switches: %d/4", count);
        __stall_to_next_switches_change(&sw);
        delay(NULL, 1);
    }
    printfNexys("get_sw_16() is 0x%x\nget_sw_32() is 0x%x", get_sw_16(), get_sw_32());
    printfNexys("CHANGE SWITCHES TO EXIT");
    __stall_to_next_switches_change(&sw);
}
