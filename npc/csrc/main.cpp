#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vtop.h"
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstdlib>  // for system()

VerilatedContext* contextp = nullptr;
VerilatedVcdC* tfp = nullptr;
static Vtop* top;

extern "C" void end_simulation() {
    printf("Simulation ended by ebreak instruction\n");
    if (contextp) contextp->gotFinish(true);
}

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
        tfp->close();
        delete tfp;
    }
    delete top;
    delete contextp;
}

// 单步仿真并输出波形
void step_and_dump_wave() {
    top->eval();
    contextp->timeInc(1);
    if (tfp) {
        tfp->dump(contextp->time());
    }
}

// 检查系统命令执行结果
void check_system_call(const std::string& cmd) {
    int ret = system(cmd.c_str());//执行传入的命令字符串
    if (ret != 0) {//命令执行失败
        fprintf(stderr, "Command failed (code %d): %s\n", ret, cmd.c_str());
        exit(EXIT_FAILURE);//终止程序
    }
}

// 从ELF文件生成指令hex文件
void prepare_instructions(const char* elf_path) {//ELF文件路径elf_path
    // 生成Verilog格式的指令文件
    std::string cmd = "riscv64-unknown-elf-objcopy -O verilog --only-section=.text ";//字符串类,只提取.text段,输出格式为Verilog兼容的十六进制
    cmd += elf_path;
    cmd += " build/inst.hex";//输出的 Verilog 十六进制文件
    check_system_call(cmd);
    //删除@ 地址行，仅保留指令数据
    check_system_call("sed -i '/@/d' build/inst.hex");
}

int main(int argc, char** argv) {
    // 初始化仿真环境
    sim_init();

    // 处理程序加载
    if (argc > 1) {//带参数
        printf("Loading program: %s\n", argv[1]);
        prepare_instructions(argv[1]);//第二个为elf文件的路径
    } else {//无参数
        printf("Loading default program\n");
        prepare_instructions("build/dummy-riscv32e-npc.elf");
    }

    // 复位序列
    top->rst = 1;
    top->clk = 0;
    step_and_dump_wave();
    top->clk = 1;
    step_and_dump_wave();
    top->rst = 0;
    top->clk = 0;
    step_and_dump_wave();

    // 主仿真循环
    for (int i = 0; i < 1000; i++) {  // 最大周期数限制
        top->clk = 0;
        step_and_dump_wave();
        top->clk = 1;
        step_and_dump_wave();

        if (contextp->gotFinish()) {
            break;
        }
    }

    // 结束仿真
    sim_exit();
    return 0;
}

