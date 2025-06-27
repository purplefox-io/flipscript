/**
 * FlipScript - A Python-like language for Flipper Zero with C library binding
 * Main Header File
 */

#ifndef FLIPSCRIPT_H
#define FLIPSCRIPT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Forward declarations of main structures
typedef struct Lexer Lexer;
typedef struct Token Token;
typedef struct Parser Parser;
typedef struct ASTNode ASTNode;
typedef struct Compiler Compiler;
typedef struct Runtime Runtime;

// Token types for lexical analysis
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_MODULO,
    TOKEN_ASSIGN,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_GREATER,
    TOKEN_LESS,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_DOT,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_NEWLINE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_ELIF,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_DEF,
    TOKEN_RETURN,
    TOKEN_IMPORT,
    TOKEN_CFUNC,
    TOKEN_CLASS,
} TokenType;

// AST node types
typedef enum {
    NODE_PROGRAM,
    NODE_BLOCK,
    NODE_EXPR,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_LITERAL,
    NODE_IDENTIFIER,
    NODE_ASSIGNMENT,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_FUNCTION_DEF,
    NODE_FUNCTION_CALL,
    NODE_RETURN,
    NODE_IMPORT,
    NODE_C_BINDING,
    NODE_CLASS_DEF,
} NodeType;

// Bytecode instruction types
typedef enum {
    OP_LOAD_CONST,
    OP_LOAD_NAME,
    OP_STORE_NAME,
    OP_BINARY_ADD,
    OP_BINARY_SUB,
    OP_BINARY_MUL,
    OP_BINARY_DIV,
    OP_BINARY_MOD,
    OP_COMPARE_EQ,
    OP_COMPARE_NEQ,
    OP_COMPARE_GT,
    OP_COMPARE_LT,
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_CALL_FUNCTION,
    OP_RETURN_VALUE,
    OP_CALL_C_FUNCTION,
} OpCode;

// C function pointer type for binding
typedef void* (*CFunctionPtr)(void**);

// Function declarations

// Lexer functions
Lexer* init_lexer(char* source);
Token get_next_token(Lexer* lexer);

// Parser functions
Parser* init_parser(Lexer* lexer);
ASTNode* parse_program(Parser* parser);
ASTNode* parse_statement(Parser* parser);
ASTNode* parse_expression(Parser* parser);

// Compiler functions
Compiler* init_compiler(ASTNode* ast);
void compile_ast(Compiler* compiler, ASTNode* node);

// C code generation functions
void generate_c_from_ast(ASTNode* node, FILE* file, int indent_level);

// Runtime functions
Runtime* init_runtime(Compiler* compiler);
void execute_bytecode(Runtime* runtime);

#endif /* FLIPSCRIPT_H */