#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vtop.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
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

// 函数声明
void sim_init(int argc, char** argv);
void sim_exit();
void step_and_dump_wave();
void display_state();
void cpu_exec(uint64_t cycles);
void run_to_completion();
void cmd_x(const std::string& args);
void log_memory_access();

#ifdef  DIFFTEST_ON

extern uint64_t *dut_reg;
extern uint64_t dut_pc;

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };
void (*ref_difftest_memcpy)(uint64_t addr, void *buf, size_t n, bool direction) = NULL;
void (*ref_difftest_regcpy)(void *dut, bool direction) = NULL;
void (*ref_difftest_exec)(uint64_t n) = NULL;
void (*ref_difftest_raise_intr)(uint64_t NO) = NULL;

void difftest_init(char *ref_so_file, long img_size) {
  assert(ref_so_file != NULL);

  void *handle;
  handle = dlopen(ref_so_file, RTLD_LAZY);
  assert(handle);

  ref_difftest_memcpy = (void (*)(uint64_t addr, void *buf, size_t n, bool direction))dlsym(handle , "difftest_memcpy");
  assert(ref_difftest_memcpy);

  ref_difftest_regcpy = (void (*)(void *dut, bool direction))dlsym(handle, "difftest_regcpy");
  assert(ref_difftest_regcpy);

  ref_difftest_exec = (void (*)(uint64_t n))dlsym(handle, "difftest_exec");
  assert(ref_difftest_exec);

  ref_difftest_raise_intr = (void (*)(uint64_t NO))dlsym(handle, "difftest_raise_intr");
  assert(ref_difftest_raise_intr);

  void (*ref_difftest_init)() = (void (*)())dlsym(handle, "difftest_init");
  assert(ref_difftest_init);

  ref_difftest_init();
  ref_difftest_memcpy(PMEM_START,guest_to_host(PMEM_START), img_size, DIFFTEST_TO_REF);

  regfile dut = pack_dut_regfile(dut_reg, INST_START);
  ref_difftest_regcpy(&dut, DIFFTEST_TO_REF);
}

bool difftest_check() {
  regfile ref,dut;
  ref_difftest_regcpy(&ref, DIFFTEST_TO_DUT);
  dut = pack_dut_regfile(dut_reg,dut_pc);
  return checkregs(&ref, &dut);
}

void difftest_step() {
  ref_difftest_exec(1);
}

#endif

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

 //DPI相关函数
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
        // 非调试模式：直接运行到程序结束
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
            #ifdef DIFFTEST_ON
    top->eval();                              // update rst_n_sync
    if(top->i_clk && rst_n_sync){
      //printf("pc = 0x%lx\n", dut_pc);
      if(dut_pc != 0 && dut_pc != 1){         // 0 means branch bubble, 1 means data bubble.
        if(!difftest_check()){                // check last cycle reg/mem.
          print_regs();
          break; 
        }
        difftest_step();                      // ref step and update regs/mem.
      }
    }
#endif
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
