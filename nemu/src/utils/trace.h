#ifndef TRACE_H
#define TRACE_H
#define INST_NUM 16

typedef struct{
	word_t pc;
	uint32_t inst;
}InstBuf;

void trace_inst(word_t pc, uint32_t inst);
void display_inst();
#endif
