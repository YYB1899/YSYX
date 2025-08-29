#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <string>

// 调试命令函数声明
void cmd_p(const std::string& args);
void init_capstone();

#endif // DEBUGGER_H 