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
#include <limits.h>
#include <stdbool.h>
#include <memory/paddr.h>
#include <isa.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
enum {
    TK_NOTYPE = 256,
    NUM = 1,
    PLUS = 2,
    SUB = 3,
    MUL = 4,
    DIV = 5,
    LEFT = 6,
    RIGHT = 7,
    TK_EQ = 8,
    NOTEQ = 9,
    AND = 10,
    HEX = 11,
    REG = 12,
    /* TODO: Add more token types */
};

static struct rule {
    const char *regex;
    int token_type;
} rules[] = {
	  /* TODO: Add more rules.
           * Pay attention to the precedence level of different rules.
           */
        {" +", TK_NOTYPE},    // spaces
        {"\\+", PLUS},         // plus
        {"\\-", SUB},         // sub
        {"\\*", MUL},         // mul
        {"\\/", DIV},         // div
        {"\\(", LEFT},         // left
        {"\\)", RIGHT},         // right
        {"\\=\\=", TK_EQ},        // equal
        {"\\!\\=", NOTEQ},      // not equal
        {"\\&\\&", AND},        // and
        {"0x[0-9a-fA-F]+", HEX},     // hex
        {"[0-9]+", NUM},      // num
        {"\\$[a-zA-Z]*[0-9]*",REG}, //reg
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
 
void init_regex() {
    int i;
    char error_msg[128];
    int ret;

    for (i = 0; i < NR_REGEX; i++) {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0) {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}

typedef struct token {
    int type;
    char str[64];
} Token;

static Token tokens[256] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e) {
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0') {
    	/* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len, substr_start);

                position += substr_len;
        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
                switch (rules[i].token_type) {
                    case NUM:
                    case HEX:
                        tokens[nr_token].type = rules[i].token_type;
                        strncpy(tokens[nr_token].str, substr_start, substr_len);
                        tokens[nr_token].str[substr_len] = '\0';
                        nr_token++;
                        break;
                     case REG:
            		tokens[nr_token].type = rules[i].token_type;
			if (substr_start[0] == '$' && (substr_start[1] < '0' || substr_start[1] > '9')) {// 去掉 $ 并复制剩余部分
               			strncpy(tokens[nr_token].str, substr_start + 1, substr_len - 1);
                		tokens[nr_token].str[substr_len - 1] = '\0';  // 确保字符串以 \0 结尾
		    	}else{// 直接复制整个子字符串
                		strncpy(tokens[nr_token].str, substr_start, substr_len);
                		tokens[nr_token].str[substr_len] = '\0';  // 确保字符串以 \0 结尾
           		 }
                    case TK_NOTYPE:
                        break;
                    default:
                        tokens[nr_token].type = rules[i].token_type;
                        nr_token++;
                        break;
                }
                break;
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }

    return true;
}

bool check_parentheses(int p, int q) {
    int simple = 0;
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == LEFT){
            simple ++;
        } else if (tokens[i].type == RIGHT){
            simple --;
        }
        if (simple < 0) {
            return false;
        }
    }   
    printf("simple=%d\n",simple);
    if(simple == 0){
        return true;
    }
    else{
        return false;
    }
}

int get_precedence(int i) {
    switch (tokens[i].type) {
        case PLUS:
        case SUB:
           return 1; // Lower precedence
        case MUL:
        case DIV:
           return 2; // Higher precedence
        default:
            return 0; // Not an operator
    }
}
int eval(int p, int q) {
	printf("p=%d,q=%d\n",p,q);
    if (p > q) {
      	/* Bad expression */
        assert(0);
        return -1;
    }
    else if (p == q) {
        if(tokens[p].type == NUM || tokens[p].type == HEX) {
            return atoi(tokens[p].str);
        }
        else{
            assert(0);
            return -1;
        }
    }
    else if (!check_parentheses(p, q)) {
            assert(0);
            return -1;
    }
    else{
        int op = -1;
        int simple = 0;
        int precedence = INT_MAX;
        int op_simple = INT_MAX;
        for (int i = p; i <= q; i++) {
            switch (tokens[i].type) {
                case LEFT:
                    simple ++;
                    break;
                case RIGHT:
                    simple --;
                    if(simple < 0){
                        assert(0);
                        return -1;
                    }
                    break;
                case PLUS:
                case SUB:
                case MUL:
                case DIV:
                case TK_EQ:
                case NOTEQ:
                case AND:
       		    {
                    if(simple <= op_simple){
                        int current_precedence = get_precedence(i);
                        if(simple < op_simple || (simple == op_simple && current_precedence <= precedence)){
                            op = i;
                            precedence = current_precedence;
                            op_simple = simple;
                        }
                    }
                }
                break;
            default:
                break;
            }
        }
        if (op == -1) {
            return eval(p+1, q-1);
        }
        else{
             int val1 = eval(p + op_simple, op -1);
             int val2 = eval(op + 1, q - op_simple);
            switch(tokens[op].type){
                  case PLUS:
                  return val1 + val2;
                case SUB:
                    return val1 - val2;
                case MUL:
                    return val1 * val2;
                case DIV:
                    return val1 / val2;
                default:
                    return 0;
            }

        }
    }
}
word_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }
    int tokens_len = 0;
    for (int i = 0; i < 256; i++) {
        if (tokens[i].type == 0) break;
        tokens_len++;
    }
    //HEX
    for (int i = 0; i < tokens_len; i++) {
        if (tokens[i].type == HEX) {
            long int hex_value = strtol(tokens[i].str, NULL, 16);
            snprintf(tokens[i].str, sizeof(tokens[i].str), "%ld", hex_value);
            tokens[i].type = NUM;
         }
    }
    //REG//
    for(int i = 0 ; i < tokens_len ; i ++){
  	if(tokens[i].type == REG){
  	    bool simple = true;
  	    long int reg_value = isa_reg_str2val(tokens[i].str,&simple);
  	    if(simple == true){
            snprintf(tokens[i].str, sizeof(tokens[i].str), "%ld", reg_value);
            tokens[i].type = NUM;
            }else{
            	  printf("reg value error.\n");
		  assert(0);
		} 
  	        
	}
   }
   /*negative*/  
   for(int i = 0 ; i < tokens_len ; i ++){
	if((tokens[i].type == SUB && i > 0 
	    && tokens[i-1].type != NUM && tokens[i-1].type != HEX && tokens[i-1].type != REG && tokens[i-1].type != RIGHT
	    && tokens[i+1].type == NUM
	    )||
	    (tokens[i].type == SUB && i > 0
             && tokens[i-1].type != NUM && tokens[i-1].type !=HEX && tokens[i-1].type != REG && tokens[i-1].type != RIGHT
             && tokens[i+1].type == HEX
             )||
             (tokens[i].type == SUB && i == 0)
           ){	
	    tokens[i].type = TK_NOTYPE;
	    for(int j = 31 ; j >= 0 ; j --){
		tokens[i+1].str[j] = tokens[i+1].str[j-1];
	    }
	    tokens[i+1].str[0] = '-' ;
	    for(int j = 0 ; j < tokens_len ; j ++){
	       if(tokens[j].type == TK_NOTYPE)
	       {
		    for(int k = j +1 ; k < tokens_len ; k ++){
			tokens[k - 1] = tokens[k];
		    }
		   tokens_len -- ;
	       }
	    }
	  }
    }
   /*derefence*/
   for(int i = 0 ; i < tokens_len ; i ++){
	if((tokens[i].type == MUL && i > 0 
	    && tokens[i-1].type != NUM && tokens[i-1].type != HEX && tokens[i-1].type != REG && tokens[i-1].type != RIGHT
	    && tokens[i+1].type == NUM
	    )||
	    (tokens[i].type == MUL && i > 0
             && tokens[i-1].type != NUM && tokens[i-1].type !=HEX && tokens[i-1].type != REG && tokens[i-1].type != RIGHT
             && tokens[i+1].type == HEX
             )||
             (tokens[i].type == MUL && i == 0)
           ){
	    tokens[i].type = TK_NOTYPE;
	    char* firstaddr = tokens[i+1].str;  
	    if (strncmp(firstaddr, "0x", 2) == 0) {
    		size_t new_len = strlen(firstaddr + 2) + 1;
    		memmove(firstaddr, firstaddr + 2, new_len); 
		}
	    paddr_t addr = 0;
	    printf("%s\n",firstaddr);
	    sscanf(firstaddr,"%x",&addr);
	    printf("%s\n",tokens[i+1].str);
	    printf("%x\n",paddr_read(addr,4));
 	    snprintf(tokens[i+1].str, sizeof(tokens[i+1].str), "%x", paddr_read(addr,4));
	    for(int j = 0 ; j < tokens_len ; j ++){
		if(tokens[j].type == TK_NOTYPE){
		    for(int k = j +1 ; k < tokens_len ; k ++){
			tokens[k - 1] = tokens[k];
		    }
		    tokens_len -- ;
		}
	    }
	  }
	  else {   //HEX
    	for (int i = 0; i < tokens_len; i++) {
        if (tokens[i].type == HEX) {
            long int hex_value = strtol(tokens[i].str, NULL, 16);
            snprintf(tokens[i].str, sizeof(tokens[i].str), "%ld", hex_value);
            tokens[i].type = NUM;
         }
    }
    }
    }

    uint32_t result = eval(0, tokens_len - 1);
    printf("result = %d\n", result);
    *success = true;
    memset(tokens, 0, sizeof(tokens));
    memset(&tokens_len, 0, sizeof(tokens_len));
    return result;
}
