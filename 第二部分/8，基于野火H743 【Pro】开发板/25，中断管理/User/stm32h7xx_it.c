/* 该文件统一用于存放中断服务函数 */
#include "stm32h7xx_hal.h"
#include "stm32h7xx.h"
#include "stm32h7xx_it.h"

#include "board.h"
#include "rtthread.h"

/* 外部定义消息队列控制块 */
extern rt_mq_t test_mq;

uint32_t send_data1 = 1;
uint32_t send_data2 = 2;

/*********************************************************************************
  * @ 函数名  ： KEY1_IRQHandler
  * @ 功能说明： 中断服务函数
  * @ 参数    ： 无  
  * @ 返回值  ： 无
  ********************************************************************************/
void KEY1_IRQHandler(void)
{
  /* 进入中断 */
  rt_interrupt_enter();
  
  //确保是否产生了EXTI Line中断
	if(__HAL_GPIO_EXTI_GET_IT(KEY1_INT_GPIO_PIN) != RESET) 
	{
    /* 将数据写入（发送）到队列中，等待时间为 0  */
    rt_mq_send(	test_mq,	/* 写入（发送）队列的ID(句柄) */
                &send_data1,			/* 写入（发送）的数据 */
                sizeof(send_data1));			/* 数据的长度 */
    //清除中断标志位
		__HAL_GPIO_EXTI_CLEAR_IT(KEY1_INT_GPIO_PIN);     
	}  
  
  /* 离开中断 */
  rt_interrupt_leave();
}
/*********************************************************************************
  * @ 函数名  ： KEY2_IRQHandler
  * @ 功能说明： 中断服务函数
  * @ 参数    ： 无  
  * @ 返回值  ： 无
  ********************************************************************************/
void KEY2_IRQHandler(void)
{
  /* 进入中断 */
  rt_interrupt_enter();
  
  //确保是否产生了EXTI Line中断
	if(__HAL_GPIO_EXTI_GET_IT(KEY2_INT_GPIO_PIN) != RESET) 
	{
    /* 将数据写入（发送）到队列中，等待时间为 0  */
    rt_mq_send(	test_mq,	/* 写入（发送）队列的ID(句柄) */
                &send_data2,			/* 写入（发送）的数据 */
                sizeof(send_data2));			/* 数据的长度 */
    //清除中断标志位
		__HAL_GPIO_EXTI_CLEAR_IT(KEY2_INT_GPIO_PIN);     
	}  
  
  /* 离开中断 */
  rt_interrupt_leave();
}


// 串口中断服务函数
/* USER CODE BEGIN 1 */
void DEBUG_USART_IRQHandler(void)
{
  /* 进入中断 */
  rt_interrupt_enter();
  
  if((READ_REG(UartHandle.Instance->ISR)& USART_ISR_IDLE) != RESET)
  {
			UART_IdelCallback();
      __HAL_UART_CLEAR_IT(&UartHandle, UART_CLEAR_IDLEF);
  }
  
  /* 离开中断 */
  rt_interrupt_leave();
}

