#include <rtthread.h>

#define THREAD_PRIORITY         25
#define THREAD_STACK_SIZE       512
#define THREAD_TIMESLICE        5

static rt_thread_t tid1 = RT_NULL;

/* 线程 1 的入口函数 */
static void thread1_entry(void *parameter)
{
    rt_uint32_t count = 0;

    while (1)
    {
        /* 线程 1 采用低优先级运行，一直打印计数值 */
        rt_kprintf("thread1 count: %d\n", count ++);
        rt_thread_mdelay(500);
    }
}

ALIGN(RT_ALIGN_SIZE)
static char thread2_stack[1024];
static struct rt_thread thread2;
static char thread1_stack[1024];
static struct rt_thread thread1;
/* 线程 2 入口 */
static void thread2_entry(void *param)
{
    rt_uint32_t count = 0;

    /* 线程 2 拥有较高的优先级，以抢占线程 1 而获得执行 */
    for (count = 0; count < 10 ; count++)
    {
        /* 线程 2 打印计数值 */
        rt_kprintf("thread2 count: %d\n", count);
    }
    rt_kprintf("thread2 exit\n");
    /* 线程 2 运行结束后也将自动被系统脱离 */
}

int thread_sample(void);
/* 线程示例 */
int thread_sample(void)
{
    rt_err_t res;
    tid1 = &thread1;
    /* 创建线程 1，名称是 thread1，入口是 thread1_entry*/
    res = rt_thread_init(tid1, "thread1",
                            thread1_entry, RT_NULL,
                            thread1_stack,
                            sizeof(thread1_stack),
                            7, THREAD_TIMESLICE);

    RT_ASSERT(res == RT_EOK);
    /* 如果获得线程控制块，启动这个线程 */
    rt_thread_startup(tid1);

    tid1 = &thread2;
    /* 初始化线程 2，名称是 thread2，入口是 thread2_entry */
    res = rt_thread_init(tid1,
                   "thread2",
                   thread2_entry,
                   RT_NULL,
                   thread2_stack,
                   sizeof(thread2_stack),
                   6, THREAD_TIMESLICE);
    RT_ASSERT(res == RT_EOK);
    rt_thread_startup(tid1);

    return res;
}