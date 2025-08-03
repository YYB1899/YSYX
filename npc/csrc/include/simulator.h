#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vtop.h"
#include <iostream>
#include <map>
#include <cstdint>
#include <svdpi.h>
#include <capstone/capstone.h>

// 内存配置
#define CONFIG_MBASE 0x80000000
#define CONFIG_MSIZE 0x10000000  // 256MB

// 全局变量声明
extern VerilatedContext* contextp;
extern VerilatedVcdC* tfp;
extern Vtop* top;
extern vluint64_t main_time;
extern bool debug_mode;
extern std::map<uint32_t, uint32_t> mem_access_log;
extern csh handle;
extern bool capstone_initialized;
extern std::map<uint32_t, uint32_t> memory;

// 函数声明
void sim_init(int argc, char** argv);
void sim_exit();
void step_and_dump_wave();
void display_state();
void cpu_exec(uint64_t cycles);
void run_to_completion();

// DPI函数声明
extern "C" {
    int pmem_read(int raddr);
    void pmem_write(int waddr, int wdata, char wmask);
    void end_simulation();
    int get_reg_value(int reg_num);
    void register_file_scope();
}

#endif // SIMULATOR_H 