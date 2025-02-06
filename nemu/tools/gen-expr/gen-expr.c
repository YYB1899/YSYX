/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned long long result = %s; "
"  printf(\"%%llu\\n\", result); "
"  return 0; "
"}";
int index_buf  = 0;
int choose(int n);
void gen_num();
void gen(char c);
void gen_safe_divisor();
void gen_rand_op();
void gen_rand_expr();

// Function definitio
int choose(int n){
    int flag = rand() % n ; // 0 1 2 ... n
    return flag;
}
void gen_num(){
    int num = rand()% 100 + 1;
    int num_size = 0, num_tmp = num;
    while(num_tmp){
	num_tmp /= 10;
	num_size ++;
    }
    int x = 1;
    while(num_size)
    {
	x *= 10;
	num_size -- ;
    }
    x /= 10;
    while(num)
    {
	char c = num / x + '0';
	num %= x;
	x /= 10;
	buf[index_buf ++] = c;
    }
}
void gen(char c){
    buf[index_buf ++] = c;
}

void gen_rand_op(){
    char op[4] = {'+', '-', '*'};
    int op_position = rand() % 3;
    buf[index_buf++] = op[op_position];
}
void gen_rand_op_div(){
	gen('(');
	gen_num();
	gen(')');
	gen('/');
	gen('(');
	gen_num();
	gen(')');
}
void gen_rand_expr(int depth) {
   if(depth > 2){
   	gen_num();
   	return;
   }	
   if(index_buf > 65530)
       	printf("overSize\n");
    switch (choose(4)) {
	case 0:
	    gen_num();
	    break;
	case 1:
	    gen('(');
	    gen_rand_expr(depth + 1);
	    gen(')');
	    break;	
         case 2: 
            gen('(');
            gen_rand_expr(depth + 1); 
            gen_rand_op(); 
            gen_rand_op_div(depth + 1);
            gen_rand_op();
            gen_rand_expr(depth + 1); 
            gen(')');
            break;
         case 3:
            gen_rand_expr(depth + 1);
            gen_rand_op();
            gen_rand_expr(depth + 1);
            break;
    }
}
int main(int argc, char *argv[]) {
    int seed = time(0);
    srand(seed);
    int loop = 1;
    if (argc > 1) {
	sscanf(argv[1], "%d", &loop);
    }//获取循环次数
    int i;
    for (i = 0; i < loop; i ++) {
	gen_rand_expr(0);	
	buf[index_buf] = '\0';
	sprintf(code_buf, code_format, buf);

	FILE *fp = fopen("/tmp/.code.c", "w");
	assert(fp != NULL);
	fputs(code_buf, fp);
	fclose(fp);

	int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
	if (ret != 0) continue;

	fp = popen("/tmp/.expr", "r");
	assert(fp != NULL);

	unsigned int result;
	ret = fscanf(fp, "%d", &result);
	pclose(fp);

	printf("%u %s\n", result, buf);
	index_buf = 0;
    }
    return 0;
}
