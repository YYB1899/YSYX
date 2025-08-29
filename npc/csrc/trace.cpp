#include "include/trace.h"
#include "include/simulator.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <elf.h>

// 全局变量定义
InstBuf iringbuf[INST_NUM];
int cur_inst = 0;
Symbol *symbol = NULL;
int func_num = 0;

// 指令跟踪实现
void trace_inst(uint32_t pc, uint32_t inst) {
    iringbuf[cur_inst].pc = pc;
    iringbuf[cur_inst].inst = inst;
    cur_inst = (cur_inst + 1) % INST_NUM;
}

// 反汇编函数（使用Capstone）
void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte) {
    if (!capstone_initialized) {
        snprintf(str, size, "unknown");
        return;
    }
    
    cs_insn *insn;
    size_t count = cs_disasm(handle, code, nbyte, pc, 1, &insn);
    if (count > 0) {
        snprintf(str, size, "%s %s", insn[0].mnemonic, insn[0].op_str);
        cs_free(insn, count);
    } else {
        snprintf(str, size, "unknown");
    }
}

// 显示指令跟踪
void display_inst() {
    int end = cur_inst;
    char buf[128];
    char *p;
    int i = cur_inst;

    if (iringbuf[(i + 1) % INST_NUM].pc == 0) i = 0; // 环形缓冲区为空或未初始化
    
    do {
        p = buf;
        p += sprintf(buf, "%s0x%08x:  %08x\t", 
                    (i + 1) % INST_NUM == end ? "-->" : "   ", 
                    iringbuf[i].pc, iringbuf[i].inst);
        
        disassemble(p, buf + sizeof(buf) - p, iringbuf[i].pc, 
                   (uint8_t *)&iringbuf[i].inst, 4);
        printf("%s\n", buf);
        i = (i + 1) % INST_NUM;
    } while (i != end);
}

// 解析ELF文件获取符号表
void parse_elf(const char *elf_file) {
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
    
    // 查找字符串表
    fseek(fp, ehdr.e_shoff, SEEK_SET);
    Elf32_Shdr shdr;
    char *string_table = NULL;
    
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (fread(&shdr, sizeof(Elf32_Shdr), 1, fp) <= 0) {
            printf("Fail to read the shdr.\n");
            fclose(fp);
            return;
        }
        if (shdr.sh_type == SHT_STRTAB) {
            string_table = (char*)malloc(shdr.sh_size);
            fseek(fp, shdr.sh_offset, SEEK_SET);
            if (fread(string_table, shdr.sh_size, 1, fp) <= 0) {
                printf("Fail to read the strtab.\n");
                free(string_table);
                fclose(fp);
                return;
            }
        }
    }
    
    // 查找符号表
    fseek(fp, ehdr.e_shoff, SEEK_SET);
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (fread(&shdr, sizeof(Elf32_Shdr), 1, fp) <= 0) {
            printf("Fail to read the shdr.\n");
            fclose(fp);
            return;
        }
        if (shdr.sh_type == SHT_SYMTAB) {
            fseek(fp, shdr.sh_offset, SEEK_SET);
            Elf32_Sym sym;
            size_t sym_count = shdr.sh_size / shdr.sh_entsize;
            
            // 计算函数数量
            func_num = 0;
            for (size_t j = 0; j < sym_count; j++) {
                if (fread(&sym, sizeof(Elf32_Sym), 1, fp) <= 0) {
                    printf("Fail to read the sym.\n");
                    fclose(fp);
                    return;
                }
                if (ELF32_ST_TYPE(sym.st_info) == STT_FUNC) {
                    func_num++;
                }
            }
            
            // 分配符号表空间
            symbol = (Symbol*)malloc(func_num * sizeof(Symbol));
            if (symbol == NULL) {
                printf("Fail to allocate symbol table.\n");
                fclose(fp);
                return;
            }
            
            // 重新读取符号表
            fseek(fp, shdr.sh_offset, SEEK_SET);
            int func_idx = 0;
            for (size_t j = 0; j < sym_count; j++) {
                if (fread(&sym, sizeof(Elf32_Sym), 1, fp) <= 0) {
                    printf("Fail to read the sym.\n");
                    fclose(fp);
                    return;
                }
                if (ELF32_ST_TYPE(sym.st_info) == STT_FUNC) {
                    symbol[func_idx].addr = sym.st_value;
                    symbol[func_idx].size = sym.st_size;
                    if (string_table && sym.st_name < shdr.sh_size) {
                        strncpy(symbol[func_idx].name, 
                               string_table + sym.st_name, 63);
                        symbol[func_idx].name[63] = '\0';
                    } else {
                        strcpy(symbol[func_idx].name, "unknown");
                    }
                    func_idx++;
                }
            }
            break;
        }
    }
    
    if (string_table) free(string_table);
    fclose(fp);
}

// 查找函数名
const char* find_func_name(uint32_t addr) {
    if (symbol == NULL) return NULL;
    
    for (int i = 0; i < func_num; i++) {
        if (addr >= symbol[i].addr && 
            addr < symbol[i].addr + symbol[i].size) {
            return symbol[i].name;
        }
    }
    return NULL;
}

// 显示函数调用
void display_call_func(uint32_t pc, uint32_t func_addr) {
    const char* func_name = find_func_name(func_addr);
    if (func_name) {
        printf("Call function: %s at 0x%08x\n", func_name, func_addr);
    } else {
        printf("Call function: unknown at 0x%08x\n", func_addr);
    }
}

// 显示函数返回
void display_ret_func(uint32_t pc) {
    printf("Return from function at 0x%08x\n", pc);
} 