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
#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include "memory/paddr.h"
#include "/home/yyb/ysyx-workbench/nemu/src/monitor/sdb/watchpoint.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

void display_watchpoint(){
	bool flag = false;
	for(int i = 0; i < NR_WP; i ++){
		if(wp_pool[i].flag == true){
			printf("Watchpoint:NO = %d,expr = %s,old_value = %d,new_value = %d\n",wp_pool[i].NO,wp_pool[i].expr,wp_pool[i].old_value,wp_pool[i].new_value);
			flag = true;
		}
	}
	if(flag == false) printf("Without watchpoint could be displayed");
}

void create_watchpoint(char *args){
	WP* p = new_wp();
	strcpy(p -> expr,args);
	bool success = false;
	int tmp = expr(p -> expr,&success);
	if(success == true) p -> old_value = tmp;
	else printf("Expr evaluation error\n");
	printf("Creat watchpoint NO.%d success\n",p -> NO);
}

void delete_watchpoint(int NO){
	for(int i = 0; i < NR_WP; i ++){
		if(wp_pool[i].NO == NO){
			free_wp(&wp_pool[i]);
			return;
		}
	}
}
/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
	int step;
	step = 0;
	if(args == NULL) step = 1;
	else sscanf(args,"%d",&step);
	cpu_exec(step);
	return 0;
}

static int cmd_info(char *args){
	if(args == NULL) printf("No regs or watchpoint to print\n");
	else if(strcmp(args,"r") == 0) isa_reg_display();
	else if(strcmp(args,"w") == 0) display_watchpoint();
	return 0;
}

static  int cmd_d(char *args){
	if(args == NULL) printf("No args to delete\n");
	else delete_watchpoint(atoi(args));
	return 0;
}

static int cmd_w(char *args){
	create_watchpoint(args);
	return 0;
}

static int cmd_x(char *args){
	char* num = strtok(args," ");
	char* firstaddr = strtok(NULL," ");
	int len = 0;
	paddr_t addr = 0;
	int i = 0;
	sscanf(num,"%d",&len);
	sscanf(firstaddr,"%x",&addr);
	for(i = 0;i < len;i ++){
		printf("%x\n",paddr_read(addr,4));
		addr = addr + 4;
	}
	return 0;
}

static int cmd_p(char* args){
	if(args == NULL){
		printf("No args to evaluation\n");
		return 0;
	}
	bool simple = false;
	expr(args,&simple);
	return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si","Next step",cmd_si },
  {"info","Print the reg",cmd_info},
  {"x","Memory scan",cmd_x},
  {"p","Evaluating expressions",cmd_p},
  {"d", "Delete watchpoint by NO", cmd_d},
  {"w", "Create watchpoint with expr", cmd_w},
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
