
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
};

static struct rule {
    const char *regex;
    int token_type;
} rules[] = {
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
        for (i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len, substr_start);

                position += substr_len;
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
    if (tokens[p].type != LEFT || tokens[q].type != RIGHT) {
        return false;
    }

    int balance = 0;
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == LEFT) {
            balance++;
        } else if (tokens[i].type == RIGHT) {
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
void int_to_char(int x, char str[]) {
    memset(str, 0, 32); 
    int tmp_index = 0;
    int tmp_x = x;
    int x_size = 0, flag = 1;
    while (tmp_x) {
        tmp_x /= 10;
        x_size++;
        flag *= 10;
    }
    flag /= 10;
    while (x || flag) {
        int a = x / flag;
        x %= flag;
        str[tmp_index++] = a + '0';
        flag /= 10;
    }
    str[tmp_index] = '\0'; 
}	

int char_to_int(char s[]){
    int s_size = strlen(s);
    int res = 0 ;
    for(int i = 0 ; i < s_size ; i ++)
    {
	res += s[i] - '0';
	res *= 10;
    }
    res /= 10;
    return res;
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
  	if(tokens[i].type == REG)
  	{
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

    uint32_t result = eval(0, tokens_len - 1);
    printf("Result: %d\n", result);
    memset(tokens, 0, sizeof(tokens));
    return result;
}

