#ifndef TRACE_H
#define TRACE_H
#define INST_NUM 16

typedef struct{
	word_t pc;
	uint32_t inst;
}InstBuf;

void trace_inst(word_t pc, uint32_t inst);
void display_inst();
void display_memory_read(paddr_t addr, int len);
void display_memory_wirte(paddr_t addr, int len, word_t data);
#endif
