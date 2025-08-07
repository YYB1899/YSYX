#ifndef MEMORY_H
#define MEMORY_H

#include <string>
#include <map>
#include <cstdint>
#include "simulator.h"

extern uint32_t memory[CONFIG_MSIZE/4];

// 内存管理函数声明
void init_memory(const char* filename);
void log_memory_access();

// 内存访问命令函数
void cmd_x(const std::string& args);

#endif // MEMORY_H 

