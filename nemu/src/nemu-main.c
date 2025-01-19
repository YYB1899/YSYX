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

#include <common.h>
//#include </home/yyb/ysyx-workbench/nemu/tools/gen-expr/gen-expr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "/home/yyb/ysyx-workbench/nemu/src/monitor/sdb/sdb.h"
void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

int main(int argc, char *argv[]) {
	printf("a");
    FILE *input_fp = fopen("/home/yyb/ysyx-workbench/nemu/tools/gen-expr/input", "r");
    if (input_fp == NULL) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }
    // 创建临时文件
    FILE *temp_fp = fopen("/home/yyb/ysyx-workbench/nemu/tools/gen-expr/temp", "w");
    if (temp_fp == NULL) {
        perror("Failed to create temporary file");
        fclose(input_fp);
        return EXIT_FAILURE;
    }
    char line[1024];  // 假设每行不超过1024个字符
    while (fgets(line, sizeof(line), input_fp) != NULL) {
        // 移除行尾的换行符（如果存在）
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        // 获取要添加的内容
        char *line_buf;
        line_buf = strtok(line," ");
        bool success = false;
        int expr_res = expr(line_buf, &success);
        // 写入原始行和附加内容到临时文件
        fprintf(temp_fp, "%s = %d\n", line, expr_res);
    }
    // 关闭文件
    fclose(input_fp);
    fclose(temp_fp);
   // 替换文件
    if (rename("/home/yyb/ysyx-workbench/nemu/tools/gen-expr/temp", "/home/yyb/ysyx-workbench/nemu/tools/gen-expr/input") != 0) {
        perror("Failed to rename temporary file to original file");
        remove("/home/yyb/ysyx-workbench/nemu/tools/gen-expr/temp");  // 清理临时文件
        return EXIT_FAILURE;
    }
   printf("File processing completed successfully.\n");
    return EXIT_SUCCESS;
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  /* Start engine. */
  engine_start();
  return is_exit_status_bad();
  
  
//}

//int main(int argc, char *argv[]) {
    
}

