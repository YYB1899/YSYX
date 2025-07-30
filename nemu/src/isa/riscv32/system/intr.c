/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include "../local-include/reg.h"

word_t isa_raise_intr(word_t NO, vaddr_t epc) { //触发自陷操作
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  //if(NO == 4){epc += 4;} //不能在这里+4，导致 yield() 返回地址被错误覆盖,会只输出AB
  cpu.csr.mepc = epc;   // PC -> CSR[mepc];
  cpu.csr.mcause = NO;  // NO -> CSR[mcause];
  return cpu.csr.mtvec; // CSR[mtvec] -> handler_addr;
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
