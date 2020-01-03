#include "./bsp/dma_uart/bsp_dma_uart.h"
#include "fsl_debug_console.h"  
#include "fsl_lpuart_edma.h"
#include "board.h"
#include "rtthread.h"

/* 外部定义信号量控制块 */
extern rt_sem_t test_sem;
uint8_t ucTemp[UCTEMP_LEN];

/*句柄定义*/
lpuart_edma_handle_t g_lpuartEdmaHandle;
edma_handle_t g_lpuartTxEdmaHandle;
edma_handle_t g_lpuartRxEdmaHandle;
lpuart_transfer_t receiveXfer;
/*全局变量*/
int Get_Uart_Data_flag=0;
uint32_t data;

uint8_t g_rxBuffer[RX_BUFFER_LEN] = {0};
uint8_t RX_BUFF[USART_RBUFF_SIZE]={0};
/*
*函数功能：初始化串口引脚
注意:本实验使用的是UART1,UART1是系统串口，在系统启动后已经完成了引脚的初始化
所以在这里可以屏蔽了UART1引脚初始化，如果使用其他串口请添加上引脚初始化代码是必须添加的。
*/
void UART_GPIO_Init(void)
{
  /* 定义gpio初始化配置结构体 */
  gpio_pin_config_t adc_config;
  /*ADC，GPIO配置*/   
  adc_config.direction = kGPIO_DigitalInput; //输入模式
//adc_config.outputLogic =  1;                //默认高电平，在输入模式下配置该选项无效
  adc_config.interruptMode = kGPIO_NoIntmode; //不使用中断
  
  
  IOMUXC_SetPinMux(UART_RX_IOMUXC, 0U);                                      
  IOMUXC_SetPinMux(UART_TX_IOMUXC, 0U);
  
  IOMUXC_SetPinConfig(UART_RX_IOMUXC, UART_RX_PAD_CONFIG_DATA);
  IOMUXC_SetPinConfig(UART_TX_IOMUXC, UART_TX_PAD_CONFIG_DATA);
  
  GPIO_PinInit(UART_RX_GPIO, UART_RX_GPIO_PIN, &adc_config);
  
  adc_config.direction = kGPIO_DigitalOutput; //输入模式
  adc_config.outputLogic =  1;                //默认高电平
  GPIO_PinInit(UART_TX_GPIO, UART_TX_GPIO_PIN, &adc_config);
  
}


/*
*函数功能：初始化串口
*/
void UART_Init(void)
{
  lpuart_config_t lpuartConfig;//定义LUART初始化结构体
  
  /*初始化UART引脚*/
  UART_GPIO_Init();
  /* LPUART.默认配置 */
  /*
  * lpuartConfig.baudRate_Bps = 115200U;
  * lpuartConfig.parityMode = kLPUART_ParityDisabled;
  * lpuartConfig.stopBitCount = kLPUART_OneStopBit;
  * lpuartConfig.txFifoWatermark = 0;
  * lpuartConfig.rxFifoWatermark = 0;
  * lpuartConfig.enableTx = false;
  * lpuartConfig.enableRx = false;
  */
  LPUART_GetDefaultConfig(&lpuartConfig);
  lpuartConfig.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
  lpuartConfig.enableTx = true;
  lpuartConfig.enableRx = true;
  
  LPUART_Init(DEMO_LPUART, &lpuartConfig, DEMO_LPUART_CLK_FREQ);
}

/*
	串口空闲中断
*/
void LPUART_UserCallback(LPUART_Type *base, lpuart_edma_handle_t *handle, status_t status, void *userData)
{

}
extern uint8_t ucTemp[];

/*
*函数功能：初始化串口所使用的DMA
*/
void UART_DMA_Init(void)
{
  edma_config_t config;//定义eDMA初始化结构体
  
  /*初始化DMAMUX */
  DMAMUX_Init(EXAMPLE_LPUART_DMAMUX_BASEADDR);
  /* 为LPUART设置DMA传输通道*/
  DMAMUX_SetSource(EXAMPLE_LPUART_DMAMUX_BASEADDR, LPUART_TX_DMA_CHANNEL, LPUART_TX_DMA_REQUEST);
  DMAMUX_SetSource(EXAMPLE_LPUART_DMAMUX_BASEADDR, LPUART_RX_DMA_CHANNEL, LPUART_RX_DMA_REQUEST);
  DMAMUX_EnableChannel(EXAMPLE_LPUART_DMAMUX_BASEADDR, LPUART_TX_DMA_CHANNEL);
  DMAMUX_EnableChannel(EXAMPLE_LPUART_DMAMUX_BASEADDR, LPUART_RX_DMA_CHANNEL);
  
  /* 初始化DMA */
  EDMA_GetDefaultConfig(&config);
  EDMA_Init(EXAMPLE_LPUART_DMA_BASEADDR, &config);
  /*创建eDMA传句柄*/
  EDMA_CreateHandle(&g_lpuartTxEdmaHandle, EXAMPLE_LPUART_DMA_BASEADDR, LPUART_TX_DMA_CHANNEL);
  EDMA_CreateHandle(&g_lpuartRxEdmaHandle, EXAMPLE_LPUART_DMA_BASEADDR, LPUART_RX_DMA_CHANNEL);
  
  /* 创建 LPUART DMA 句柄 */
  LPUART_TransferCreateHandleEDMA(DEMO_LPUART, &g_lpuartEdmaHandle, LPUART_UserCallback, NULL, &g_lpuartTxEdmaHandle,
                                  &g_lpuartRxEdmaHandle);
	/*使能空闲中断*/
	LPUART_EnableInterrupts(DEMO_LPUART, kLPUART_IdleLineInterruptEnable);
	/*使能串口中断**/
	EnableIRQ(DEMO_LPUART_IRqn);
	
	receiveXfer.data = RX_BUFF; //地址数组
	receiveXfer.dataSize = USART_RBUFF_SIZE;    //接收的最大数量
	LPUART_ReceiveEDMA(LPUART1, &g_lpuartEdmaHandle, &receiveXfer);  //使用eDMA接收
}

void DEBUG_UART_IRQHandler(void)
{
		/* 进入中断 */
		rt_interrupt_enter();

		/*	接收到数据满了触发中断	*/
    if ((kLPUART_IdleLineFlag) & LPUART_GetStatusFlags(DEMO_LPUART))
    {
			/*清除空闲中断*/
			DEMO_LPUART->STAT |= LPUART_STAT_IDLE_MASK; 
			/*接收eDMA的数据量*/
			LPUART_TransferGetReceiveCountEDMA(DEMO_LPUART, &g_lpuartEdmaHandle, &data); 
			Get_Uart_Data_flag=1;
			LPUART_TransferAbortReceiveEDMA(DEMO_LPUART, &g_lpuartEdmaHandle);   //eDMA终止接收数据
			LPUART_ReceiveEDMA(DEMO_LPUART, &g_lpuartEdmaHandle, &receiveXfer);  //使用eDMA接收
			
			//   //给出二值信号量 ，发送接收到新数据标志，供前台程序查询
			rt_sem_release(test_sem);  
			
    }
		/* 离开中断 */
		rt_interrupt_leave();
}
