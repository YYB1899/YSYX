#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vtop.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <cstdint>
#include <fstream>
#include <map>
#include <svdpi.h>
#include "/home/yyb/ysyx-workbench/nemu/tools/capstone/repo/include/capstone/capstone.h"
#include <dlfcn.h> 
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
static csh handle;
static bool capstone_initialized = false;
static std::map<uint32_t, uint32_t> memory;

// 函数声明
void sim_init(int argc, char** argv);
void sim_exit();
void step_and_dump_wave();
void display_state();
void cpu_exec(uint64_t cycles);
void run_to_completion();
void cmd_x(const std::string& args);
void log_memory_access();
 //DPI相关函数
extern "C" void load_program(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Failed to open " << filename << std::endl;
        exit(1);
    }

    uint32_t addr = 0;
    uint32_t word;
    while (file.read(reinterpret_cast<char*>(&word), sizeof(word))) {
        memory[addr] = word;
        addr += 4;
    }
    std::cout << "Loaded " << addr << " bytes into memory" << std::endl;
}

// 内存读取函数
extern "C" int pmem_read(int raddr) {
    uint32_t aligned_addr = static_cast<uint32_t>(raddr) & ~0x3u;
    auto it = memory.find(aligned_addr);
    if (it != memory.end()) {
        return it->second;
    }
    std::cerr << "Warning: Read from uninitialized address 0x" 
              << std::hex << aligned_addr << std::endl;
    return 0;
}

extern "C" void pmem_write(int waddr, int wdata, char wmask) {
    uint32_t aligned_addr = waddr & ~0x3u;
    uint32_t current = pmem_read(aligned_addr);
    uint32_t new_data = current;
    
    // 按字节掩码更新数据
    for (int i = 0; i < 4; i++) {
        if (wmask & (1 << i)) {
            new_data &= ~(0xFF << (i*8));       // 清除目标字节
            new_data |= (wdata & (0xFF << (i*8))); // 设置新字节
        }
    }
    
    memory[aligned_addr] = new_data;
}
extern "C" void end_simulation() {
    printf("Simulation ended by ebreak instruction\n");
    if (contextp) contextp->gotFinish(true);
}

extern "C" {
    int get_reg_value(int reg_num);
    void register_file_scope();
}

extern "C" void register_file_scope() {
    // 空实现，用来设置上下文
}

// 初始化 Capstone
void init_capstone() {
    if (capstone_initialized) return;
    
    // 动态加载 Capstone 库
    void* dl_handle = dlopen("libcapstone.so.5", RTLD_LAZY); //动态加载库
    if (!dl_handle) { //缺少这部分代码,dl_handle缺少备用路径
        fprintf(stderr, "Failed to load Capstone: %s\n", dlerror());
        fprintf(stderr, "Trying default path...\n");
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
    // 初始化 Capstone
    cs_err err = cs_open(CS_ARCH_RISCV, CS_MODE_RISCV32, &handle); //设置 RISC-V 32位模式
    if (err != CS_ERR_OK) {
        fprintf(stderr, "Failed to initialize Capstone: %d\n", err);
        return;
    }
    capstone_initialized = true;
}


// 内存相关函数
void log_memory_access() {
    uint32_t op = top->instruction;
    mem_access_log[top->pc] = top->instruction;
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
                it != mem_access_log.end() ? "(accessed)" : "(failure)");
        }
    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        printf("Usage: x [len] [addr] (e.g. 'x 4 0x80000000')\n");
    }
}

// 打印寄存器函数
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

//仿真控制函数
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

// 核心函数 
int main(int argc, char** argv) {
    // 检查调试模式（通过--debug参数）
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            debug_mode = true;
            printf("Debug mode enabled\n");
            break;
        }
    }
    // 初始化仿真环境和内存
    sim_init(argc, argv);
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
                    case 'p': {  // 打印寄存器
    			if (cmd.length() > 1) cmd_p(cmd.substr(1));
    			else cmd_p("");
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
    register_file_scope();
    contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    tfp = new VerilatedVcdC;
    top = new Vtop{contextp};
    
    // 初始化波形跟踪
    contextp->traceEverOn(true);
    top->trace(tfp, 99);
    tfp->open("wave.vcd");

    // 初始化 Capstone
    init_capstone();

    // 复位序列
    top->rst = 1; top->clk = 0; step_and_dump_wave();
    top->clk = 1; step_and_dump_wave();
    top->rst = 0; top->clk = 0; step_and_dump_wave();
}

void sim_exit() {
    if (capstone_initialized) {
        cs_close(&handle);
    }
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
    if (top->clk == 1) {
        // 反汇编指令
        char disasm_str[256] = {0};
        if (capstone_initialized) {
            uint8_t code[4];
            // 将32位指令转换为字节数组,小端模式转换
            code[0] = (top->instruction >> 0) & 0xFF;
            code[1] = (top->instruction >> 8) & 0xFF;
            code[2] = (top->instruction >> 16) & 0xFF;
            code[3] = (top->instruction >> 24) & 0xFF;
            
            cs_insn *insn;
            size_t count = cs_disasm(handle, code, sizeof(code), top->pc, 1, &insn); //反汇编一条指令
            if (count > 0) {
                snprintf(disasm_str, sizeof(disasm_str), "%s %s", insn[0].mnemonic, insn[0].op_str); //格式化输出
                cs_free(insn, count); //资源释放
            } else {
                strcpy(disasm_str, "(unknown)");
            }
        }
        
        printf("PC: 0x%08x  Instruction: 0x%08x  %s\n", 
               top->pc, top->instruction, disasm_str);
    }
}
