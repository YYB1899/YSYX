#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <dlfcn.h>

#include "include/simulator.h"

// DiffTest 相关全局变量
static void (*ref_difftest_memcpy)(uint32_t addr, void *buf, size_t n, bool direction) = NULL;
static void (*ref_difftest_regcpy)(void *dut, bool direction) = NULL;
static void (*ref_difftest_exec)(uint32_t n) = NULL;
static void (*ref_difftest_raise_intr)(uint32_t NO) = NULL;
static void (*ref_difftest_init)() = NULL;

static int DIFFTEST_TO_DUT = 0;
static int DIFFTEST_TO_REF = 1;

extern "C" {
void difftest_init(const char *ref_so_file, long img_size, uint32_t inst_start, void *img_buf, void *regfile_buf) {
    assert(ref_so_file != NULL);
    
    void *handle = dlopen(ref_so_file, RTLD_NOW);
    if (!handle) {
        printf("dlopen failed: %s\n", dlerror());
        exit(1);
    }
    ref_difftest_memcpy = (void (*)(uint32_t, void*, size_t, bool))dlsym(handle, "difftest_memcpy");
    assert(ref_difftest_memcpy);
    ref_difftest_regcpy = (void (*)(void*, bool))dlsym(handle, "difftest_regcpy");
    assert(ref_difftest_regcpy);
    ref_difftest_exec = (void (*)(uint32_t))dlsym(handle, "difftest_exec");
    assert(ref_difftest_exec);
    ref_difftest_raise_intr = (void (*)(uint32_t))dlsym(handle, "difftest_raise_intr");
    assert(ref_difftest_raise_intr);
    ref_difftest_init = (void (*)())dlsym(handle, "difftest_init");
    assert(ref_difftest_init);
    ref_difftest_init();
    ref_difftest_memcpy(inst_start, img_buf, img_size, DIFFTEST_TO_REF);
    ref_difftest_regcpy(regfile_buf, DIFFTEST_TO_REF);
}

void difftest_memcpy(uint32_t addr, void *buf, size_t n, bool direction) {
    assert(ref_difftest_memcpy);
    ref_difftest_memcpy(addr, buf, n, direction);
}

void difftest_regcpy(void *dut, bool direction) {
    assert(ref_difftest_regcpy);
    ref_difftest_regcpy(dut, direction);
}

void difftest_exec(uint32_t n) {
    assert(ref_difftest_exec);
    
    // 执行NEMU的n条指令
    ref_difftest_exec(n);
    
    // 从NEMU获取状态
    cpu_state_t ref_state;
    ref_difftest_regcpy(&ref_state, DIFFTEST_TO_DUT);
    
    // 对比PC
    if (cpu_state.pc != ref_state.pc) {
        printf("DiffTest FAILED: PC mismatch!\n");
        printf("  NPC PC = 0x%08x\n", cpu_state.pc);
        printf("  NEMU PC = 0x%08x\n", ref_state.pc);
        exit(1);
    }
    
    // 对比通用寄存器
    for (int i = 0; i < 32; i++) {
        if (cpu_state.gpr[i] != ref_state.gpr[i]) {
            printf("DiffTest FAILED: Register x%d mismatch!\n", i);
            printf("  NPC x%d = 0x%08x\n", i, cpu_state.gpr[i]);
            printf("  NEMU x%d = 0x%08x\n", i, ref_state.gpr[i]);
            exit(1);
        }
    }
    
    // 只在有错误时输出，正常情况下不输出PASSED信息
}

void difftest_raise_intr(uint32_t NO) {
    assert(ref_difftest_raise_intr);
    ref_difftest_raise_intr(NO);
}
} // extern "C" 