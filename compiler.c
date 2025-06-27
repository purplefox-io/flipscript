/**
 * FlipScript - A Python-like language for Flipper Zero with C library binding
 * Compiler Implementation - Translates AST to bytecode
 */

#include "flipscript.h"
#include "flipscript_types.h"

// Forward declarations
void compile_ast(Compiler* compiler, ASTNode* node);
int add_c_function(Compiler* compiler, char* name);

// Initialize compiler
Compiler* init_compiler(ASTNode* ast) {
    Compiler* compiler = (Compiler*)malloc(sizeof(Compiler));
    compiler->ast = ast;
    compiler->bytecode = (Instruction*)malloc(1000 * sizeof(Instruction));
    compiler->bytecode_size = 0;
    compiler->bytecode_capacity = 1000;
    compiler->constants = (char**)malloc(100 * sizeof(char*));
    compiler->constant_count = 0;
    compiler->names = (char**)malloc(100 * sizeof(char*));
    compiler->name_count = 0;
    compiler->c_functions = (char**)malloc(100 * sizeof(char*));
    compiler->c_function_count = 0;

    // Initialize unified function table
    compiler->function_capacity = 20;
    compiler->functions = (CompiledFunction*)malloc(compiler->function_capacity * sizeof(CompiledFunction));
    compiler->function_count = 0;

    // Pre-register built-in native functions like str()
    CompiledFunction* str_func = &compiler->functions[compiler->function_count++];
    str_func->name = strdup("str");
    str_func->type = FUNC_NATIVE;
    str_func->arity = 1;
    // The address here is the index in the c_functions array that the runtime will use.
    // We map it to a C function named "int_to_str" which is provided by codegen.c
    str_func->address = add_c_function(compiler, "int_to_str");

    // Pre-register the built-in 'print' function
    CompiledFunction* print_func = &compiler->functions[compiler->function_count++];
    print_func->name = strdup("print");
    print_func->type = FUNC_NATIVE;
    print_func->arity = 1;
    // Map it to the 'print' C function provided by codegen.c
    print_func->address = add_c_function(compiler, "print");


    return compiler;
}

// Add a constant to the constant pool
int add_constant(Compiler* compiler, char* value) {
    for (size_t i = 0; i < compiler->constant_count; i++) {
        if (strcmp(compiler->constants[i], value) == 0) return i;
    }
    compiler->constants[compiler->constant_count] = strdup(value);
    return compiler->constant_count++;
}

// Add a name to the name pool
int add_name(Compiler* compiler, char* name) {
    for (size_t i = 0; i < compiler->name_count; i++) {
        if (strcmp(compiler->names[i], name) == 0) return i;
    }
    compiler->names[compiler->name_count] = strdup(name);
    return compiler->name_count++;
}

// Find a function in the unified function table
int find_function(Compiler* compiler, const char* name) {
    for (size_t i = 0; i < compiler->function_count; i++) {
        if (strcmp(compiler->functions[i].name, name) == 0) return i;
    }
    return -1; // Not found
}

// Add a C function name to the pool, returning its index
int add_c_function(Compiler* compiler, char* name) {
    for (size_t i = 0; i < compiler->c_function_count; i++) {
        if (strcmp(compiler->c_functions[i], name) == 0) return i;
    }
    compiler->c_functions[compiler->c_function_count] = strdup(name);
    return compiler->c_function_count++;
}

// Emit bytecode instruction
void emit_byte(Compiler* compiler, OpCode opcode, int operand) {
    if (compiler->bytecode_size >= compiler->bytecode_capacity) {
        compiler->bytecode_capacity *= 2;
        compiler->bytecode = (Instruction*)realloc(compiler->bytecode, compiler->bytecode_capacity * sizeof(Instruction));
    }
    compiler->bytecode[compiler->bytecode_size].opcode = opcode;
    compiler->bytecode[compiler->bytecode_size].operand = operand;
    compiler->bytecode_size++;
}

