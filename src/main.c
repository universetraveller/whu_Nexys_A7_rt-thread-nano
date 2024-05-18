#include <rtthread.h>
int main(void)
{
    int count = 0;
    for(;;++ count)
    {
        rt_kprintf("Count: %d", count);
        rt_thread_delay(10);
    }
}