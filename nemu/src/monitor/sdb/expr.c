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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
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
                    case 1:
                    case 11:
                    case 12:
                        tokens[nr_token].type = rules[i].token_type;
                        strncpy(tokens[nr_token].str, substr_start, substr_len);
                        tokens[nr_token].str[substr_len] = '\0';
                        nr_token++;
                        break;
                    case 256:
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
    if (tokens[p].type != 6 || tokens[q].type != 7) {
        return false;
    }

    int balance = 0;
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == 6) {
            balance++;
        } else if (tokens[i].type == 7) {
            balance--;
        }
        if (balance < 0) {
            return false;
        }
    }
    return balance == 0;
}


uint32_t eval(int p, int q) {
    if (p > q) {
    	/* Bad expression */
        assert(0);
        return -1;
    }
    else if (p == q) {
        if(tokens[p].type == 1 || tokens[p].type == 11) {
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
        int balance = 0;
        int min_precedence = INT_MAX;

        for (int i = p; i <= q; i++) {
            switch (tokens[i].type) {
                case 6:
                    balance++;
                    break;
                case 7:
                    balance--;
                    break;
                case 2:
                case 3:
                case 4:
                case 5:
                case 8:
                case 9:
                case 10:
                    if(balance < min_precedence) {
                        min_precedence = balance;
                        op = i;
                    }
                    break;
            }
        }

        if (op == -1) {
            return eval(p+1, q-1);
        }
        else{
            uint32_t val1 = eval(p+1, op - 1);
            uint32_t val2 = eval(op + 1, q-1);

            switch (tokens[op].type) {
                case 2:   return val1 + val2;
                case 3:   return val1 - val2;
                case 4:   return val1 * val2;
                case 5:   
                  if (val2 == 0) {
        		assert(0); 
    		  }
    		  return val1 / val2;
                case 8:   return val1 == val2;
                case 9:   return val1 != val2;
                case 10:  return val1 && val2;
                default:
                    assert(0);
                    return -1;
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
        if (tokens[i].type == 11) {
            long int hex_value = strtol(tokens[i].str, NULL, 16);
            snprintf(tokens[i].str, sizeof(tokens[i].str), "%ld", hex_value);
            tokens[i].type = 1;
        }
    }
    //REG//
    for(int i = 0 ; i < tokens_len ; i ++){
  	if(tokens[i].type == REG){
  	    bool simple = false;
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
	if((tokens[i].type == 3 && i > 0 
	    && tokens[i-1].type != 1 && tokens[i-1].type != 11 && tokens[i-1].type != 12 && tokens[i-1].type != 7
	    && tokens[i+1].type == 1 
	    )||
	    (tokens[i].type == 3 && i > 0
             && tokens[i-1].type != 1 && tokens[i-1].type != 11 && tokens[i-1].type != 12 && tokens[i-1].type != 7
             && tokens[i+1].type == HEX
             )||
             (tokens[i].type == 3 && i == 0)
           ){	
	    tokens[i].type = 256;
	    for(int j = 31 ; j >= 0 ; j --){
		tokens[i+1].str[j] = tokens[i+1].str[j-1];
	    }
	    tokens[i+1].str[0] = '-' ;
	    for(int j = 0 ; j < tokens_len ; j ++){
	       if(tokens[j].type == 256)
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
	if(((tokens[i].type == 4 || tokens[i].type == 3 )&& i > 0 
           && tokens[i-1].type != 1 && tokens[i-1].type != 11 && tokens[i-1].type != 12 && tokens[i-1].type != 7
           && tokens[i+1].type == 1 
           )||
	   ((tokens[i].type == 4 || tokens[i].type == 3 )&& i > 0
           && tokens[i-1].type != 1 && tokens[i-1].type != 11 && tokens[i-1].type != 12 && tokens[i-1].type != 7
           && tokens[i+1].type == HEX
           )||
           ((tokens[i].type == 4 || tokens[i].type == 3 )&& i == 0)
          ){printf("a");
	    tokens[i].type = 256;
	    int tmp = atoi(tokens[i+1].str);
	    uint32_t a = (uint32_t)tmp;
	    int value = 0;
	    memcpy(&value, &a, sizeof(int));
	    snprintf(tokens[i+1].str, sizeof(tokens[i+1].str),"%d" ,value);	    
	    for(int j = 0 ; j < tokens_len ; j ++){
		if(tokens[j].type == 256){
		    for(int k = j +1 ; k < tokens_len ; k ++){
			tokens[k - 1] = tokens[k];
		    }
		    tokens_len -- ;
		}
	    }
	  }
    }
    uint32_t result = eval(0, tokens_len - 1);
    printf("result= %d\n", result);
    memset(tokens, 0, sizeof(tokens));
    return result;
}

