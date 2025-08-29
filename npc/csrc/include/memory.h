#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <cstdio>
#include <map>
#include <string>

// 使用simulator.h中的定义，避免重复定义
#ifndef CONFIG_MBASE
#define CONFIG_MBASE 0x80000000
#endif
#ifndef CONFIG_MSIZE
#define CONFIG_MSIZE 0x10000000  // 256MB
#endif

extern uint8_t memory[CONFIG_MSIZE];
extern long mem_size;
extern uint32_t reg_file[32];
extern std::map<uint32_t, uint32_t> mem_access_log;

// 函数声明
void init_memory(const char* filename);
void load_elf_to_memory(const char* elf_file);
void cmd_x(const std::string& args);
void log_memory_access();

#endif 

