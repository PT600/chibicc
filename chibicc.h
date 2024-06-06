#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node Node;
typedef struct Type Type;

//
// tokenize.c
//

typedef enum {
    TK_IDENT,   // Identifier
    TK_PUNCT,   // Keywords or punctuators
    TK_KEYWORD, // Keywords
    TK_NUM,     // Numeric literals
    TK_EOF,     // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
    TokenKind kind; // Token kind
    Token *next;    // Next token
    int val;        // If kind is TK_NUM, its value
    char *loc;      // Token location
    int len;        // Token length
};

// Local variable
typedef struct Obj Obj;
struct Obj {
    Obj *next;
    char *name;
    Type *ty;
    int offset;
};

// Function
typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    Node *body;
    Obj *locals;
    int stack_size;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *op);
bool consume(Token **rest, Token *tok, char *str);
Token *tokenize(char *input);

//
// parse.c
//

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NEG, // unary -
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_ASSIGN,
    ND_ADDR,      // unary &
    ND_DEREF,     // unary *
    ND_RETURN,    // "return"
    ND_IF,        // "if"
    ND_FOR,       // "for" or "while"
    ND_BLOCK,     // {...}
    ND_FUNCALL,   // Function call
    ND_EXPR_STMT, // Expression statement
    ND_VAR,
    ND_NUM, // Integer
} NodeKind;

// AST node type
struct Node {
    NodeKind kind; // Node kind
    Node *next;    // Next node
    Type *ty;      // Type, e.g. int or pointer to int
    Token *tok;    // Representive token
    Node *lhs;     // Left-hand side
    Node *rhs;     // Right-hand side
    // "if" or "for" statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;
    // Block
    Node *body; // Block
    // Funciton call
    char *funcname;
    Node *args;
    Obj *var; // Used if kind == ND_VAR
    int val;  // Used if kind == ND_NUM
};

Function *parse(Token *tok);

//
// type.c
//
typedef enum {
    TY_INT,
    TY_PTR,
    TY_FUNC,
} TypeKind;

struct Type {
    TypeKind kind;
    Type *base;      // for Pointer
    Token *name;     // Declaration
    Type *return_ty; // for Function type
};

extern Type *ty_int;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
void add_type(Node *node);
//
// codegen.c
//

void codegen(Function *prog);
