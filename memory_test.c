#include "drivers.h"

typedef struct {
    uint32_t total_bytes;      /* 测试总字节数 */
    uint32_t error_count;       /* 错误数量 */
    uint32_t first_error_addr;  /* 第一个错误地址 */
    uint32_t expected_data;     /* 期望数据 */
    uint32_t actual_data;       /* 实际数据 */
    uint32_t test_pattern;      /* 使用的测试模式 */
    uint8_t  test_passed;       /* 测试是否通过 */
} SdramTestResult_t;

/* PRBS生成器（使用线性反馈移位寄存器 LFSR）*/
static uint32_t prbs_next(uint32_t *state)
{
    /* 使用多项式 x^32 + x^22 + x^2 + x^1 + 1 */
    uint32_t bit = ((*state >> 31) ^ (*state >> 21) ^ 
                    (*state >> 1) ^ (*state >> 0)) & 1;
    *state = (*state << 1) | bit;
    return *state;
}

SdramTestResult_t sdram_test_prbs(uint32_t start_addr, uint32_t size, uint32_t seed)
{
    SdramTestResult_t result = {0};
    volatile uint32_t *addr;
    uint32_t lfsr_state;
    uint32_t expected;
    uint32_t step;
    
    /* 参数检查 */
    if(size < 4) {
        result.test_passed = 0;
        return result;
    }
    
    /* 确保地址4字节对齐 */
    if(start_addr & 0x3) {
        start_addr = (start_addr + 3) & ~0x3;
    }
    
    /* 确保大小是4的倍数 */
    size = size & ~0x3;
    
    result.total_bytes = size;
    result.test_pattern = 0;
    
    /* === 第一阶段：写入伪随机序列 === */
    printf("Writing PRBS pattern (seed: 0x%08X)...\r\n", seed);
    
    lfsr_state = seed;
    for(step = 0; step < size / 4; step++) {
        addr = (volatile uint32_t *)(start_addr + step * 4);
        expected = prbs_next(&lfsr_state);
        *addr = expected;
        
        /* 每写入1MB打印一个点表示进度 */
        if((step & 0x3FFFF) == 0) {  /* 约1MB = 262144个32位字 */
            printf(".");
        }
    }
    printf(" Write complete!\r\n");
    
    /* === 第二阶段：读出并校验 === */
    printf("Verifying data...\r\n");
    
    lfsr_state = seed;  /* 重新初始化LFSR */
    for(step = 0; step < size / 4; step++) {
        addr = (volatile uint32_t *)(start_addr + step * 4);
        expected = prbs_next(&lfsr_state);
        
        uint32_t actual = *addr;
        
        if(actual != expected) {
            result.error_count++;
            
            /* 记录第一个错误 */
            if(result.error_count == 1) {
                result.first_error_addr = (uint32_t)addr;
                result.expected_data = expected;
                result.actual_data = actual;
                
                /* 打印第一个错误详细信息 */
                printf("\r\n!!! First error at address 0x%08X !!!\r\n", (uint32_t)addr);
                printf("  Expected: 0x%08X, Actual: 0x%08X\r\n", expected, actual);
                printf("  Bit errors: ");
                
                /* 显示哪些位出错 */
                uint32_t xor = expected ^ actual;
                for(int bit = 31; bit >= 0; bit--) {
                    if(xor & (1 << bit)) {
                        printf("%d ", bit);
                    }
                }
                printf("\r\n");while(1);
            }
            
            /* 如果错误太多，提前终止 */
            if(result.error_count >= 100) {
                printf("Too many errors (>100), stopping test...\r\n");
                break;
            }
        }
        
        /* 每校验1MB打印一个点 */
        if((step & 0x3FFFF) == 0 && result.error_count == 0) {
            printf(".");
        }
    }
    
    result.test_passed = (result.error_count == 0);
    printf(" Verification %s!\r\n", result.test_passed ? "PASSED" : "FAILED");
    
    return result;
}