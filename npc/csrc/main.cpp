#include "include/simulator.h"
#include "include/memory.h"
#include "include/debugger.h"
#include "include/trace.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    // 检查调试模式（通过--debug参数）
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            debug_mode = true;
            printf("Debug mode enabled\n");
            break;
        }
    }
    
    // 初始化仿真环境和内存
    sim_init(argc, argv);
    
    if (!debug_mode) {
        // 非调试模式
        printf("Starting simulation in batch mode...\n");
        run_to_completion();
    } else {
        // 调试模式：交互式命令行
        printf("Debugging commands available:\n");
        printf("  s[n] - Step n instructions (default 1)\n");
        printf("  c    - Continue to completion\n");
        printf("  x len addr - Examine memory (e.g. 'x 4 0x80000000')\n");
        printf("  p [reg_num] - Print register value(s)\n");
        printf("  i    - Show instruction trace (itrace)\n");
        printf("  f    - Show function trace (ftrace)\n");
        printf("  q    - Quit simulation\n");

        while (!contextp->gotFinish() && main_time < 100000) {
            printf("(npc) ");
            fflush(stdout);            
            std::string cmd;
            std::getline(std::cin, cmd);

            // 移除首尾空白字符
            cmd.erase(0, cmd.find_first_not_of(" \t\n\r"));
            cmd.erase(cmd.find_last_not_of(" \t\n\r") + 1);

            if (cmd.empty()) continue;

            // 命令解析
            try {
                switch (cmd[0]) {
                    case 's': {  // 单步执行
                        int steps = 1;
                        if (cmd.length() > 1) {
                            steps = std::stoi(cmd.substr(1));
                            if (steps <= 0) throw std::invalid_argument("Step count must be positive");
                        }
                        printf("Stepping %d instruction(s)\n", steps);
                        cpu_exec(steps);
                        break;
                    }
                    case 'c': {  // 执行到结束
                        printf("Continuing to completion...\n");
                        run_to_completion();
                        break;
                    }
                    case 'x': {  // 内存扫描
                        if (cmd.length() <= 1) {
                            printf("Usage: x [len] [addr] (e.g. 'x 4 0x80000000')\n");
                            break;
                        }
                        cmd_x(cmd.substr(1));
                        break;
                    }
                    case 'p': {  // 打印寄存器
                        if (cmd.length() > 1) cmd_p(cmd.substr(1));
                        else cmd_p("");
                        break;
                    }
                    case 'i': {  // 显示指令跟踪
                        printf("Instruction trace:\n");
                        display_inst();
                        break;
                    }
                    case 'f': {  // 显示函数跟踪
                        printf("Function trace enabled\n");
                        break;
                    }
                    case 'q': {  // 退出
                        printf("Exiting simulation\n");
                        sim_exit();
                        return 0;
                    }
                    default:
                        printf("Unknown command. Valid commands:\n");
                        printf("  s[n] - Step execution\n");
                        printf("  c    - Continue\n");
                        printf("  x len addr - Examine memory\n");
                        printf("  p [reg_num] - Print register value(s)\n");
                        printf("  i    - Show instruction trace\n");
                        printf("  f    - Show function trace\n");
                        printf("  q    - Quit\n");
                }
            } catch (const std::exception& e) {
                printf("Error: %s\n", e.what());
            }
        }
    }

    // 清理资源
    sim_exit();
    return 0;
}
