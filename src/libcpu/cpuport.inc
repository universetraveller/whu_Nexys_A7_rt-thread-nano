#include "cpuport.h"
.macro M_RT_PUSH_REGFILE
    addi sp, sp, -32 * REGBYTES

    STORE x1,   1 * REGBYTES(sp)

    STORE x4,   4 * REGBYTES(sp)
    STORE x5,   5 * REGBYTES(sp)
    STORE x6,   6 * REGBYTES(sp)
    STORE x7,   7 * REGBYTES(sp)
    STORE x8,   8 * REGBYTES(sp)
    STORE x9,   9 * REGBYTES(sp)
    STORE x10, 10 * REGBYTES(sp)
    STORE x11, 11 * REGBYTES(sp)
    STORE x12, 12 * REGBYTES(sp)
    STORE x13, 13 * REGBYTES(sp)
    STORE x14, 14 * REGBYTES(sp)
    STORE x15, 15 * REGBYTES(sp)
    STORE x16, 16 * REGBYTES(sp)
    STORE x17, 17 * REGBYTES(sp)
    STORE x18, 18 * REGBYTES(sp)
    STORE x19, 19 * REGBYTES(sp)
    STORE x20, 20 * REGBYTES(sp)
    STORE x21, 21 * REGBYTES(sp)
    STORE x22, 22 * REGBYTES(sp)
    STORE x23, 23 * REGBYTES(sp)
    STORE x24, 24 * REGBYTES(sp)
    STORE x25, 25 * REGBYTES(sp)
    STORE x26, 26 * REGBYTES(sp)
    STORE x27, 27 * REGBYTES(sp)
    STORE x28, 28 * REGBYTES(sp)
    STORE x29, 29 * REGBYTES(sp)
    STORE x30, 30 * REGBYTES(sp)
    STORE x31, 31 * REGBYTES(sp)
.endm

.macro M_RT_LOAD_MEPC
    /* resw ra to mepc */
    LOAD a0,   0 * REGBYTES(sp)
    csrw mepc, a0
.endm

.macro M_RT_INIT_MSTATUS
    li    t0, 0x00001800
    csrs  mstatus, t0
.endm

.macro M_RT_LOAD_MSTATUS
    LOAD a0,   2 * REGBYTES(sp)
    csrs mstatus, a0
.endm

.macro M_RT_INIT_AND_LOAD_MSTATUS
    M_RT_INIT_MSTATUS
    M_RT_LOAD_MSTATUS
.endm

.macro M_RT_POP_REGFILE
    LOAD x1,   1 * REGBYTES(sp)
    LOAD x4,   4 * REGBYTES(sp)
    LOAD x5,   5 * REGBYTES(sp)
    LOAD x6,   6 * REGBYTES(sp)
    LOAD x7,   7 * REGBYTES(sp)
    LOAD x8,   8 * REGBYTES(sp)
    LOAD x9,   9 * REGBYTES(sp)
    LOAD x10, 10 * REGBYTES(sp)
    LOAD x11, 11 * REGBYTES(sp)
    LOAD x12, 12 * REGBYTES(sp)
    LOAD x13, 13 * REGBYTES(sp)
    LOAD x14, 14 * REGBYTES(sp)
    LOAD x15, 15 * REGBYTES(sp)
    LOAD x16, 16 * REGBYTES(sp)
    LOAD x17, 17 * REGBYTES(sp)
    LOAD x18, 18 * REGBYTES(sp)
    LOAD x19, 19 * REGBYTES(sp)
    LOAD x20, 20 * REGBYTES(sp)
    LOAD x21, 21 * REGBYTES(sp)
    LOAD x22, 22 * REGBYTES(sp)
    LOAD x23, 23 * REGBYTES(sp)
    LOAD x24, 24 * REGBYTES(sp)
    LOAD x25, 25 * REGBYTES(sp)
    LOAD x26, 26 * REGBYTES(sp)
    LOAD x27, 27 * REGBYTES(sp)
    LOAD x28, 28 * REGBYTES(sp)
    LOAD x29, 29 * REGBYTES(sp)
    LOAD x30, 30 * REGBYTES(sp)
    LOAD x31, 31 * REGBYTES(sp)

    addi sp,  sp, 32 * REGBYTES
.endm

.macro M_RT_POP_REGFILE_AND_STATUS
    M_RT_LOAD_MEPC
    M_RT_INIT_AND_LOAD_MSTATUS
    M_RT_POP_REGFILE
.endm

/* this macro save mstatus and mepc CSRs on stack then store sp in the Application control block */
.macro M_RT_SAVE_CONTEXT  pAppCB, spLocationInAppCB
    /* Store current stackpointer in task control block (TCB) */
    LOAD    t0, \pAppCB
    STORE   sp, \spLocationInAppCB(t0)
    /* Store mepc */
    csrr          t0, mepc
    STORE   t0, 0(sp)
.endm

.macro M_RT_BEFORE_HANDLE enter_interrupt
    move s0, sp
    M_PSP_CHANGE_SP_FROM_APP_TO_ISR_STACK
    call \enter_interrupt
.endm

.macro M_RT_AFTER_HANDLE exit_interrupt
    call \exit_interrupt
    move sp, s0
.endm

.macro M_RT_HANDLE_INTERRUPT f_enter, f_handler, f_exit
    M_RT_BEFORE_HANDLE \f_enter
    M_PSP_CALL_INT_HANDLER \handler
    M_RT_AFTER_HANDLE  \f_exit
.endm

/* this macro restore sp from the Application control block then restore mstatus and mepc CSRs from stack */
.macro M_RT_RESTORE_CONTEXT  pAppCB, spLocationInAppCB
    /* Load stack pointer from the current TCB */
    LOAD    sp, \pAppCB
    LOAD    sp, \spLocationInAppCB(sp)
    /* Load task program counter */
    LOAD    t0, 0(sp)
    csrw          mepc, t0
.endm

/* This macro is activated at the end of ISR handling. It does the following:
 * (1) checks whether a context-switch is required.
 * (2) If no - jump to 'branch_label'
 * (2) if yes - then
 *    (a) it clears the context-switch indication and
 *    (b) it calls M_RTOSAL_SWITCH_CONTEXT (macro that calls OS function to do context-switch)
 */
.macro M_RT_CHECK_CONTEXT_SWITCH branch_label
    /* save address of g_rtosalContextSwitch -> a0 */
    la            a0, rt_thread_switch_interrupt_flag
    /* load the value g_rtosalContextSwitch -> a1 */
    LOAD    a1, 0x0(a0)
    /* check if g_rtosalContextSwitch is set - need to do context switch */
    beqz          a1, \branch_label
    /* clear g_rtosalContextSwitch */
    /* TODO: if bitmanip exist add bit set */
    STORE   zero, 0x0(a0)
    /* call OS to perform context switch */
    M_RT_SWITCH_CONTEXT
.endm

.macro M_RT_SWITCH_CONTEXT
    csrr  a0, mepc
    STORE a0, 0 * REGBYTES(sp)

    la    s0, rt_interrupt_from_thread
    LOAD  s1, 0(s0)
    STORE sp, 0(s1)

    la    s0, rt_interrupt_to_thread
    LOAD  s1, 0(s0)
    LOAD  sp, 0(s1)

    LOAD  a0,  0 * REGBYTES(sp)
    csrw  mepc, a0
.endm
