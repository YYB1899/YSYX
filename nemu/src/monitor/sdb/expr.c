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
  {"\\!\\=",NOTEQ},      //not_equal
  {"\\&\\&",AND},        //and
  {"0x[0-9a-fA-F]+",HEX},     //hex
  {"[0-9]+", NUM},      // num
  {"\\$[a-zA-Z]*[0-9]*",REG}, //reg
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
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
	Token token1;
	
        switch (rules[i].token_type) {
          case 2:
          	token1.type = 2;
          	tokens[nr_token ++] = token1;
          	
          	break;
          case 3:
          	token1.type = 3;
          	tokens[nr_token ++] = token1;
          	
          	break;
          case 4:
          	token1.type = 4;
          	tokens[nr_token ++] = token1;
          	
          	break;
          case 5:
          	token1.type = 5;
          	tokens[nr_token ++] = token1;
          	
          	break;
          case 6:
          	token1.type = 6;
          	tokens[nr_token ++] = token1;
          	
          	break;
          case 7:
                token1.type = 7;
          	tokens[nr_token ++] = token1;
          	break;
          case 256:
          	break;
          case 11:
          	token1.type = 11;
          	strncpy(token1.str,&e[position - substr_len],substr_len);
          	tokens[nr_token ++] = token1;
          	break;
          case 12:
          	token1.type = 12;
          	strncpy(token1.str,&e[position - substr_len],substr_len);
          	tokens[nr_token ++] = token1;
          	break;
          case 1:
          	token1.type = 1;
          	strncpy(token1.str,&e[position - substr_len],substr_len);
          	tokens[nr_token ++] = token1;
          	break;
          case 8:
          	token1.type = 8;
          	strcpy(token1.str,"==");
          	tokens[nr_token ++] = token1;
          	break;
          case 9:
          	token1.type = 9;
          	strcpy(token1.str,"!=");
          	tokens[nr_token ++] = token1;
          	break;
           case 10:
          	token1.type = 10;
          	strcpy(token1.str,"&&");
          	tokens[nr_token ++] = token1;
          	break;
          default:
                printf("i = %d and without rules\n", i);
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
bool check_parentheses(int p, int q)
{
    // return true;
    //    printf("p = %d, q = %d\n",tokens[p].type, tokens[q].type);
    if(tokens[p].type != '('  || tokens[q].type != ')')
	return false;
    int l = p , r = q;
    while(l < r)
    {
	if(tokens[l].type == '('){
	    if(tokens[r].type == ')')
	    {
		l ++ , r --;
		continue;
	    } 

	    else 
		r --;
	}
	else if(tokens[l].type == ')')
	    return false;
	else l ++;
    }
    return true;
}

int max(int a,int b){
	return (a > b) ? a : b;
}

uint32_t eval(int p, int q) {
    if (p > q) {
        /* Bad expression */
        printf("p=%d\n",p);
        printf("q=%d\n",q);
        assert(0);
        return -1;
    }
    else if (p == q) {
        /* Single token.
         * For now this token should be a number.
         * Return the value of the number.
         */
        return atoi(tokens[p].str);
    }
    else if (check_parentheses(p, q) == true) {
        /* The expression is surrounded by a matched pair of parentheses.
         * If that is the case, just throw away the parentheses.
         */
        return eval(p + 1, q - 1);
    }
    else {
        int op = -1; // op = the position of 主运算符 in the token expression;
        bool simple = false;
        for(int i = p ; i <= q ; i ++)
        {
            if(tokens[i].type == 6)
            {
                while(tokens[i].type != 7)
                    i ++;
            }
            if(!simple && tokens[i].type == 10 ){
                simple = true;
                op = max(op, i);
            }
            if(!simple && (tokens[i].type == 8 || tokens[i].type == 9 )){
                simple = true;
                op = max(op, i);
            }
            if(!simple && (tokens[i].type == 2 || tokens[i].type == 3 )){
                simple = true;
                op = max(op, i);
            }
            if(!simple && (tokens[i].type == 4 || tokens[i].type == 5 )){
            	simple = true;
                op = max(op, i);
            }
       }
        
        int  op_type = tokens[op].type;
        uint32_t  val1 = eval(p,op - 1);
        uint32_t  val2 = eval(op + 1,q);
        
        switch (op_type) {
            case 2:
                return val1 + val2;
            case 3:
                return val1 - val2;
            case 4:
                return val1 * val2;
            case 5:
                if(val2 == 0) return 0;
                else return val1 / val2;
            case 8:
                return val1 == val2;
            case 9:
                return val1 != val2;    
            case 10:
                return val1 && val2;        
            default:
                printf("No Op");
                assert(0);
        }
    }
}

void int_to_char(int num , char str[]){
    int len = strlen(str);
    memset(str,0,len); /*initialize str*/
    int num_size = 0, num_tmp = num , base = 1 , index_str = 0;
    while(num_tmp){
	num_tmp /= 10;
	num_size ++;
	base *= 10;
    }
    base /= 10;
    while(num)
    {
	char c = num / base + '0';
	num %= base;
	base /= 10;
	str[index_str ++] = c;
    }
}	
	
int char_to_int(char c[]){
  int c_size = strlen(c);
  int c_int = 0;
  int base = 1;
  for(int i = c_size - 1 ; i > 0 ; i --){
  	c_int += (c[i] - '0') * base;
  	base *= 10;
  	}
  	return c_int;
  }
  	
word_t expr(char *e, bool *success) {
  if (make_token(e) == false) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  /*get tokens_len*/
  int tokens_len = 0;
  int tokens_size = sizeof(tokens)/sizeof(tokens[0]);
  for(int i = 0 ; i < tokens_size ; i ++){
  	if(tokens[i].type == 0) break;
  	tokens_len ++;
  }
  /*REG*/
  for(int i = 0 ; i < tokens_len ; i ++){
  	if(tokens[i].type == 12)
  	{
  		bool simple = false;
  		int reg_value = isa_reg_str2val(tokens[i].str,&simple);
  		if(simple == true){
  			int_to_char(reg_value,tokens[i].str);
  		}else{
  			printf("reg valve error.\n");
			assert(0);
		}  	
	}
  }
  /*HEX*/
  for(int i = 0 ; i < tokens_len ; i ++){
  	if(tokens[i].type == 11)
  	{
  		int hex_value = strtol(tokens[i].str,NULL,16);
  		int_to_char(hex_value,tokens[i].str);	
	}
  }
  /*negative*/
  for (int i = 0; i < nr_token; i ++) {
  if (tokens[i].type == '-' && (i == 0 || tokens[i - 1].type != 1) ) {
	    tokens[i].type = TK_NOTYPE;
	    for(int j = 31 ; j >= 0 ; j --){
		tokens[i+1].str[j] = tokens[i+1].str[j-1];
	    }
	    tokens[i+1].str[0] = '-';
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
  for (int i = 0; i < nr_token; i ++) {
  if (tokens[i].type == '*' && (i == 0 || tokens[i - 1].type != 1) ) {
    
    }
  }
  *success = true;
   if(check_parentheses(0,nr_token - 1) == false){
  	assert(0);
  	return 0;
  }
   uint32_t result = 0;
 	result = eval(0,nr_token - 1);
  	printf("result = %d\n", result);
  	return result;
  
}
