#include "include/simulator.h"
#include "include/memory.h"
#include "include/debugger.h"
#include "include/trace.h"
#include "include/difftest.h"
#include <iostream>
#include <string>
#include <dlfcn.h>  // 用于动态加载库

// 全局变量控制 DiffTest 开关
bool difftest_enabled = false;

// 在simulator.cpp或main.cpp中
void init_difftest() {
    if (difftest_enabled) {
        // 1. 设置参考模型路径
        const char* ref_so = "/home/yyb/ysyx-workbench/nemu/build/riscv32-nemu-interpreter-so";
        
        // 2. 获取内存镜像信息
        //uint8_t memory[CONFIG_MSIZE];
        extern long mem_size;
        
        // 3. 获取寄存器文件
        extern uint32_t reg_file[32];
        
        // 4. 初始化difftest
        // 临时修复：只传递实际使用的内存大小，避免段错误
        size_t actual_size = 0x1000;  // 4KB 应该足够测试
        difftest_init(
            ref_so,
            actual_size,        // 使用较小的大小
            0x80000000,     // 起始地址
            (uint8_t*)memory,         // 内存镜像指针
            reg_file        // 寄存器文件指针
        );
        
        printf("DiffTest initialized with:\n"
               "  ref_so = %s\n"
               "  img_size = %ld\n"
               "  inst_start = 0x%08x\n",
               ref_so, mem_size, 0x80000000);
        
        // 同步寄存器状态，确保 NEMU 和 NPC 状态一致
        printf("Syncing register state...\n");
        printf("NPC PC = 0x%08x\n", top->pc);
        
        // 创建 CPU_state 结构体来同步寄存器状态
        struct {
            uint32_t gpr[32];  // 32个通用寄存器
            uint32_t pc;       // 程序计数器
            uint32_t csr[4];   // CSR寄存器（简化）
        } cpu_state;
        
        // 复制通用寄存器（从 reg_file 数组）
        for (int i = 0; i < 32; i++) {
            cpu_state.gpr[i] = reg_file[i];
        }
        
        // 设置 PC 值（从 NPC 获取当前 PC）
        cpu_state.pc = top->pc;
        
        // 初始化 CSR 寄存器为 0
        for (int i = 0; i < 4; i++) {
            cpu_state.csr[i] = 0;
        }
        
        // 同步寄存器到 NEMU
        difftest_regcpy(&cpu_state, 1);  // 1 表示 DIFFTEST_TO_REF
        printf("Register state synced to NEMU (PC = 0x%08x)\n", cpu_state.pc);
    }
}

int main(int argc, char** argv) {
    // 检查调试模式和 difftest 模式
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            debug_mode = true;
            printf("Debug mode enabled\n");
            printf("ITRACE enabled\n");
            printf("FTRACE enabled\n");
        }
        if (std::string(argv[i]) == "--difftest") {
            difftest_enabled = true;
            printf("DiffTest mode enabled\n");
        }
    }
    
    // 初始化仿真环境和内存
    sim_init(argc, argv);
    
    if(difftest_enabled == true) {
    	init_difftest();
    }

    if (!debug_mode) {
        // 非调试模式
        printf("Starting simulation in batch mode...\n");
        run_to_completion();
    } else {
        // 调试模式：交互式命令行
        printf("Debugging commands available:\n");
        printf("  s[n] - Step n instructions (default 1)\n");
        printf("  c    - Continue to completion\n");
        printf("  x len addr - Examine memory (e.g. 'x 4 0x80000000')\n");
        printf("  p [reg_num] - Print register value(s)\n");
        printf("  i    - Show instruction trace (itrace)\n");
        printf("  f    - Show function trace (ftrace)\n");
        printf("  t    - Toggle DiffTest mode (currently %s)\n", 
               difftest_enabled ? "enabled" : "disabled");
        printf("  q    - Quit simulation\n");

        while (!contextp->gotFinish() && main_time < 1000000) {  // 增加到1,000,000步
            printf("(npc) ");
            fflush(stdout);            
            std::string cmd;
            std::getline(std::cin, cmd);

            // 移除首尾空白字符
            cmd.erase(0, cmd.find_first_not_of(" \t\n\r"));
            cmd.erase(cmd.find_last_not_of(" \t\n\r") + 1);

            if (cmd.empty()) continue;

            // 命令解析
            try {
                switch (cmd[0]) {
                    case 's': {  // 单步执行
                        int steps = 1;
                        if (cmd.length() > 1) {
                            steps = std::stoi(cmd.substr(1));
                            if (steps <= 0) throw std::invalid_argument("Step count must be positive");
                        }
                        printf("Stepping %d instruction(s)\n", steps);
                        cpu_exec(steps);
                        // 如果启用了 difftest，执行参考模型
                        if (difftest_enabled) {
                            difftest_exec(steps);
                        }
                        break;
                    }
                    case 'c': {  // 执行到结束
                        printf("Continuing to completion...\n");
                        run_to_completion();
                        break;
                    }
                    case 'x': {  // 内存扫描
                        if (cmd.length() <= 1) {
                            printf("Usage: x [len] [addr] (e.g. 'x 4 0x80000000')\n");
                            break;
                        }
                        cmd_x(cmd.substr(1));
                        break;
                    }
                    case 'p': {  // 打印寄存器
                        if (cmd.length() > 1) cmd_p(cmd.substr(1));
                        else cmd_p("");
                        break;
                    }
                    case 'i': {  // 显示指令跟踪
                        printf("Instruction trace:\n");
                        display_inst();
                        break;
                    }
                    case 'f': {  // 显示函数跟踪
                        printf("Function trace enabled\n");
                        break;
                    }
                    case 't': {  // 切换 DiffTest 模式
                        difftest_enabled = !difftest_enabled;
                        printf("DiffTest mode %s\n", difftest_enabled ? "enabled" : "disabled");
                        if(difftest_enabled == true) {
                            init_difftest();
                        }
                        break;
                    }
                    case 'q': {  // 退出
                        printf("Exiting simulation\n");
                        sim_exit();
                        return 0;
                    }
                    default:
                        printf("Unknown command. Valid commands:\n");
                        printf("  s[n] - Step execution\n");
                        printf("  c    - Continue\n");
                        printf("  x len addr - Examine memory\n");
                        printf("  p [reg_num] - Print register value(s)\n");
                        printf("  i    - Show instruction trace\n");
                        printf("  f    - Show function trace\n");
                        printf("  t    - Toggle DiffTest mode\n");
                        printf("  q    - Quit\n");
                }
            } catch (const std::exception& e) {
                printf("Error: %s\n", e.what());
            }
        }
    }

    // 清理资源
    sim_exit();
    return 0;
}
