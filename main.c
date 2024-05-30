#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TK_PUNCT, 
    TK_NUM,
    TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    Token *next;        // Next token
    int val;            // If kind is TK_NUM, the value is stored here
    char *loc;          // Token location
    int len;            // Token length
};

static char *current_input;
static void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void verror_at(char *loc, char *fmt, va_list ap){
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void error_at(char *loc, char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

static void error_tok(Token *tok, char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}
static bool equal(Token *tok, char *op){
    return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

static Token *skip(Token *tok, char *s){
    if(!equal(tok, s))
        error_tok(tok, "expected '%s", s);
    return tok->next;
}

static int get_number(Token *tok){
    if(tok->kind != TK_NUM)
        error_tok(tok, "expected a number");
    return tok->val;
}

static Token *new_token(TokenKind kind, char *start, char *end){
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = start;
    tok->len = end - start;
    return tok;
}

static Token *tokenize(){
    char *p = current_input;
    Token head = {};
    Token *cur = &head;
    while(*p) {
       if(isspace(*p)){
            p++;
            continue;
       } 
       if(isdigit(*p)){
           cur = cur->next = new_token(TK_NUM, p, p);
           char *q = p;
           cur->val = strtol(p, &p, 10);
           cur->len = p - q;
           continue;
       }
       if(*p == '+' || *p == '-'){
            cur = cur->next = new_token(TK_PUNCT, p, p+1);
            p++;
            continue;
       }

       error_at(p, "invalid token");
    }   
    cur = cur->next = new_token(TK_EOF, p, p);
    return head.next;
}

int main(int argc, char **argv) {
    if (argc != 2) { 
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }
    current_input = argv[1];
    Token *tok = tokenize();
    printf("  .globl main\n");
    printf("main:\n");
    printf("  mov $%d, %%rax\n", get_number(tok));
    tok = tok->next;

    while(tok->kind != TK_EOF) {
        if(equal(tok, "+")){
            tok = tok->next;
            printf("  add $%d, %%rax\n", get_number(tok));
            tok = tok->next;
            continue;
        }
        if(equal(tok, "-")){
            tok = tok->next;
            printf("  sub $%d, %%rax\n", get_number(tok));
            tok = tok->next;
            continue;
        }
        fprintf(stderr, "unexpected character: '%s'\n", tok->loc);
        return 1;
    }
    printf("  ret\n");
    return 0;
}