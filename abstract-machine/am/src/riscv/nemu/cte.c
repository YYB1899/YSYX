#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) { //事件分发
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case -1: ev.event = EVENT_YIELD; c->mepc += 0x4; //应该在这里+4,确保 yield() 返回后继续执行下一条指令
      		break;
      default: ev.event = EVENT_ERROR; break;
    }
   // printf("%d\n",c->mcause);
    c = user_handler(ev, c);
    assert(c != NULL);
  }
  //printf("12121mepc = %d, expected f = %d\n", c->mepc, f);
  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) { //设置异常入口地址
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *ctx = (Context*)((uint8_t*)kstack.end - sizeof(Context));
  //printf("kcontext: entry=%d, f=%d\n", entry, f);
  
  // 设置关键执行状态
  ctx->mstatus = 0x1800;
  ctx->mepc = (uintptr_t)entry;        // 入口地址
  ctx->gpr[10] = (uintptr_t)arg;      // a0 参数
  //ctx->gpr[32] = (uintptr_t)ctx;
  return ctx;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
