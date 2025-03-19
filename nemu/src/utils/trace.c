#include <common.h>
#include <elf.h>
#include <device/map.h>
#include "trace.h"

InstBuf iringbuf[INST_NUM];
int cur_inst = 0;
int func_num = 0;

void trace_inst(word_t pc, uint32_t inst){
	iringbuf[cur_inst].pc = pc;
	iringbuf[cur_inst].inst = inst;
	cur_inst = (cur_inst + 1) % INST_NUM;
} 

void display_inst(){
    int end = cur_inst;//16
    char buf[128];
    char *p;
    int i = cur_inst;//16

    if(iringbuf[i+1].pc == 0) i = 0;//环形缓冲区为空或未初始化
    do{
        p = buf;
        p += sprintf(buf, "%s" FMT_WORD ":  %08x\t", (i + 1) % INST_NUM == end ? "-->" : "   ", iringbuf[i].pc, iringbuf[i].inst);//格式化 标识,PC 值和指令编码为字符串
        void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
        disassemble(p, buf + sizeof(buf) - p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);//二进制编码反汇编为可读的汇编指令
        puts(buf);
        i = (i + 1) % INST_NUM;
    } while (i != end);
}

void display_memory_read(paddr_t addr, int len){
	printf("Read memory: %u, the len is %d\n.", addr, len);
}

void display_memory_write(paddr_t addr, int len, word_t data){
	printf("Wirte memory: %u, the len is %d\n, the written data is %u.", addr, len, data);
}

