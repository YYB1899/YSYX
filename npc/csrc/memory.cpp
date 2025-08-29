#include "include/simulator.h"
#include "include/memory.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <elf.h>
#include <cstring>

uint8_t memory[CONFIG_MSIZE];  // 内存数组，按字节存储
long mem_size = CONFIG_MSIZE;  // 内存大小
uint32_t reg_file[32];         // 寄存器文件

void init_memory(const char* filename) {
    // 1. 初始化内存为0（而不是NOP指令）
    memset(memory, 0, CONFIG_MSIZE);

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

    // 按字节小端序写入到memory（memory是uint8_t数组）
    memory[addr + 0] = bytes[i];
    memory[addr + 1] = bytes[i + 1];
    memory[addr + 2] = bytes[i + 2];
    memory[addr + 3] = bytes[i + 3];
        
    // 调试输出（可留可去）
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

void load_elf_to_memory(const char* elf_file) {
    if (elf_file == NULL) return;
    
    // 打开 ELF 文件
    FILE *fp = fopen(elf_file, "rb");
    if (fp == NULL) {
        printf("Fail to open the elf file: %s\n", elf_file);
        return;
    }
    
    // 读取 ELF 文件头
    Elf32_Ehdr ehdr;
    if (fread(&ehdr, sizeof(Elf32_Ehdr), 1, fp) <= 0) {
        printf("Fail to read the elf_head.\n");
        fclose(fp);
        return;
    }
    
    // 检查是否是有效的 ELF 文件
    if (ehdr.e_ident[0] != 0x7f || ehdr.e_ident[1] != 'E' || 
        ehdr.e_ident[2] != 'L' || ehdr.e_ident[3] != 'F') {
        printf("The opened file isn't a elf file.\n");
        fclose(fp);
        return;
    }
    
    printf("Loading ELF file: %s\n", elf_file);
    printf("Entry point: 0x%08x\n", ehdr.e_entry);
    
    // 读取程序头表
    printf("Reading %d program headers from offset 0x%x\n", ehdr.e_phnum, ehdr.e_phoff);
    
    for (int i = 0; i < ehdr.e_phnum; i++) {
        // 确保每次都从正确的位置开始读取
        fseek(fp, ehdr.e_phoff + i * sizeof(Elf32_Phdr), SEEK_SET);
        
        Elf32_Phdr phdr;
        if (fread(&phdr, sizeof(Elf32_Phdr), 1, fp) <= 0) {
            printf("Fail to read program header %d.\n", i);
            fclose(fp);
            return;
        }
        
        printf("Program header %d: type=0x%x, vaddr=0x%08x, filesz=0x%08x, memsz=0x%08x, offset=0x%08x\n", 
               i, phdr.p_type, phdr.p_vaddr, phdr.p_filesz, phdr.p_memsz, phdr.p_offset);
        
        // 只加载LOAD类型的段 (PT_LOAD = 1)
        if (phdr.p_type == 1) {
            printf("Loading segment %d: vaddr=0x%08x, filesz=0x%08x, memsz=0x%08x\n", 
                   i, phdr.p_vaddr, phdr.p_filesz, phdr.p_memsz);
            
            // 检查地址范围
            if (phdr.p_vaddr < CONFIG_MBASE || 
                phdr.p_vaddr + phdr.p_memsz > CONFIG_MBASE + CONFIG_MSIZE) {
                printf("Warning: Segment %d address out of range\n", i);
                continue;
            }
            
            // 计算在内存数组中的偏移
            uint32_t mem_offset = phdr.p_vaddr - CONFIG_MBASE;
            
            // 先清零整个段（处理BSS段）
            memset(memory + mem_offset, 0, phdr.p_memsz);
            
            // 如果有文件数据，则加载
            if (phdr.p_filesz > 0) {
                fseek(fp, phdr.p_offset, SEEK_SET);
                
                // 先读取到临时缓冲区
                uint8_t* temp_buffer = new uint8_t[phdr.p_filesz];
                if (fread(temp_buffer, phdr.p_filesz, 1, fp) <= 0) {
                    printf("Fail to read segment %d data.\n", i);
                    delete[] temp_buffer;
                    fclose(fp);
                    return;
                }
                
                // 逐字节拷贝（保持文件中小端序字节逐一落到内存），避免丢失尾部不足4字节的数据
                for (uint32_t j = 0; j < phdr.p_filesz; j++) {
                    memory[mem_offset + j] = temp_buffer[j];
                }
                
                delete[] temp_buffer;
                
                // 调试输出：显示加载的数据
                printf("Loaded %d bytes to 0x%08x\n", phdr.p_filesz, phdr.p_vaddr);
                if (phdr.p_filesz >= 4) {
                    printf("First few bytes: ");
                    for (int j = 0; j < std::min(8, (int)phdr.p_filesz); j++) {
                        printf("%02x ", memory[mem_offset + j]);
                    }
                    printf("\n");
                }
            }
        }
    }
    
    fclose(fp);
    printf("ELF file loaded successfully.\n");
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
