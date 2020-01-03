/**
  ******************************************************************
  * @file    bsp_key.c
  * @author  fire
  * @version V1.0
  * @date    2018-xx-xx
  * @brief   按键应用函数接口
  ******************************************************************
  * @attention
  *
  * 实验平台:野火  i.MXRT1052开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************
  */
#include "fsl_iomuxc.h"
#include "fsl_gpio.h" 

#include "pad_config.h" 
#include "./key/bsp_key.h"  
#include "./bsp/nvic/bsp_nvic.h"
#include "./delay/core_delay.h"  

#include "board.h"
#include "rtthread.h"

/* 外部定义消息队列控制块 */
extern rt_mq_t test_mq;

uint32_t send_data1 = 1;
uint32_t send_data2 = 2;

/******************************************************************
 * 变量定义
  ******************************************************************/
/* 按键是否被按下的中断标志 */
__IO bool g_KeyDown[2] = { false};

/******************************************************************
 * 宏
  ******************************************************************/
/* 按键中断检测引脚的PAD配置 */
#define KEY_PAD_CONFIG_DATA            (SRE_0_SLOW_SLEW_RATE| \
                                        DSE_0_OUTPUT_DRIVER_DISABLED| \
                                        SPEED_2_MEDIUM_100MHz| \
                                        ODE_0_OPEN_DRAIN_DISABLED| \
                                        PKE_1_PULL_KEEPER_ENABLED| \
                                        PUE_1_PULL_SELECTED| \
                                        PUS_3_22K_OHM_PULL_UP| \
                                        HYS_1_HYSTERESIS_ENABLED)   
    /* 配置说明 : */
    /* 转换速率: 转换速率慢
      驱动强度: 关闭
      速度配置 : medium(100MHz)
      开漏配置: 关闭 
      拉/保持器配置: 使能
      拉/保持器选择: 上下拉
      上拉/下拉选择: 22K欧姆上拉
      滞回器配置: 开启 （仅输入时有效，施密特触发器，使能后可以过滤输入噪声）*/

/******************************************************************
 * 声明
  ******************************************************************/
static void Key_IOMUXC_MUX_Config(void);
static void Key_IOMUXC_PAD_Config(void);
static void Key_GPIO_Mode_Config(void);

/******************************************************************
 * 函数代码
  ******************************************************************/
/**
* @brief  初始化按键相关IOMUXC的MUX复用配置
* @param  无
* @retval 无
*/
static void Key_IOMUXC_MUX_Config(void)
{
  /* 设置按键引脚的复用模式为GPIO，不使用SION功能 */
  IOMUXC_SetPinMux(CORE_BOARD_WAUP_KEY_IOMUXC, 0U);
  IOMUXC_SetPinMux(CORE_BOARD_MODE_KEY_IOMUXC, 0U); 
}

/**
* @brief  初始化按键相关引脚PAD属性
* @param  无
* @retval 无
*/
static void Key_IOMUXC_PAD_Config(void)
{
  /* 设置按键引脚属性功能 */    
  IOMUXC_SetPinConfig(CORE_BOARD_WAUP_KEY_IOMUXC, KEY_PAD_CONFIG_DATA); 
  IOMUXC_SetPinConfig(CORE_BOARD_MODE_KEY_IOMUXC, KEY_PAD_CONFIG_DATA); 
}

 /**
  * @brief  初始化按键相关的GPIO模式
  * @param  无
  * @retval 无
  */
static void Key_GPIO_Mode_Config(void)
{     
  /* 配置为输入模式，低电平中断，后面通过GPIO_PinInit函数加载配置 */
  gpio_pin_config_t key_config;
  
  /** 核心板的按键，GPIO配置 **/       
  key_config.direction = kGPIO_DigitalInput;    //输入模式
  key_config.outputLogic =  1;                  //默认高电平（输入模式时无效）
  key_config.interruptMode = kGPIO_IntLowLevel;//kGPIO_IntFallingEdge; //低电平触发中断
  
  /* 初始化 KEY GPIO. */
  GPIO_PinInit(CORE_BOARD_WAUP_KEY_GPIO, CORE_BOARD_WAUP_KEY_GPIO_PIN, &key_config);
  GPIO_PinInit(CORE_BOARD_MODE_KEY_GPIO, CORE_BOARD_MODE_KEY_GPIO_PIN, &key_config);
}


/**
 * @brief  初始化按键中断相关的内容
 * @param  无
 * @retval 无
 */