// Compile AST to bytecode
void compile_ast(Compiler* compiler, ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_PROGRAM: {
            // Pass 1: Register all functions and C bindings first
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                ASTNode* stmt = node->data.block.statements[i];
                if (!stmt) continue;

                if (compiler->function_count >= compiler->function_capacity) {
                    compiler->function_capacity *= 2;
                    compiler->functions = realloc(compiler->functions, compiler->function_capacity * sizeof(CompiledFunction));
                }

                if (stmt->type == NODE_FUNCTION_DEF) {
                    if (find_function(compiler, stmt->data.function_def.name) != -1) continue; // Already seen
                    CompiledFunction* func = &compiler->functions[compiler->function_count++];
                    func->name = strdup(stmt->data.function_def.name);
                    func->type = FUNC_SCRIPT;
                    func->arity = stmt->data.function_def.parameter_count;
                    func->address = 0; // Placeholder for now
                } else if (stmt->type == NODE_C_BINDING) {
                    if (find_function(compiler, stmt->data.c_binding.name) != -1) continue; // Already seen
                    CompiledFunction* func = &compiler->functions[compiler->function_count++];
                    func->name = strdup(stmt->data.c_binding.name);
                    func->type = FUNC_NATIVE;
                    func->arity = stmt->data.c_binding.parameter_count;
                    func->address = add_c_function(compiler, stmt->data.c_binding.c_function_name);
                }
            }
            
            // Pass 2: Compile the rest of the program
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                compile_ast(compiler, node->data.block.statements[i]);
            }
            break;
        }
        case NODE_FUNCTION_DEF: {
            // Function definitions are now handled on the first pass
            // Here we just compile the body and would handle more complex scoping
            int func_index = find_function(compiler, node->data.function_def.name);
            if(func_index != -1) {
                // In a real compiler, we would jump over the function body
                // For this simple version, we can just compile it.
                // A better approach would use a jump to skip this code during main execution.
                compile_ast(compiler, node->data.function_def.body);
                emit_byte(compiler, OP_RETURN_VALUE, 0); // Implicit return
            }
            break;
        }
        case NODE_C_BINDING:
            // Handled in the first pass, do nothing here.
            break;
        
        case NODE_FUNCTION_CALL: {
            // Compile arguments first
            for(int i = node->data.function_call.argument_count - 1; i >= 0; i--) {
                compile_ast(compiler, node->data.function_call.arguments[i]);
            }

            int func_index = find_function(compiler, node->data.function_call.name);
            if (func_index == -1) {
                fprintf(stderr, "Compile Error: Function '%s' not defined.\n", node->data.function_call.name);
                exit(1);
            }

            CompiledFunction* func = &compiler->functions[func_index];
            if (func->type == FUNC_NATIVE) {
                emit_byte(compiler, OP_CALL_C_FUNCTION, func->address);
            } else {
                emit_byte(compiler, OP_CALL_FUNCTION, func_index);
            }
            break;
        }
        
        // FIX: Handle NODE_CLASS_DEF to prevent "Unhandled node type" error
        case NODE_CLASS_DEF:
            // Class definitions are used by the C code generator,
            // but for bytecode compilation, we can simply ignore them.
            break;

        // --- Other Cases (largely unchanged) ---
        case NODE_BLOCK:
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                compile_ast(compiler, node->data.block.statements[i]);
            }
            break;
        case NODE_LITERAL:
            emit_byte(compiler, OP_LOAD_CONST, add_constant(compiler, node->data.literal.value));
            break;
        case NODE_IDENTIFIER:
            emit_byte(compiler, OP_LOAD_NAME, add_name(compiler, node->data.identifier.name));
            break;
        case NODE_ASSIGNMENT:
            compile_ast(compiler, node->data.assignment.value);
            emit_byte(compiler, OP_STORE_NAME, add_name(compiler, node->data.assignment.name));
            break;
        case NODE_BINARY_OP:
            compile_ast(compiler, node->data.binary_op.left);
            compile_ast(compiler, node->data.binary_op.right);
            switch (node->data.binary_op.operator) {
                case TOKEN_PLUS:      emit_byte(compiler, OP_BINARY_ADD, 0); break;
                case TOKEN_MINUS:     emit_byte(compiler, OP_BINARY_SUB, 0); break;
                // ... other binary ops
                default: break;
            }
            break;
        case NODE_IF:
            compile_ast(compiler, node->data.if_statement.condition);
            int jump_else = compiler->bytecode_size;
            emit_byte(compiler, OP_JUMP_IF_FALSE, 0); // Placeholder
            compile_ast(compiler, node->data.if_statement.if_block);
            int jump_end = compiler->bytecode_size;
            emit_byte(compiler, OP_JUMP, 0); // Placeholder
            compiler->bytecode[jump_else].operand = compiler->bytecode_size;
            if(node->data.if_statement.else_block) {
                compile_ast(compiler, node->data.if_statement.else_block);
            }
            compiler->bytecode[jump_end].operand = compiler->bytecode_size;
            break;
        case NODE_IMPORT:
             // Imports are now just markers, processed by the parser.
             // The compiler can ignore them.
            break;
        case NODE_RETURN:
            if(node->data.return_statement.value) {
                compile_ast(compiler, node->data.return_statement.value);
            } else {
                emit_byte(compiler, OP_LOAD_CONST, add_constant(compiler, "None"));
            }
            emit_byte(compiler, OP_RETURN_VALUE, 0);
            break;
        default:
            fprintf(stderr, "Error: Unhandled node type %d in compilation\n", node->type);
            break;
    }
}