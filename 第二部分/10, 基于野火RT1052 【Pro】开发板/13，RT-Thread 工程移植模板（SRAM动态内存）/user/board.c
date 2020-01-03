#include "board.h"                  //修改（1）增加头文件
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "pin_mux.h"

#include <rthw.h>
#include <rtthread.h>
#include "fsl_lpuart.h"
#if 0                               //修改（2） 注释原本的时钟配置 并增加 现有板子的配置
#define _SCB_BASE       (0xE000E010UL)
#define _SYSTICK_CTRL   (*(rt_uint32_t *)(_SCB_BASE + 0x0))
#define _SYSTICK_LOAD   (*(rt_uint32_t *)(_SCB_BASE + 0x4))
#define _SYSTICK_VAL    (*(rt_uint32_t *)(_SCB_BASE + 0x8))
#define _SYSTICK_CALIB  (*(rt_uint32_t *)(_SCB_BASE + 0xC))
#define _SYSTICK_PRI    (*(rt_uint8_t  *)(0xE000ED23UL))

// Updates the variable SystemCoreClock and must be called 
// whenever the core clock is changed during program execution.
extern void SystemCoreClockUpdate(void);

// Holds the system core clock, which is the system clock 
// frequency supplied to the SysTick timer and the processor 
// core clock.
extern uint32_t SystemCoreClock;

static uint32_t _SysTick_Config(rt_uint32_t ticks)
{
    if ((ticks - 1) > 0xFFFFFF)
    {
        return 1;
    }
    
    _SYSTICK_LOAD = ticks - 1; 
    _SYSTICK_PRI = 0xFF;
    _SYSTICK_VAL  = 0;
    _SYSTICK_CTRL = 0x07;  
    
    return 0;
}
#endif
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/* Get debug console frequency. */
uint32_t BOARD_DebugConsoleSrcFreq(void)//增加板级系统时钟相关初始化
{
    uint32_t freq;

    /* To make it simple, we assume default PLL and divider settings, and the only variable
       from application is use PLL3 source or OSC source */
    if (CLOCK_GetMux(kCLOCK_UartMux) == 0) /* PLL3 div6 80M */
    {
        freq = (CLOCK_GetPllFreq(kCLOCK_PllUsb1) / 6U) / (CLOCK_GetDiv(kCLOCK_UartDiv) + 1U);
    }
    else
    {
        freq = CLOCK_GetOscFreq() / (CLOCK_GetDiv(kCLOCK_UartDiv) + 1U);
    }

    return freq;
}

/* Initialize debug console. */
void BOARD_InitDebugConsole(void)
{
    uint32_t uartClkSrcFreq = BOARD_DebugConsoleSrcFreq();

    DbgConsole_Init(BOARD_DEBUG_UART_BASEADDR, BOARD_DEBUG_UART_BAUDRATE, BOARD_DEBUG_UART_TYPE, uartClkSrcFreq);
}

/* MPU configuration. */
void BOARD_ConfigMPU(void)
{
    /* Disable I cache and D cache */
    if (SCB_CCR_IC_Msk == (SCB_CCR_IC_Msk & SCB->CCR)) {
        SCB_DisableICache();
    }
    if (SCB_CCR_DC_Msk == (SCB_CCR_DC_Msk & SCB->CCR)) {
        SCB_DisableDCache();
    }

    /* Disable MPU */
    ARM_MPU_Disable();

    /* MPU configure:
     * Use ARM_MPU_RASR(DisableExec, AccessPermission, TypeExtField, IsShareable, IsCacheable, IsBufferable, SubRegionDisable, Size) 
     * API in core_cm7.h.
     * param DisableExec       Instruction access (XN) disable bit,0=instruction fetches enabled, 1=instruction fetches disabled.
     * param AccessPermission  Data access permissions, allows you to configure read/write access for User and Privileged mode.
     *      Use MACROS defined in core_cm7.h: ARM_MPU_AP_NONE/ARM_MPU_AP_PRIV/ARM_MPU_AP_URO/ARM_MPU_AP_FULL/ARM_MPU_AP_PRO/ARM_MPU_AP_RO
     * Combine TypeExtField/IsShareable/IsCacheable/IsBufferable to configure MPU memory access attributes.
     *  TypeExtField  IsShareable  IsCacheable  IsBufferable   Memory Attribtue    Shareability        Cache
     *     0             x           0           0             Strongly Ordered    shareable
     *     0             x           0           1              Device             shareable           
     *     0             0           1           0              Normal             not shareable   Outer and inner write through no write allocate           
     *     0             0           1           1              Normal             not shareable   Outer and inner write back no write allocate           
     *     0             1           1           0              Normal             shareable       Outer and inner write through no write allocate    
     *     0             1           1           1              Normal             shareable       Outer and inner write back no write allocate    
     *     1             0           0           0              Normal             not shareable   outer and inner noncache
     *     1             1           0           0              Normal             shareable       outer and inner noncache
     *     1             0           1           1              Normal             not shareable   outer and inner write back write/read acllocate
     *     1             1           1           1              Normal             shareable       outer and inner write back write/read acllocate
     *     2             x           0           0              Device              not shareable   
     *  Above are normal use settings, if your want to see more details or want to config different inner/outter cache policy.      
     *  please refer to Table 4-55 /4-56 in arm cortex-M7 generic user guide <dui0646b_cortex_m7_dgug.pdf>
     * param SubRegionDisable  Sub-region disable field. 0=sub-region is enabled, 1=sub-region is disabled.
     * param Size              Region size of the region to be configured. use ARM_MPU_REGION_SIZE_xxx MACRO in core_cm7.h.
     */

    /* Region 0 setting: Memory with Device type, not shareable, non-cacheable. */
    MPU->RBAR = ARM_MPU_RBAR(0, 0xC0000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 2, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_512MB);

    /* Region 1 setting: Memory with Device type, not shareable,  non-cacheable. */
    MPU->RBAR = ARM_MPU_RBAR(1, 0x80000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 2, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_1GB);

/* Region 2 setting */
#if defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1)
    /* Setting Memory with Normal type, not shareable, outer/inner write back. */
    MPU->RBAR = ARM_MPU_RBAR(2, 0x60000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_512MB);
#else
    /* Setting Memory with Device type, not shareable, non-cacheable. */
    MPU->RBAR = ARM_MPU_RBAR(2, 0x60000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 2, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_512MB);
#endif

    /* Region 3 setting: Memory with Device type, not shareable, non-cacheable. */
    MPU->RBAR = ARM_MPU_RBAR(3, 0x00000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 2, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_1GB);

    /* Region 4 setting: Memory with Normal type, not shareable, outer/inner write back */
    MPU->RBAR = ARM_MPU_RBAR(4, 0x00000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_128KB);

    /* Region 5 setting: Memory with Normal type, not shareable, outer/inner write back */
    MPU->RBAR = ARM_MPU_RBAR(5, 0x20000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_128KB);

    /* Region 6 setting: Memory with Normal type, not shareable, outer/inner write back */
    MPU->RBAR = ARM_MPU_RBAR(6, 0x20200000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_256KB);

    /* Region 7 setting: Memory with Normal type, not shareable, outer/inner write back */
    MPU->RBAR = ARM_MPU_RBAR(7, 0x80000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_32MB);

    /* Region 8 setting, set last 2MB of SDRAM can't be accessed by cache, glocal variables which are not expected to be
     * accessed by cache can be put here */
    /* Memory with Normal type, not shareable, non-cacheable */
    MPU->RBAR = ARM_MPU_RBAR(8, 0x81E00000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_2MB);

    /* Enable MPU */
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);

    /* Enable I cache and D cache */
    SCB_EnableDCache();
    SCB_EnableICache();
}