static void Key_Interrupt_Config(void)   
{
  /* 开启GPIO引脚的中断 */
  GPIO_PortEnableInterrupts(CORE_BOARD_WAUP_KEY_GPIO, 1U << CORE_BOARD_WAUP_KEY_GPIO_PIN);                           
  GPIO_PortEnableInterrupts(CORE_BOARD_MODE_KEY_GPIO, 1U << CORE_BOARD_MODE_KEY_GPIO_PIN); 
  
  /*设置中断优先级,*/
  set_IRQn_Priority(CORE_BOARD_WAUP_KEY_IRQ,Group4_PreemptPriority_6, Group4_SubPriority_0);
  set_IRQn_Priority(CORE_BOARD_MODE_KEY_IRQ,Group4_PreemptPriority_6, Group4_SubPriority_1);
  
  /* 开启GPIO端口中断 */
  EnableIRQ(CORE_BOARD_WAUP_KEY_IRQ);
  EnableIRQ(CORE_BOARD_MODE_KEY_IRQ);
}


 /**
  * @brief  初始化控制KEY的IO    
  * @param  无
  * @retval 无
  */
void Key_IT_GPIO_Config(void)
{
  /* 初始化GPIO复用、属性、模式及中断*/
  Key_IOMUXC_MUX_Config();
  Key_IOMUXC_PAD_Config();
  Key_GPIO_Mode_Config();
  Key_Interrupt_Config();
}

/********************中断服务函数**************************/
/**
 * @brief  GPIO 输入中断服务函数
 *         CORE_BOARD_WAUP_KEY_IRQHandler只是一个宏，
 *         在本例中它指代函数名GPIO5_Combined_0_15_IRQHandler，
 *         中断服务函数名是固定的，可以在启动文件中找到。
 * @param  中断服务函数不能有输入参数
 * @note   中断函数一般只使用标志位进行指示，完成后尽快退出，
 *         具体操作或延时尽量不放在中断服务函数中
 * @retval 中断服务函数不能有返回值
 */
void CORE_BOARD_WAUP_KEY_IRQHandler(void)
{ 
		/* 进入中断 */
		rt_interrupt_enter();
		/*	检测按键中断标志	*/	
		if(GPIO_GetPinsInterruptFlags(CORE_BOARD_WAUP_KEY_GPIO)&(1U<<CORE_BOARD_WAUP_KEY_GPIO_PIN))
		{
			CPU_TS_Tmr_Delay_MS(10);
				/*	判断按键是否 按下	*/
			if(GPIO_PinRead(CORE_BOARD_WAUP_KEY_GPIO, CORE_BOARD_WAUP_KEY_GPIO_PIN) == 1)
			{
				/* 将数据写入（发送）到队列中，等待时间为 0  */
				rt_mq_send(	test_mq,	/* 写入（发送）队列的ID(句柄) */
										&send_data1,			/* 写入（发送）的数据 */
										sizeof(send_data1));			/* 数据的长度 */	
			}	
		}
			/* 清除中断标志位 */
		GPIO_PortClearInterruptFlags(CORE_BOARD_WAUP_KEY_GPIO,
																	 1U << CORE_BOARD_WAUP_KEY_GPIO_PIN);  
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
		/* 离开中断 */
		rt_interrupt_leave();
}

/**
 * @brief  GPIO 输入中断服务函数
 *         CORE_BOARD_MODE_KEY_IRQHandler只是一个宏，
 *         在本例中它指代函数名GPIO1_Combined_0_15_IRQHandler，
 *         中断服务函数名是固定的，可以在启动文件中找到。
 * @param  中断服务函数不能有输入参数
 * @note   中断函数一般只使用标志位进行指示，完成后尽快退出，
 *         具体操作或延时尽量不放在中断服务函数中
 * @retval 中断服务函数不能有返回值
 */
void CORE_BOARD_MODE_KEY_IRQHandler(void)
{ 
		/* 进入中断 */
		rt_interrupt_enter();

			/*	检测按键中断标志	*/	
		if(GPIO_GetPinsInterruptFlags(CORE_BOARD_MODE_KEY_GPIO)&(1U<<CORE_BOARD_MODE_KEY_GPIO_PIN))
		{
			CPU_TS_Tmr_Delay_MS(10);
				/*	判断按键是否 按下	*/
			if(GPIO_PinRead(CORE_BOARD_MODE_KEY_GPIO, CORE_BOARD_MODE_KEY_GPIO_PIN) == 1)
			{
    /* 将数据写入（发送）到队列中，等待时间为 0  */
    rt_mq_send(	test_mq,	/* 写入（发送）队列的ID(句柄) */
                &send_data2,			/* 写入（发送）的数据 */
                sizeof(send_data2));			/* 数据的长度 */	
			}
		}
		/* 清除中断标志位 */
    GPIO_PortClearInterruptFlags(CORE_BOARD_MODE_KEY_GPIO,
                                 1U << CORE_BOARD_MODE_KEY_GPIO_PIN);  
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
		/* 离开中断 */
		rt_interrupt_leave();
}

