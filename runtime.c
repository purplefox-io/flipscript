/**
 * FlipScript - A Python-like language for Flipper Zero with C library binding
 * Runtime Implementation - Executes Bytecode
 */

#include "flipscript.h"
#include "flipscript_types.h"

// Faux C binding for a print function for testing.
void* c_print(void** args) {
    // In a real system, you would check the type of args[0]
    printf("%s\n", (char*)args[0]);
    return NULL;
}

// Initialize runtime environment
Runtime* init_runtime(Compiler* compiler) {
    Runtime* runtime = (Runtime*)malloc(sizeof(Runtime));
    runtime->bytecode = compiler->bytecode;
    runtime->bytecode_size = compiler->bytecode_size;
    runtime->constants = compiler->constants;
    runtime->constant_count = compiler->constant_count;
    runtime->variables = (void**)calloc(compiler->name_count, sizeof(void*));
    runtime->variable_count = compiler->name_count;
    
    // Wire up the print function
    runtime->c_functions = (CFunctionPtr*)calloc(1, sizeof(CFunctionPtr)); // Only 1 C func for now
    runtime->c_functions[0] = &c_print;
    runtime->c_function_count = 1;

    runtime->stack_capacity = 1000;
    runtime->stack = (void**)malloc(runtime->stack_capacity * sizeof(void*));
    runtime->stack_size = 0;
    
    runtime->pc = 0;

    // Initialize call frames
    runtime->frame_capacity = 256;
    runtime->call_frames = (CallFrame*)malloc(runtime->frame_capacity * sizeof(CallFrame));
    runtime->frame_count = 0;
    
    // Get reference to compiled functions
    runtime->functions = compiler->functions;
    runtime->function_count_ref = compiler->function_count;

    return runtime;
}

// Push a value onto the stack
void push(Runtime* runtime, void* value) {
    if (runtime->stack_size >= runtime->stack_capacity) {
        runtime->stack_capacity *= 2;
        runtime->stack = (void**)realloc(
            runtime->stack, runtime->stack_capacity * sizeof(void*));
    }
    runtime->stack[runtime->stack_size++] = value;
}

// Pop a value from the stack
void* pop(Runtime* runtime) {
    if (runtime->stack_size == 0) {
        fprintf(stderr, "Error: Stack underflow\n");
        exit(1);
    }
    return runtime->stack[--runtime->stack_size];
}

// Execute bytecode
void execute_bytecode(Runtime* runtime) {
    while (runtime->pc < runtime->bytecode_size) {
        Instruction instruction = runtime->bytecode[runtime->pc++];
        
        switch (instruction.opcode) {
            case OP_LOAD_CONST:
                push(runtime, runtime->constants[instruction.operand]);
                break;
            
            case OP_LOAD_NAME:
                if ((size_t)instruction.operand >= runtime->variable_count) {
                    fprintf(stderr, "Error: Variable index out of bounds\n");
                    return;
                }
                push(runtime, runtime->variables[instruction.operand]);
                break;
            
            case OP_STORE_NAME:
                if ((size_t)instruction.operand >= runtime->variable_count) {
                    fprintf(stderr, "Error: Variable index out of bounds\n");
                    return;
                }
                runtime->variables[instruction.operand] = pop(runtime);
                break;
            
            // --- BINARY OPERATIONS ---
            case OP_BINARY_ADD: {
                long right = atol((char*)pop(runtime));
                long left = atol((char*)pop(runtime));
                long result = left + right;
                char* str_res = malloc(21);
                snprintf(str_res, 21, "%ld", result);
                push(runtime, str_res);
                break;
            }
            case OP_BINARY_SUB: {
                long right = atol((char*)pop(runtime));
                long left = atol((char*)pop(runtime));
                push(runtime, (void*)(left - right));
                break;
            }
            case OP_BINARY_MUL: {
                long right = atol((char*)pop(runtime));
                long left = atol((char*)pop(runtime));
                push(runtime, (void*)(left * right));
                break;
            }
            case OP_BINARY_DIV: {
                long right = atol((char*)pop(runtime));
                long left = atol((char*)pop(runtime));
                if (right == 0) {
                    fprintf(stderr, "Error: Division by zero\n");
                    return;
                }
                push(runtime, (void*)(left / right));
                break;
            }
            // --- COMPARISON OPERATIONS ---
            case OP_COMPARE_EQ: {
                void* right = pop(runtime);
                void* left = pop(runtime);
                push(runtime, (void*)((long)left == (long)right));
                break;
            }
            // --- JUMP OPERATIONS ---
            case OP_JUMP_IF_FALSE: {
                void* condition = pop(runtime);
                if (!(long)condition) {
                    runtime->pc = instruction.operand;
                }
                break;
            }
            case OP_JUMP:
                runtime->pc = instruction.operand;
                break;
            
            // --- FUNCTION OPERATIONS ---
            case OP_CALL_FUNCTION: {
                if (runtime->frame_count >= runtime->frame_capacity) {
                    fprintf(stderr, "Error: Call stack overflow\n");
                    return;
                }
                // Push a new call frame
                CallFrame* frame = &runtime->call_frames[runtime->frame_count++];
                frame->return_address = runtime->pc;

                // Jump to the function's bytecode
                runtime->pc = runtime->functions[instruction.operand].address;
                break;
            }
            case OP_RETURN_VALUE: {
                if (runtime->frame_count == 0) {
                    // Returning from top-level script, so we are done
                    return;
                }
                // Pop the call frame
                CallFrame* frame = &runtime->call_frames[--runtime->frame_count];
                
                // Jump back to where we were before the call
                runtime->pc = frame->return_address;
                break;
            }
            case OP_CALL_C_FUNCTION: {
                if ((size_t)instruction.operand >= runtime->c_function_count) {
                    fprintf(stderr, "Error: C function index out of bounds\n");
                    return;
                }
                CFunctionPtr func = runtime->c_functions[instruction.operand];
                
                // Simple argument handling for one argument
                void* args[1];
                args[0] = pop(runtime);

                void* result = func(args);
                push(runtime, result);
                break;
            }
            default:
                fprintf(stderr, "Error: Unknown opcode: %d\n", instruction.opcode);
                return;
        }
    }
}
