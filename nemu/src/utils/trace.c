#include <common.h>
#include <elf.h>
#include <device/map.h>
#include "trace.h"

InstBuf iringbuf[INST_NUM];
int cur_inst = 0;
int func_num = 0;
//IRINGBUF
void trace_inst(word_t pc, uint32_t inst){
	iringbuf[cur_inst].pc = pc;
	iringbuf[cur_inst].inst = inst;
	cur_inst = (cur_inst + 1) % INST_NUM;
} 
/*
void display_inst(){
    int end = cur_inst;//16
    char buf[128];
    char *p;
    int i = cur_inst;//16

    if(iringbuf[i+1].pc == 0) i = 0;//环形缓冲区为空或未初始化
    do{
        p = buf;
        p += sprintf(buf, "%s" FMT_WORD ":  %08x\t", (i + 1) % INST_NUM == end ? "-->" : "   ", iringbuf[i].pc, iringbuf[i].inst);//格式化 标识,PC 值和指令编码为字符串,并且添加到buf
        void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
        disassemble(p, buf + sizeof(buf) - p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);//二进制编码反汇编为可读的汇编指令,并且添加到buf
        puts(buf);//输出buf
        i = (i + 1) % INST_NUM;
    } while (i != end);
}
*/
//MTRACE
void display_memory_read(paddr_t addr, int len){
	printf("Read memory: " FMT_PADDR ", the len is %d\n.", addr, len);
}

void display_memory_write(paddr_t addr, int len, word_t data){
	printf("Wirte memory: " FMT_PADDR", the len is %d, the written data is " FMT_WORD".\n", addr, len, data);
}
//FTRACE
typedef struct {
	char name[64];
	paddr_t addr;
	Elf32_Xword size;
}Symbol;

Symbol *symbol = NULL;

void parse_elf(const char *elf_file){
	if(elf_file == NULL) return;
	//打开 ELF 文件
	FILE *fp;
	fp = fopen(elf_file, "rb");//以只读二进制模式打开文件
	if(fp == NULL){//打开 ELF 文件是否成功
		printf("Fail to open the elf file.\n");
		exit(0);
	}
	//读取 ELF 文件头
	Elf32_Ehdr edhr;//e_ident标识符数组;e_shoff节头表的起始位置偏移量;e_shnum节头表中条目的数量
	if(fread(&edhr, sizeof(Elf32_Ehdr), 1, fp) <= 0){//读取 ELF 文件头是否成功
		printf("Fail to read the elf_head.\n");
		exit(0);
	}
	if(edhr.e_ident[0] != 0x7f || edhr.e_ident[1] != 'E' || edhr.e_ident[2] != 'L' || edhr.e_ident[3] != 'F' ){//文件是否是有效的 ELF 文件
		printf("The opened file isn't a elf file.\n");
		exit(0);
	}
	//查找字符串表(线性结构)
	fseek(fp, edhr.e_shoff, SEEK_SET);
	Elf32_Shdr shdr;//sh_type表示节的类型;sh_size表示节的大小;sh_offset符号表的起始位置偏移量
	char *string_table = NULL;
	for(int i = 0; i < edhr.e_shnum; i ++){
		if(fread(&shdr, sizeof(Elf32_Shdr), 1, fp) <= 0){//读取节头表项
			printf("Fail to read the shdr.\n");
			exit(0);
		}
		if(shdr.sh_type == SHT_STRTAB){//查找字符串表
			string_table = (char*)malloc(shdr.sh_size);
			fseek(fp, shdr.sh_offset, SEEK_SET);
			if(fread(string_table, shdr.sh_size, 1, fp) <= 0){
				printf("Fail to read the strtab.\n");
				exit(0);
			}
		}
	}
	//查找符号表
	size_t func_num = 0;
	fseek(fp, edhr.e_shoff, SEEK_SET);
	for(int i = 0; i < edhr.e_shnum; i ++){
		if(fread(&shdr, sizeof(Elf32_Shdr), 1, fp) <= 0){//读取节头表项
			printf("Fail to read the shdr.\n");
			exit(0);
		}
		if(shdr.sh_type == SHT_SYMTAB){
			fseek(fp, shdr.sh_offset, SEEK_SET);
			Elf32_Sym sym;
			size_t sym_count = shdr.sh_size / shdr.sh_entsize;//表中符号的数量
			symbol = (Symbol*)malloc(sizeof(Symbol) * sym_count);
			for(size_t j = 0; j < sym_count; j ++){//遍历符号表中的所有符号
				if(fread(&sym, sizeof(Elf32_Sym), 1, fp) <= 0){
					printf("Fail to read the symtab.\n");
					exit(0);
				}
				if(ELF32_ST_TYPE(sym.st_info) == STT_FUNC){//符号是否为函数符号
					const char *name = string_table + sym.st_name;//字符串表中获取函数名称
					strncpy(symbol[func_num].name, name, sizeof(symbol[func_num].name) - 1);//函数名称存储到 Symbol 结构体数组中
					symbol[func_num].addr = sym.st_value;//地址存储到 Symbol 结构体数组中
					symbol[func_num].size = sym.st_size;//大小存储到 Symbol 结构体数组中
					func_num ++;
				}
			}
		}
	}
	//关闭文件和释放内存
	fclose(fp);
	free(string_table);
	free(symbol);
}

int rec_depth = 1;//递归深度
void display_call_func(word_t pc, word_t func_addr){//显示函数调用的信息,pc:调用指令的地址,func_addr:调用函数的地址
    int i = 0;
    for(i = 0; i < func_num; i++){//查找函数符号
        if(func_addr >= symbol[i].addr && func_addr < (symbol[i].addr + symbol[i].size)){
            break;
        }
    }
    printf("0x%08x:", pc);
    for(int k = 0; k < rec_depth; k++) printf("  ");
    rec_depth++;
    printf("call  [%s@0x%08x]\n", symbol[i].name, func_addr);
}

void display_ret_func(word_t pc){//显示函数返回的信息,pc:调用指令的地址
    int i = 0;
    for(i = 0; i < func_num; i++){//查找函数符号
        if(pc >= symbol[i].addr && pc < (symbol[i].addr + symbol[i].size)){
            break;
        }
    }
    printf("0x%08x:", pc);
    rec_depth--;
    for(int k = 0; k < rec_depth; k++) printf("  ");
    printf("ret  [%s]\n", symbol[i].name);
}

//DTRACE
void display_device_read(paddr_t addr, int len, IOMap *map)
{
    log_write(ANSI_FMT("read memory: ", ANSI_FG_BLUE) FMT_PADDR ", the len is %d, the read device is " 
                    ANSI_FMT(" %s ", ANSI_BG_BLUE) "\n", addr, len, map->name);
}

void display_device_write(paddr_t addr, int len, word_t data, IOMap *map)
{
    log_write(ANSI_FMT("write memory: ", ANSI_FG_YELLOW) FMT_PADDR ", the len is %d, the written data is " FMT_WORD 
                    ", the written device is "ANSI_FMT(" %s ", ANSI_BG_YELLOW)"\n", addr, len, data, map->name);
}
