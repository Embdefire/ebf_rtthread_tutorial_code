  /**
  ******************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2019-xx-xx
  * @brief   RT-Thread系统 双向链表
  ******************************************************************
  * @attention
  *
  * 实验平台:基于野火I.MX RT全系列开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :http://firestm32.taobao.com
  *
  ******************************************************************
  */
/*
*************************************************************************
*                             包含的头文件
*************************************************************************
*/ 

#include "board.h"
#include "fsl_debug_console.h"  
/* RT-Thread 头文件 */
#include "rtthread.h"
#include "cpuusage.h"

/*
*************************************************************************
*                               变量
*************************************************************************
*/
/* 定义线程控制块 */
static rt_thread_t led1_thread = RT_NULL;
static rt_thread_t get_cpu_use_thread = RT_NULL;
/*
*************************************************************************
*                             函数声明
*************************************************************************
*/
static void led1_thread_entry(void* parameter);
static void get_cpu_use_thread_entry(void* parameter);

/*
*************************************************************************
*                             main 函数
*************************************************************************
*/
/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void)
{
/*
     * 开发板硬件初始化，RTT系统初始化已经在main函数之前完成，
     * 即在component.c文件中的rtthread_startup()函数中完成了。
     * 所以在main函数中，只需要创建线程和启动线程即可。
     */

    rt_kprintf("这是一个[野火]-I.MX RT全系列开发板-RTT-CPU利用率统计实验\r\n");

    led1_thread =                          /* 线程控制块指针 */
        rt_thread_create( "led1",              /* 线程名字 */
                          led1_thread_entry,   /* 线程入口函数 */
                          RT_NULL,             /* 线程入口函数参数 */
                          512,                 /* 线程栈大小 */
                          3,                   /* 线程的优先级 */
                          20);                 /* 线程时间片 */

/* 启动线程，开启调度 */
if (led1_thread != RT_NULL)
        rt_thread_startup(led1_thread);
else
return -1;

    get_cpu_use_thread =                          /* 线程控制块指针 */
        rt_thread_create( "get_cpu_use",              /* 线程名字 */
                          get_cpu_use_thread_entry,   /* 线程入口函数 */
                          RT_NULL,             /* 线程入口函数参数 */
                          512,                 /* 线程栈大小 */
                          5,                   /* 线程的优先级 */
                          20);                 /* 线程时间片 */

/* 启动线程，开启调度 */
if (get_cpu_use_thread != RT_NULL)
        rt_thread_startup(get_cpu_use_thread);
else
return -1;
}

void delay_test(int num)
{
	int i=0;
	int j=0;
	for(j=0;j<num;j++)
	{
		for(;i<1000000;i++)
		{
		}
	}
}

/*
*************************************************************************
*                             线程定义
*************************************************************************
*/

static void led1_thread_entry(void* parameter)
{
    rt_uint16_t i;

    while (1) 
    {
        LED1_TOGGLE;
        /* 模拟占用CPU资源，修改数值作为模拟测试 */
        for (i = 0; i < 1000; i++)
        {
					delay_test(1000);
        }
			rt_thread_delay(500);   /* 延时5个tick */
    }
}

static void get_cpu_use_thread_entry(void* parameter)
{
    rt_uint8_t major,minor;
    while (1) 
    {
        /* 获取CPU利用率数据 */
        cpu_usage_get(&major,&minor);

        /* 打印CPU利用率 */
        rt_kprintf("CPU利用率 = %d.%d%\r\n",major,minor);

        rt_thread_delay(1000);   /* 延时1000个tick */
    }
}
 
 /*******************************END OF FILE****************************/
