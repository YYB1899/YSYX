#ifndef TRACE_H
#define TRACE_H

#include <cstdint>
#include <cstdio>

// 指令跟踪相关
#define INST_NUM 16

typedef struct {
    uint32_t pc;
    uint32_t inst;
} InstBuf;

// 函数跟踪相关
typedef struct {
    char name[64];
    uint32_t addr;
    uint32_t size;
} Symbol;

// 全局变量声明
extern InstBuf iringbuf[INST_NUM];
extern int cur_inst;
extern Symbol *symbol;
extern int func_num;

// 函数声明
void trace_inst(uint32_t pc, uint32_t inst);
void display_inst();
void parse_elf(const char *elf_file);
void display_call_func(uint32_t pc, uint32_t func_addr);
void display_ret_func(uint32_t pc);

// 配置宏
#define CONFIG_ITRACE 1
#define CONFIG_FTRACE 1

#endif // TRACE_H 