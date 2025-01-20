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

#include <isa.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
        {"\\$[a-zA-Z]*[0-9]*", REG}, // reg
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

                //Token token1;
                switch (rules[i].token_type) {
                    case NUM:
                    case HEX:
                    case REG:
                        tokens[nr_token].type = rules[i].token_type;
                        strncpy(tokens[nr_token].str, substr_start, substr_len);
                        tokens[nr_token].str[substr_len] = '\0';
                        nr_token++;
                        break;
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
    } else if (p == q) {
        return atoi(tokens[p].str);
    } else if (check_parentheses(p, q)) {
        return eval(p + 1, q - 1);
    } else {
        int op = -1;
        int balance = 0;
        for (int i = p; i <= q; i++) {
            if (tokens[i].type == LEFT) {
                balance++;
            } else if (tokens[i].type == RIGHT) {
                balance--;
            }
            if (balance == 0) {
                if (tokens[i].type == PLUS || tokens[i].type == SUB ||
                    tokens[i].type == MUL || tokens[i].type == DIV ||
                    tokens[i].type == TK_EQ || tokens[i].type == NOTEQ ||
                    tokens[i].type == AND) {
                    op = i;
                }
            }
        }

        if (op == -1) {
            assert(0);
            return -1;
        }

        uint32_t val1 = eval(p, op - 1);
        uint32_t val2 = eval(op + 1, q);

        switch (tokens[op].type) {
            case PLUS:  return val1 + val2;
            case SUB:   return val1 - val2;
            case MUL:   return val1 * val2;
            case DIV:   return val2 == 0 ? 0 : val1 / val2;
            case TK_EQ: return val1 == val2;
            case NOTEQ: return val1 != val2;
            case AND:   return val1 && val2;
            default:
                assert(0);
                return -1;
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

    // Handle register and hexadecimal values
    for (int i = 0; i < tokens_len; i++) {
        if (tokens[i].type == REG) {
            bool simple = false;
            int reg_value = isa_reg_str2val(tokens[i].str, &simple);
            if (simple) {
                snprintf(tokens[i].str, sizeof(tokens[i].str), "%d", reg_value);
                tokens[i].type = NUM;
            } else {
                printf("reg value error.\n");
                assert(0);
            }
        } else if (tokens[i].type == HEX) {
            long int hex_value = strtol(tokens[i].str, NULL, 16);
            snprintf(tokens[i].str, sizeof(tokens[i].str), "%ld", hex_value);
            tokens[i].type = NUM;
        }
    }

    uint32_t result = eval(0, tokens_len - 1);
    printf("result = %d\n", result);
    memset(tokens, 0, sizeof(tokens));
    return result;
}
