#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vtop.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <map>

// 内存配置
#define CONFIG_MBASE 0x80000000
#define CONFIG_MSIZE 0x10000000  // 256MB

// 全局变量
VerilatedContext* contextp = nullptr;
VerilatedVcdC* tfp = nullptr;
static Vtop* top = nullptr;
vluint64_t main_time = 0;
bool debug_mode = false;

std::map<uint32_t, uint32_t> mem_access_log;

// 函数声明
void sim_init(int argc, char** argv);
void sim_exit();
void step_and_dump_wave();
void display_state();
void cpu_exec(uint64_t cycles);
void run_to_completion();
void cmd_x(const std::string& args);
void log_memory_access();

extern "C" void end_simulation() {
    printf("Simulation ended by ebreak instruction\n");
    if (contextp) contextp->gotFinish(true);
}

// ========== 内存相关函数 ==========

void log_memory_access() {
    // 通过分析RTL信号推测内存访问
    // 示例：如果PC变化但指令不变，可能是内存访问
    static uint32_t last_pc = 0;
    static uint32_t last_inst = 0;
    
    if (top->pc != last_pc && top->instruction == last_inst) {
        // 假设这是内存访问周期
        uint32_t addr = top->pc;  // 示例：用PC作为地址
        uint32_t data = top->instruction;
        mem_access_log[addr] = data;
    }
    
    last_pc = top->pc;
    last_inst = top->instruction;
}

void cmd_x(const std::string& args) {
    // 去除前后空格
    std::string trimmed_args = args;
    trimmed_args.erase(0, trimmed_args.find_first_not_of(" \t\n\r"));
    trimmed_args.erase(trimmed_args.find_last_not_of(" \t\n\r") + 1);

    // 分割参数
    size_t space_pos = trimmed_args.find(' ');
    if (space_pos == std::string::npos) {
        printf("Usage: x [len] [addr] (e.g. 'x 4 0x80000000')\n");
        return;
    }

    try {
        // 提取长度和地址
        int len = std::stoi(trimmed_args.substr(0, space_pos));
        if (len <= 0) {
            printf("Length must be positive\n");
            return;
        }

        // 提取地址部分并去除可能的前导空格
        std::string addr_str = trimmed_args.substr(space_pos + 1);
        addr_str.erase(0, addr_str.find_first_not_of(" \t\n\r"));
        
        uint32_t addr = std::stoul(addr_str, nullptr, 16);
        
        printf("Scanning memory from 0x%08x, len=%d:\n", addr, len);
        for (int i = 0; i < len; i++) {
            uint32_t curr_addr = addr + i * 4;
            auto it = mem_access_log.find(curr_addr);
            printf("0x%08x: 0x%08x %s\n", 
                curr_addr,
                it != mem_access_log.end() ? it->second : 0,
                it != mem_access_log.end() ? "(accessed)" : "");
        }
    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        printf("Usage: x [len] [addr] (e.g. 'x 4 0x80000000')\n");
    }
}

// ========== 仿真控制函数 ==========

void run_to_completion() {
    printf("Running to completion...\n");
    while (!contextp->gotFinish() && main_time < 100000) {
        top->clk = 0; step_and_dump_wave();
        top->clk = 1; step_and_dump_wave();
        if (contextp->gotFinish()) break;
    }
}

void cpu_exec(uint64_t cycles) {
    for (uint64_t i = 0; i < cycles; i++) {
        top->clk = 0; step_and_dump_wave();
        top->clk = 1; step_and_dump_wave();
        log_memory_access();  // 记录内存访问
        display_state();
    }
}

// ========== 核心函数 ==========

int main(int argc, char** argv) {
    // 检查调试模式（通过--debug参数）
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            debug_mode = true;
            printf("Debug mode enabled\n");
            break;
        }
    }

    // 初始化仿真环境和影子内存
    sim_init(argc, argv);
    //init_shadow_mem();

    if (!debug_mode) {
        // 非调试模式：直接运行到程序结束
        printf("Starting simulation in batch mode...\n");
        run_to_completion();
    } else {
        // 调试模式：交互式命令行
        printf("Debugging commands available:\n");
        printf("  s[n] - Step n instructions (default 1)\n");
        printf("  c    - Continue to completion\n");
        printf("  x len addr - Examine memory (e.g. 'x 4 0x80000000')\n");
        printf("  q    - Quit simulation\n");

        while (!contextp->gotFinish() && main_time < 100000) {
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

void sim_init(int argc, char** argv) {
    contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    tfp = new VerilatedVcdC;
    top = new Vtop{contextp};
    
    // 初始化波形跟踪
    contextp->traceEverOn(true);
    top->trace(tfp, 99);
    tfp->open("wave.vcd");

    // 复位序列
    top->rst = 1; top->clk = 0; step_and_dump_wave();
    top->clk = 1; step_and_dump_wave();
    top->rst = 0; top->clk = 0; step_and_dump_wave();
}

void sim_exit() {
    if (tfp) tfp->close();
    if (top) delete top;
    if (contextp) delete contextp;
}

void step_and_dump_wave() {
    top->eval();
    contextp->timeInc(1);
    tfp->dump(contextp->time());
    main_time++;
}

void display_state() {
    if (top->clk == 1) {  // 只在上升沿显示
        printf("PC: 0x%08x  Instruction: 0x%08x\n", top->pc, top->instruction);
    }
}
