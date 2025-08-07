#ifndef DIFFTEST_H
#define DIFFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

void difftest_init(const char *ref_so_file, long img_size, uint32_t inst_start, void *img_buf, void *regfile_buf);
void difftest_memcpy(uint32_t addr, void *buf, size_t n, bool direction);
void difftest_regcpy(void *dut, bool direction);
void difftest_exec(uint32_t n);
void difftest_raise_intr(uint32_t NO);

#ifdef __cplusplus
}
#endif

#endif // DIFFTEST_H
