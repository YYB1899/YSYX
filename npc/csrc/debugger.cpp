#include "include/debugger.h"
#include "include/simulator.h"
#include <iostream>
#include <dlfcn.h>
#include <iomanip>

void init_capstone() {
    if (capstone_initialized) return;
    
    // 动态加载 Capstone 库
    void* dl_handle = dlopen("libcapstone.so.5", RTLD_LAZY); //动态加载库
    if (!dl_handle) { //缺少这部分代码,dl_handle缺少备用路径
        //fprintf(stderr, "Failed to load Capstone: %s\n", dlerror());
        //fprintf(stderr, "Trying default path...\n");
        dl_handle = dlopen("/home/yyb/ysyx-workbench/nemu/tools/capstone/repo/libcapstone.so.5", RTLD_LAZY);
        if (!dl_handle) {
            fprintf(stderr, "Failed again: %s\n", dlerror());
            return;
        }
    }
    // 动态获取 cs_open 等函数地址
    auto cs_open = (cs_err (*)(cs_arch, cs_mode, csh*))dlsym(dl_handle, "cs_open");
    auto cs_disasm = (size_t (*)(csh, const uint8_t*, size_t, uint64_t, size_t, cs_insn**))dlsym(dl_handle, "cs_disasm");
    auto cs_free = (void (*)(cs_insn*, size_t))dlsym(dl_handle, "cs_free");
    auto cs_close = (void (*)(csh))dlsym(dl_handle, "cs_close");
    
    // 初始化 Capstone - 使用数值常量确保兼容性
    // CS_ARCH_RISCV = 3, CS_MODE_RISCV32 = 0 (根据Capstone文档)
    cs_err err = cs_open((cs_arch)3, (cs_mode)0, &handle);
    if (err != CS_ERR_OK) {
        fprintf(stderr, "Failed to initialize Capstone: %d\n", err);
        return;
    }
    capstone_initialized = true;
}

void cmd_p(const std::string& args) {
    try {
        if (args.empty()) {
            printf("Registers:\n");
            for (int i = 0; i < 32; i++) {
                uint32_t value = get_reg_value(i);
                printf("x%02d: 0x%08x", i, value);
                if (i % 4 == 3) printf("\n");
                else printf("\t");
            }
        } else {
            int reg_num = std::stoi(args);
            if (reg_num < 0 || reg_num >= 32) {
                printf("Error: Register number must be 0-31\n");
                return;
            }
            uint32_t value = get_reg_value(reg_num);
            printf("x%02d: 0x%08x\n", reg_num, value);
        }
    } catch (...) {
        printf("Usage: p [reg_num]\n");
        printf("       p (without arguments to show all registers)\n");
    }
} 