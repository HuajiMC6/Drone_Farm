#ifndef SDRAM_MALLOC_H
#define SDRAM_MALLOC_H

#include "gd32h7xx.h"

#define configTOTAL_HEAP_SIZE 		32*1024*1024 - 1024*600*2

#define portPOINTER_SIZE_TYPE uint32_t

#define NULL 0

/* 简易临界区实现：直接开关全局中断 */
#define taskENTER_CRITICAL()     __disable_irq()   // Cortex-M专用指令
#define taskEXIT_CRITICAL()      __enable_irq()    // Cortex-M专用指令

#define configASSERT( x ) if( ( x ) == 0 ) { for( ;; ); }

/* 这个宏用于内核的代码覆盖率测试，在生产代码中应定义为空。 */
#define mtCOVERAGE_TEST_MARKER()
/* 这个宏用于freertos的trace功能，定义为空。 */
#define traceMALLOC( pvReturn, xWantedSize )
#define traceFREE( pv, xBlockSize )

/* 根据你的单片机架构定义对齐基数。对于 Cortex-M 系列，通常是 8 */
#define portBYTE_ALIGNMENT 8

/* 根据对齐基数，自动推导出对齐掩码 */
#if portBYTE_ALIGNMENT == 8
    #define portBYTE_ALIGNMENT_MASK ( 0x0007 )
#elif portBYTE_ALIGNMENT == 4
    #define portBYTE_ALIGNMENT_MASK ( 0x0003 )
#elif portBYTE_ALIGNMENT == 2
    #define portBYTE_ALIGNMENT_MASK ( 0x0001 )
#elif portBYTE_ALIGNMENT == 1
    #define portBYTE_ALIGNMENT_MASK ( 0x0000 )
#else
    #error "Invalid portBYTE_ALIGNMENT definition"
#endif

typedef struct xHeapStats
{
    size_t xAvailableHeapSpaceInBytes;      /* 当前可用的总堆空间（所有空闲块之和） */
    size_t xSizeOfLargestFreeBlockInBytes;   /* 最大空闲块的大小 */
    size_t xSizeOfSmallestFreeBlockInBytes;  /* 最小空闲块的大小 */
    size_t xNumberOfFreeBlocks;              /* 空闲块的数量 */
    size_t xMinimumEverFreeBytesRemaining;   /* 系统启动以来的最小空闲内存量 */
    size_t xNumberOfSuccessfulAllocations;   /* 成功分配内存的次数 */
    size_t xNumberOfSuccessfulFrees;         /* 成功释放内存的次数 */
} HeapStats_t;

/* 重命名 */
#define sdram_malloc pvPortMalloc
#define sdram_free vPortFree

void *pvPortMalloc( size_t xWantedSize );
void vPortFree( void *pv );
size_t xPortGetFreeHeapSize( void );
size_t xPortGetMinimumEverFreeHeapSize( void );
void vPortGetHeapStats( HeapStats_t *pxHeapStats );

#endif
