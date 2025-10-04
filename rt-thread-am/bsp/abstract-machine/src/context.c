#include <am.h>
#include <klib.h>
#include <rtthread.h>

static Context* ev_handler(Event e, Context *c) {
  rt_thread_t current = rt_thread_self();
  switch (e.event) {
    case EVENT_YIELD:// nemu
     // printf("nemu\n");
      Context** from = (Context**)(((rt_ubase_t*)(current->user_data))[1]);
      Context** to   = (Context**)(((rt_ubase_t*)(current->user_data))[0]);
      if(from != 0){
        *from = c;
      }
      c = *to;
      break; 
    case EVENT_IRQ_TIMER:// native
    	//printf("native\n");
    	break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  rt_thread_t current = rt_thread_self(); //获取当前线程的PCB
  rt_ubase_t user_data_bnk = current->user_data; // user_data保存在临时变量
  rt_ubase_t from_to[2] = {to, from};
  current->user_data = (rt_ubase_t)from_to;
  yield();// 触发自陷
  current->user_data = user_data_bnk; // 恢复原始 user_data
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  rt_hw_context_switch(0,to);
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

void wrapper_func(void *args) { //包裹函数,来支持texit的功能
  uintptr_t *args_addr = ((uintptr_t *)args) - 3 * sizeof(uintptr_t) ;// 参数解析
  void (*tentry)(void *) = (void(*)(void *))(args_addr[2]); // tentry 函数指针
  void *parameter        = (void *)args_addr[1];
  void (*texit)(void)    = (void(*)(void))(args_addr[0]); // texit 函数指针
  tentry(parameter); // 执行用户线程
  texit();// 线程退出处理
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  uintptr_t stack_end = RT_ALIGN((uintptr_t)stack_addr,sizeof(uintptr_t)); //栈对齐处理
  Area area;
  area.end = (void *)stack_end;
  area.start = (void *)stack_end - FINSH_THREAD_STACK_SIZE;
  uintptr_t *args_addr = ((uintptr_t *)((uintptr_t)stack_end - sizeof(Context))) - 3 * sizeof(uintptr_t) ; // 计算参数存储位置
  args_addr[0] = ((uintptr_t)texit);
  args_addr[1] = ((uintptr_t)parameter);
  args_addr[2] = ((uintptr_t)tentry);
  Context *c = kcontext(area, wrapper_func, (void *)((uintptr_t)stack_end - sizeof(Context))); // 不能从入口返回
  return (rt_uint8_t *)c;
}
