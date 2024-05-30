#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Tokenizer
//

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
       if(ispunct(*p)){
            cur = cur->next = new_token(TK_PUNCT, p, p+1);
            p++;
            continue;
       }

       error_at(p, "invalid token");
    }   
    cur = cur->next = new_token(TK_EOF, p, p);
    return head.next;
}

//
// Parser
//
typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
}NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val;
};

static Node *new_node(NodeKind kind){
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs){
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_num(int val){
    Node *node = new_node(ND_NUM);
    node-> val = val;
    return node;
}

static Node *expr(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);


// expr = mul ("+" mul | "-" mul)*
static Node *expr(Token **rest, Token *tok){
    Node *node = mul(&tok, tok);
    for(;;){
        if(equal(tok, "+")){
            node = new_binary(ND_ADD, node, mul(&tok, tok->next));
            continue;
        }
        if(equal(tok, "-")){
            node = new_binary(ND_SUB, node, mul(&tok, tok->next));
            continue;
        }
        *rest = tok;
        return node;
    }
}

// mul = primary ("*" primary | "/" primary)*
static Node *mul(Token **rest, Token *tok){
    Node *node = primary(&tok, tok);
    for(;;){
        if(equal(tok, "*")){
            node = new_binary(ND_MUL, node, primary(&tok, tok->next));
            continue;
        }
        if(equal(tok, "/")){
            node = new_binary(ND_DIV, node, primary(&tok, tok->next));
            continue;
        }
        *rest = tok;
        return node;
    }
}

// primary = num | "(" expr ")"
static Node *primary(Token **rest, Token *tok){
    if(equal(tok, "(")){
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }
    if(tok->kind == TK_NUM){
        Node *node = new_num(get_number(tok));
        *rest = tok->next;
        return node;
    }
    error_tok(tok, "expected an expression");
}

//
// Code generation
//
static int depth;

static void push(void){
    printf("  push %%rax\n");
    depth++;
}

static void pop(char *arg){
    printf("  pop %s\n", arg);
    depth--;
}

static void gen_expr(Node *node){
    if(node->kind == ND_NUM){
        printf("  mov $%d, %%rax\n", node->val);
        return;
    }
    gen_expr(node->rhs);
    push();
    gen_expr(node->lhs);
    pop("%rdi");
    switch (node->kind){
        case ND_ADD:
            printf("  add %%rdi, %%rax\n");
            return;
        case ND_SUB:
            printf("  sub %%rdi, %%rax\n");
            return;
        case ND_MUL:
            printf("  imul %%rdi, %%rax\n");
            return;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv %%rdi\n");
            return;
    }
    error("invalid expression");
}

int main(int argc, char **argv) {
    if (argc != 2) { 
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }
    current_input = argv[1];
    Token *tok = tokenize();
    Node *node = expr(&tok, tok);
    if(tok->kind != TK_EOF){
        error_tok(tok, "extra token");
    }
    printf("  .globl main\n");
    printf("main:\n");
    gen_expr(node);
    printf("  ret\n");

    assert(depth == 0);
    return 0;
}