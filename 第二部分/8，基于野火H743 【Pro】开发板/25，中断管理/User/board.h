#ifndef __BOARD_H__
#define __BOARD_H__

/*
*************************************************************************
*                             包含的头文件
*************************************************************************
*/
/* STM32 固件库头文件 */
#include "stm32h7xx.h"
#include "main.h"
/* 开发板硬件bsp头文件 */
#include "./led/bsp_led.h"
#include "./exti/bsp_exti.h"

#include "./delay/core_delay.h"   
#include "./usart/bsp_debug_usart.h"
/*
*************************************************************************
*                               函数声明
*************************************************************************
*/
void rt_hw_board_init(void);
void SysTick_Handler(void);
	

#endif /* __BOARD_H__ */
