/**
 * FlipScript - A Python-like language for Flipper Zero with C library binding
 * Data Structure Definitions
 */

#ifndef FLIPSCRIPT_TYPES_H
#define FLIPSCRIPT_TYPES_H

#include "flipscript.h"

// Token structure
typedef struct Token {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// Lexer structure
typedef struct Lexer {
    char* source;
    size_t source_len;
    size_t pos;
    size_t line;
    size_t column;
    int indent_level;
    Token current_token;
} Lexer;

// AST node structure
struct ASTNode {
    NodeType type;
    union {
        // For literals (numbers, strings)
        struct {
            char* value;
        } literal;
        
        // For identifiers
        struct {
            char* name;
        } identifier;
        
        // For binary operations
        struct {
            ASTNode* left;
            TokenType operator;
            ASTNode* right;
        } binary_op;
        
        // For unary operations
        struct {
            TokenType operator;
            ASTNode* operand;
        } unary_op;
        
        // For variable assignment
        struct {
            char* name;
            ASTNode* value;
        } assignment;
        
        // For blocks of code
        struct {
            ASTNode** statements;
            size_t statement_count;
        } block;
        
        struct {
            ASTNode* condition;
            ASTNode* if_block;
            // Add support for multiple elif blocks
            struct {
                ASTNode* condition;
                ASTNode* block;
            } *elif_clauses;
            size_t elif_count;
            ASTNode* else_block;
        } if_statement;
        
        // For while loops
        struct {
            ASTNode* condition;
            ASTNode* block;
        } while_loop;
        
        // For function definitions
        struct {
            char* name;
            char** parameters;
            size_t parameter_count;
            ASTNode* body;
        } function_def;
        
        // For function calls
        struct {
            char* name;
            ASTNode** arguments;
            size_t argument_count;
        } function_call;
        
        // For return statements
        struct {
            ASTNode* value;
        } return_statement;
        
        // For import statements
        struct {
            char* module_name;
        } import;
        
        // For C function bindings
        struct {
            char* name;
            char* c_function_name;
            char** parameters;
            char** parameter_types;
            size_t parameter_count;
        } c_binding;

        struct {
            char* name;
            ASTNode* body;
        } class_def;
    } data;
};

// Parser structure
typedef struct Parser {
    Lexer* lexer;
    Token current_token;
} Parser;

// Bytecode instruction
typedef struct Instruction {
    OpCode opcode;
    int operand;
} Instruction;

// Distinguish between script functions and native C functions
typedef enum {
    FUNC_SCRIPT,
    FUNC_NATIVE
} FunctionType;

// A unified representation for any callable function
typedef struct {
    char* name;
    FunctionType type;
    size_t arity;
    // For SCRIPT: the starting address in the bytecode
    // For NATIVE: the index in the runtime's C function table
    size_t address; 
} CompiledFunction;


// Compiler structure
typedef struct Compiler {
    ASTNode* ast;
    Instruction* bytecode;
    size_t bytecode_size;
    size_t bytecode_capacity;
    char** constants;
    size_t constant_count;
    char** names;
    size_t name_count;
    char** c_functions;
    size_t c_function_count;

    // A unified table for all functions
    CompiledFunction* functions;
    size_t function_count;
    size_t function_capacity;
} Compiler;

// Runtime structure
typedef struct Runtime {
    Instruction* bytecode;
    size_t bytecode_size;
    char** constants;
    size_t constant_count;
    void** variables;
    size_t variable_count;
    CFunctionPtr* c_functions;
    size_t c_function_count;
    void** stack;
    size_t stack_size;
    size_t stack_capacity;
    size_t pc; // Program counter
} Runtime;

#endif /* FLIPSCRIPT_TYPES_H */
