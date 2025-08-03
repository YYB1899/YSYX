#include "include/simulator.h"
#include "include/memory.h"
#include "include/debugger.h"
#include "include/trace.h"
#include <iostream>
#include <dlfcn.h>
#include <iomanip>

// 全局变量定义
VerilatedContext* contextp = nullptr;
VerilatedVcdC* tfp = nullptr;
Vtop* top = nullptr;
vluint64_t main_time = 0;
bool debug_mode = false;
std::map<uint32_t, uint32_t> mem_access_log;
csh handle;
bool capstone_initialized = false;
std::map<uint32_t, uint32_t> memory;

// DPI函数实现
extern "C" int pmem_read(int raddr) {
    uint32_t aligned_addr = raddr & ~0x3u;
    return memory[aligned_addr / 4];
}

extern "C" void pmem_write(int waddr, int wdata, char wmask) {
    uint32_t aligned_addr = waddr & ~0x3u;
    memory[aligned_addr] = wdata;
    // 完整写入（SW指令）
    if (wmask == 0xffffffd4) {
        memory[aligned_addr] = wdata;
        return;
    }else {
    // 部分写入（SB/SH指令）
    uint32_t current = pmem_read(aligned_addr);
    uint32_t new_data = current;
    
    for (int i = 0; i < 4; i++) {
        if (wmask & (1 << i)) {
            new_data = (new_data & ~(0xFF << (i*8))) | 
                       (wdata & (0xFF << (i*8)));
        }
    }
    memory[aligned_addr] = new_data;
    }
}

extern "C" void end_simulation() {
    printf("Simulation ended by ebreak instruction\n");
    if (contextp) contextp->gotFinish(true);
}

extern "C" void register_file_scope() {
    // 空实现，用来设置上下文
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
  
  // 初始化内存
  init_memory("build/inst.hex");  // 确保路径正确
  
  // 初始化 Capstone
  init_capstone();
  
  // 初始化 trace 功能
  #ifdef CONFIG_ITRACE
  printf("ITRACE enabled\n");
  #endif
  
  #ifdef CONFIG_FTRACE
  printf("FTRACE enabled\n");
  // 解析ELF文件获取符号表
  if (argc > 1) {
    parse_elf(argv[1]);
  }
  #endif

    // 复位序列 - 产生一个有效的复位脉冲
    top->rst = 1;  // 复位信号置高
    top->clk = 0; step_and_dump_wave();  // 第一个半周期
    top->clk = 1; step_and_dump_wave();  // 第一个完整周期
    top->clk = 0; step_and_dump_wave();  // 第二个半周期
    top->clk = 1; step_and_dump_wave();  // 第二个完整周期
    top->clk = 0; step_and_dump_wave();  // 第三个半周期
    top->rst = 0;  // 释放复位信号
    top->clk = 1; step_and_dump_wave();  // 第三个完整周期（复位已释放）
    
    // 调试打印初始内存状态
    printf("\n=== Memory Initialization Check ===\n");
    for (int i = 0; i < 4; i++) {
        uint32_t addr = i * 4;
        printf("mem[0x%08x] = 0x%08x\n", 
               CONFIG_MBASE + addr, pmem_read(addr));
    }
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
        // ITRACE: 记录指令到环形缓冲区
        #ifdef CONFIG_ITRACE
        trace_inst(top->pc, top->instruction);
        #endif
        
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
        
        // FTRACE: 检测函数调用和返回
        #ifdef CONFIG_FTRACE
        // 检测JAL指令（函数调用）
        if ((top->instruction & 0x7f) == 0x6f) { // JAL指令
            uint32_t rd = (top->instruction >> 7) & 0x1f;
            uint32_t imm = ((top->instruction >> 31) & 1) << 20 |
                          ((top->instruction >> 12) & 0xff) << 12 |
                          ((top->instruction >> 20) & 1) << 11 |
                          ((top->instruction >> 21) & 0x3ff) << 1;
            uint32_t target = top->pc + imm;
            
            if (rd == 1) { // ra寄存器，通常是函数调用
                display_call_func(top->pc, target);
            }
        }
        // 检测JALR指令（函数返回）
        else if ((top->instruction & 0x7f) == 0x67) { // JALR指令
            uint32_t rd = (top->instruction >> 7) & 0x1f;
            uint32_t rs1 = (top->instruction >> 15) & 0x1f;
            uint32_t imm = (top->instruction >> 20) & 0xfff;
            
            if (rd == 0 && rs1 == 1) { // rd=x0, rs1=ra，通常是函数返回
                display_ret_func(top->pc);
            }
        }
        #endif
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

void run_to_completion() {
    printf("Running to completion...\n");
    while (!contextp->gotFinish() && main_time < 100000) {
        top->clk = 0; step_and_dump_wave();
        top->clk = 1; step_and_dump_wave();
        if (contextp->gotFinish()) break;
    }
} 