#ifndef TRACE_H
#define TRACE_H
#define INST_NUM 16

typedef struct{
	word_t pc;
	uint32_t inst;
}InstBuf;

typedef void(*io_callback_t)(uint32_t, int, bool);
typedef struct {
  const char *name;
  // we treat ioaddr_t as paddr_t here
  paddr_t low;
  paddr_t high;
  void *space;
  io_callback_t callback;
} IOMap;

void trace_inst(word_t pc, uint32_t inst);
void display_inst();
void display_memory_read(paddr_t addr, int len);
void display_memory_write(paddr_t addr, int len, word_t data);
void parse_elf(const char *elf_file);
void display_call_func(word_t pc, word_t func_addr);
void display_ret_func(word_t pc);
void display_device_read(paddr_t addr, int len, IOMap *map);
void display_device_write(paddr_t addr, int len, word_t data, IOMap *map);
#endif