#if 0   //修改（3）注释原有的堆空间地址  增加适合板子的地址空间
#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
#define RT_HEAP_SIZE 1024
static uint32_t rt_heap[RT_HEAP_SIZE];	// heap default size: 4K(1024 * 4)
RT_WEAK void *rt_heap_begin_get(void)
{
    return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
    return rt_heap + RT_HEAP_SIZE;
}
#endif
#endif
#ifdef RT_USING_HEAP

#if defined(RT_USING_MEMHEAP) && defined(RT_USING_MEMHEAP_AS_HEAP)
/* 从外部SDRAM里面分配一部分静态内存来作为rtt的堆空间，这里配置为30MB */
#define SDRAM_BEGIN         (0x80000000u)
#define SDRAM_END           (0x81D00000u)
#define LCD_FB_BASE         (0x81D10000u)

#elif defined(RT_USING_SMALL_MEM)
/* 从内部SRAM（即DTCM）里面分配一部分静态内存来作为rtt的堆空间，这里配置为4KB */
#define RT_HEAP_SIZE 1024
static uint32_t rt_heap[RT_HEAP_SIZE];
RT_WEAK void *rt_heap_begin_get(void)
{
    return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
    return rt_heap + RT_HEAP_SIZE;
}
#endif

#endif 
/**
 * This function will initial your board.
 */
void rt_hw_board_init()
{	
#if 0               //修改（4） 增加板级驱动
	/* System Clock Update */
	SystemCoreClockUpdate();
	
	/* System Tick Configuration */
	_SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);
#endif
    //	  /* 初始化内存管理单元 */
	  /* 初始化内存管理单元 */
    BOARD_ConfigMPU();
  
		/* 初始化开发板引脚 */
    BOARD_InitPins();
  
		/* 初始化开发板时钟 */
    BOARD_BootClockRUN(); 
  
		/* 初始化调试串口 */
		BOARD_InitDebugConsole();
    PRINTF("*****欢迎使用 野火i.MX RT1052 开发板*****\r\n");
/* 将开发板硬件相关的初始化放上面 */
  
    /* 初始化SysTick */
    SysTick_Config( SystemCoreClock / RT_TICK_PER_SECOND ); 
    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
    
#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
	rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
#if 0
#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif
#endif

#ifdef RT_USING_HEAP
	#if defined(RT_USING_MEMHEAP) && defined(RT_USING_MEMHEAP_AS_HEAP)
			//PRINTF("sdram heap, begin: 0x%p, end: 0x%p\n", SDRAM_BEGIN, SDRAM_END);
			rt_system_heap_init((void *)SDRAM_BEGIN, (void *)SDRAM_END);
	#elif defined(RT_USING_SMALL_MEM)
			PRINTF("sram heap, begin: 0x%p, end: 0x%p\n", HEAP_BEGIN, HEAP_END);
			rt_system_heap_init((void *)rt_heap_begin_get, (void *)rt_heap_end_get);
	#endif
#endif 
}

void SysTick_Handler(void)
{
	/* enter interrupt */
	rt_interrupt_enter();

	rt_tick_increase();

	/* leave interrupt */
	rt_interrupt_leave();
}
