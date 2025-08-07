#include "include/memory.h"
#include "include/simulator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>

uint32_t memory[CONFIG_MSIZE/4];  // 内存数组，按32位字存储
long mem_size = CONFIG_MSIZE;  // 内存大小
uint32_t reg_file[32];         // 寄存器文件

void init_memory(const char* filename) {
    // 1. 初始化内存为NOP指令 (0x00000013)
    for (uint32_t i = 0; i < CONFIG_MSIZE/4; i++) {
        memory[i] = 0x00000013;
    }

    // 2. 读取指令文件（按空格分隔的连续字节）
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open instruction file: " << filename << std::endl;
        exit(1);
    }

    // 3. 读取所有字节（按空格分隔）
    std::vector<uint8_t> bytes;
    std::string all_bytes;
    std::getline(file, all_bytes, '\0'); // 读取整个文件
    
    std::istringstream iss(all_bytes);
    std::string byte_str;
    while (iss >> byte_str) {
        try {
            uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
            bytes.push_back(byte);
        } catch (...) {
            std::cerr << "Error: Invalid hex byte in file: " << byte_str << std::endl;
            exit(1);
        }
    }

    // 4. 组合为32位指令（小端序）
    uint32_t addr = 0;
    for (size_t i = 0; i + 3 < bytes.size(); i += 4) {
        if (addr >= CONFIG_MSIZE) {
            std::cerr << "Error: Program too large for memory" << std::endl;
            exit(1);
        }
        
        // 小端序组合：bytes[i]是最低字节
        uint32_t word = (bytes[i] << 0)   | (bytes[i+1] << 8) | 
                       (bytes[i+2] << 16) | (bytes[i+3] << 24);
        memory[addr/4] = word;
        
        // 调试输出
        std::cout << "mem[0x" << std::hex << std::setw(8) << std::setfill('0') 
                 << (CONFIG_MBASE + addr) << "] = 0x"
                 << std::setw(2) << static_cast<int>(bytes[i]) << " "
                 << std::setw(2) << static_cast<int>(bytes[i+1]) << " "
                 << std::setw(2) << static_cast<int>(bytes[i+2]) << " "
                 << std::setw(2) << static_cast<int>(bytes[i+3]) << " -> 0x"
                 << std::setw(8) << word << std::endl;
        
        addr += 4;
    }
}
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
        
        printf("Memory contents from 0x%08x, length %d words:\n", addr, len);
        for (int i = 0; i < len; i++) {
            uint32_t curr_addr = addr + i * 4;
            uint32_t value = pmem_read(curr_addr);  // 使用 pmem_read 读取内存
            printf("0x%08x: 0x%08x\n", curr_addr, value);
        }
    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        printf("Usage: x [len] [addr] (e.g. 'x 4 0x80000000')\n");
    }
} 
