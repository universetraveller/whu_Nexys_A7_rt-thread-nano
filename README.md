# whu_Nexys_A7_rt-thread-nano
Porting rt-thread-nano on Nexys A7 for whu embedded system experiments  

能跑就行版武汉大学嵌入式实验移植RT-Thread（nano）到Nexys A7

恭喜嵌入式实验移植RT-Thread荣登本科前三年最令人无语实验课（嵌入式和软件工程也是奇葩课程前三名了），参考资料是一点没有的，搜索引擎是搜不出来的，通宵是要天天通的

试一次代码上传5分钟，一点教程没有全靠乱试，玄学bug一个接一个（为什么一段汇编复制一次或者写成宏就失效），吃掉无数干正事的睡觉，你这实验咋这么牛呢我请问

不希望之后有人再被这实验浪费时间了

## Introduction
实现了硬件时钟和中断内上下文切换，最终移植的内核（不需要修改内核代码）可以进行多线程任务

说不定下一届还要搞如此浪费时间的实验，我选择开放源代码能帮一个是一个

## Porting Details
移植细节：
    1. 在board.c中实现硬件时钟中断的handler，注册到硬件中断表；实现rt_hw_console_output，使得可以在串口终端输出
    2. 在board.c中注册自动初始化需要的函数，初始化硬件时钟的函数注册到main启动之前最后调用的地方
    3. 在link.lds中显式保留自动初始化函数所在段，防止被编译器优化掉
    4. 在rtconfig.h中把idle，timer，main三个线程的栈设置大一点，否则启动后会栈溢出
    5. 在startup.S中修改函数入口，从main改成entry
    6. 在cpuport.c中实现全局中断开关，直接使用psp的api即可；修改寄存器初始化时mstatus的值到0x1880（0x7880似乎是给支持浮点数的处理器用的）
    7. 在cpuport.h中注释掉`#include <rtconfig.h>`，否则platformIO的编译器会识别里面的内容位汇编代码然后报错
    8. 在cpuport.inc中实现一些便于使用的macros
    9. 在psp_vect_instrumented.S中实现中断时切换上下文的功能，重点是处理完中断后检查是否需要切换上下文然后执行切换，最后上下文出栈，之前的实现直接用C代码在cpuport.c中切换上下文，可能导致出栈两次使得某些寄存器地址不正确
    10. 在port_utils.c中实现系统初始化的接口和一些硬件层，psp的接口，这些接口可以注册到自动初始化函数或者在应用中使用
    11. 选择性实现或移植中断管理，heap，device和shell等功能，当然与其闲着在这嵌入式实验上浪费时间，不如多干点正事

## File Structure (required for minimal porting)
```
Headers:
./include/port_utils.h      Expose utility functions
./src/bsp/rtconfig.h        RT-thread kernel config file
./src/libcpu/cpuport.h      Expose macros for assembly files

Source files:
./src/main.c                User main function
./src/port_utils.c          Layer between psp and kernel
./src/bsp/board.c           Ticks and initialization
./src/libcpu/cpuport.c      Interrupts and context switch

Assembly language files:
./src/bsp/startup.S         Modified to enable bootstrap to enrty()
./src/libcpu/context_gcc.S  Copied from kernel repository (modification: remove rt_hw_interrupt_(enable|diable))
./src/libcpu/psp_vect_instrumented.S  \  
                            Instrumented version of psp interrupt table for implementation of context switch in interrupts  

Link scripts:
./src/bsp/link.lds          Modified to enable automatic initilzation

Kernel:
./src/rtthread              A copy of kernel repository
```

## Files Diff
1. Created files
    * ./include/port_utils.h  
    * ./src/main.c  
    * ./src/port_utils.c  

2. Copied files
    * ./src/rtthread

3. Modified files
    * ./src/libcpu/context_gcc.S  
    * ./src/bsp/rtconfig.h
    * ./src/libcpu/cpuport.h  
    * ./src/bsp/board.c  
    * ./src/libcpu/cpuport.c  
    * ./src/bsp/startup.S  
    * ./src/libcpu/psp_vect_instrumented.S  
    * ./src/bsp/link.lds  

Diffs are placed in [branch_diff.patch](./branch_diff.patch)  