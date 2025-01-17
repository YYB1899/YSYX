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
#include "sdb.h"
#define NR_WP 32
#include "watchpoint.h"

/*typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  // TODO: Add more members if necessary 
  bool flag;
  char expr[32];
  int old_value;
  int new_value;
} WP;
*/

WP wp_pool[NR_WP] = {0};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].flag = false;
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(){
	for(WP* p = free_; p -> next != NULL; p = p -> next){
		if(p -> flag == false) {
			p -> flag = true;
		if(head == NULL){
			head = p;
			}
		return p;
		}
	}
	printf("All watchpoints have been used");
	assert(0);
	return NULL;
}

void free_wp(WP *wp){
	if(head -> NO == wp -> NO) {
		WP *temp = head;
		head = head -> next;
		temp -> flag = false;
		temp -> next = NULL;
		printf("Deleted watchpoint %d successfully\n",wp -> NO);
		return;
		}
	for(WP *p = head -> next; p -> next != NULL; p = p -> next){
		if(p -> NO == wp -> NO){
			WP *free = p;
			p = p -> next;
			free -> flag = false;
			free -> next = NULL;
			printf("Freed watchpoint %d successfully\n",wp -> NO);
			return;
		}
	}
}
