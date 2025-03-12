#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vtop.h" 

VerilatedContext* contextp = nullptr;
VerilatedVcdC* tfp = nullptr;
static Vtop* top; 

// 仿真初始化
void sim_init() {
    contextp = new VerilatedContext;
    tfp = new VerilatedVcdC;
    top = new Vtop{contextp}; 
    contextp->traceEverOn(true); 
    top->trace(tfp, 0); 
    tfp->open("wave.vcd"); 
}

// 仿真退出
void sim_exit() {
    if (tfp) {
        tfp->close();  // 关闭波形文件
        delete tfp;
    }
    delete top;
    delete contextp;
}
//DPI-C 函数定义
extern "C" void end_simulation() {
    printf("Ending simulation due to ebreak instruction.\n");
    contextp -> gotFinish(true);// 设置标志位
}

// 单步仿真并输出波形
void step_and_dump_wave() {
    top->eval();
    contextp->timeInc(1); 
    if (tfp) {
        tfp->dump(contextp->time());  // 输出波形
    }
}

int main() {
    sim_init();

    top->rst = 1; 
    top->clk = 0;
    step_and_dump_wave();
    top->clk = 1;
    step_and_dump_wave();
    top->rst = 0; 
    top->clk = 0;
    step_and_dump_wave();

    for (int i = 0; i < 10; i++) {
        top->clk = 0;
        step_and_dump_wave();
        top->clk = 1; 
        step_and_dump_wave();
        if(contextp -> gotFinish()) {//使用标志位
        	break;
        }
    }

    // 结束仿真
    sim_exit();
    return 0;
}
