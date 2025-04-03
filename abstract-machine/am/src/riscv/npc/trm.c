#include <am.h>
#include <klib-macros.h>

extern char _heap_start;
int main(const char *args);

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)

Area heap = RANGE(&_heap_start, PMEM_END);
static const char mainargs[MAINARGS_MAX_LEN] = MAINARGS_PLACEHOLDER; // defined in CFLAGS

void putch(char ch) {
}

void halt(int code) {
      // 将退出码写入特定内存地址（用于调试）
    volatile uint32_t *exit_code = (volatile uint32_t *)0x100000;
    *exit_code = code;

    // 内联汇编插入 ebreak
    asm volatile("ebreak");

    // 防止继续执行（冗余保护）
    while(1);
}

void _trm_init() {
  int ret = main(mainargs);
  halt(ret);
}
