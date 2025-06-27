/**
 * FlipScript - A Python-like language for Flipper Zero with C library binding
 * Main Program - Command Line Tool
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "flipscript.h"
#include "flipscript_types.h"

// Print usage information
void print_usage(const char* program_name) {
    printf("FlipScript - A Python-like language for Flipper Zero\n");
    printf("Usage:\n");
    printf("  %s [options] <filename>\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -c           Generate C code output\n");
    printf("  -b           Generate bytecode output\n");
    printf("  -r           Run the script directly\n");
    printf("  -o <output>  Specify output filename\n");
    printf("  -h           Display this help message\n");
}

// Write bytecode to a binary file
void write_bytecode_file(const char* filename, Compiler* compiler) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Error: Could not create bytecode file '%s'\n", filename);
        return;
    }
    
    // Write magic number
    const char* magic = "FSCB"; // FlipScript ByteCode
    fwrite(magic, 1, 4, file);
    
    // Write version
    uint8_t version = 1;
    fwrite(&version, 1, 1, file);
    
    // Write constant pool size
    uint32_t constant_count = compiler->constant_count;
    fwrite(&constant_count, sizeof(uint32_t), 1, file);
    
    // Write constants
    for (uint32_t i = 0; i < constant_count; i++) {
        uint32_t len = strlen(compiler->constants[i]);
        fwrite(&len, sizeof(uint32_t), 1, file);
        fwrite(compiler->constants[i], 1, len, file);
    }
    
    // Write name pool size
    uint32_t name_count = compiler->name_count;
    fwrite(&name_count, sizeof(uint32_t), 1, file);
    
    // Write names
    for (uint32_t i = 0; i < name_count; i++) {
        uint32_t len = strlen(compiler->names[i]);
        fwrite(&len, sizeof(uint32_t), 1, file);
        fwrite(compiler->names[i], 1, len, file);
    }
    
    // Write c_function pool size
    uint32_t c_function_count = compiler->c_function_count;
    fwrite(&c_function_count, sizeof(uint32_t), 1, file);
    
    // Write c_functions
    for (uint32_t i = 0; i < c_function_count; i++) {
        uint32_t len = strlen(compiler->c_functions[i]);
        fwrite(&len, sizeof(uint32_t), 1, file);
        fwrite(compiler->c_functions[i], 1, len, file);
    }
    
    // Write bytecode size
    uint32_t bytecode_size = compiler->bytecode_size;
    fwrite(&bytecode_size, sizeof(uint32_t), 1, file);
    
    // Write bytecode
    for (uint32_t i = 0; i < bytecode_size; i++) {
        fwrite(&compiler->bytecode[i].opcode, sizeof(OpCode), 1, file);
        fwrite(&compiler->bytecode[i].operand, sizeof(int), 1, file);
    }
    
    fclose(file);
    printf("Wrote bytecode to %s\n", filename);
}

// Load bytecode from a binary file
Compiler* load_bytecode_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open bytecode file '%s'\n", filename);
        return NULL;
    }
    
    // Read and verify magic number
    char magic[5] = {0};
    fread(magic, 1, 4, file);
    if (strcmp(magic, "FSCB") != 0) {
        fprintf(stderr, "Error: Not a valid FlipScript bytecode file\n");
        fclose(file);
        return NULL;
    }
    
    // Read version
    uint8_t version;
    fread(&version, 1, 1, file);
    if (version != 1) {
        fprintf(stderr, "Error: Unsupported bytecode version: %d\n", version);
        fclose(file);
        return NULL;
    }
    
    // Create a new compiler to hold the data
    Compiler* compiler = (Compiler*)malloc(sizeof(Compiler));
    compiler->ast = NULL;
    
    // Read constant pool
    uint32_t constant_count;
    fread(&constant_count, sizeof(uint32_t), 1, file);
    compiler->constant_count = constant_count;
    compiler->constants = (char**)malloc(constant_count * sizeof(char*));
    
    for (uint32_t i = 0; i < constant_count; i++) {
        uint32_t len;
        fread(&len, sizeof(uint32_t), 1, file);
        compiler->constants[i] = (char*)malloc(len + 1);
        fread(compiler->constants[i], 1, len, file);
        compiler->constants[i][len] = '\0';
    }
    
    // Read name pool
    uint32_t name_count;
    fread(&name_count, sizeof(uint32_t), 1, file);
    compiler->name_count = name_count;
    compiler->names = (char**)malloc(name_count * sizeof(char*));
    
    for (uint32_t i = 0; i < name_count; i++) {
        uint32_t len;
        fread(&len, sizeof(uint32_t), 1, file);
        compiler->names[i] = (char*)malloc(len + 1);
        fread(compiler->names[i], 1, len, file);
        compiler->names[i][len] = '\0';
    }
    
    // Read c_function pool
    uint32_t c_function_count;
    fread(&c_function_count, sizeof(uint32_t), 1, file);
    compiler->c_function_count = c_function_count;
    compiler->c_functions = (char**)malloc(c_function_count * sizeof(char*));
    
    for (uint32_t i = 0; i < c_function_count; i++) {
        uint32_t len;
        fread(&len, sizeof(uint32_t), 1, file);
        compiler->c_functions[i] = (char*)malloc(len + 1);
        fread(compiler->c_functions[i], 1, len, file);
        compiler->c_functions[i][len] = '\0';
    }
    
    // Read bytecode
    uint32_t bytecode_size;
    fread(&bytecode_size, sizeof(uint32_t), 1, file);
    compiler->bytecode_size = bytecode_size;
    compiler->bytecode_capacity = bytecode_size;
    compiler->bytecode = (Instruction*)malloc(bytecode_size * sizeof(Instruction));
    
    for (uint32_t i = 0; i < bytecode_size; i++) {
        fread(&compiler->bytecode[i].opcode, sizeof(OpCode), 1, file);
        fread(&compiler->bytecode[i].operand, sizeof(int), 1, file);
    }
    
    fclose(file);
    return compiler;
}

int main(int argc, char* argv[]) {
    fprintf(stderr, "DEBUG: FlipScript compiler starting\n");
    fprintf(stderr, "DEBUG: Arguments: %d\n", argc);
    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "DEBUG: argv[%d] = %s\n", i, argv[i]);
    }
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Default options
    int generate_c = 0;
    int generate_bytecode = 0;
    int run_script = 1;
    const char* input_filename = NULL;
    const char* output_filename = NULL;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // Option
            switch (argv[i][1]) {
                case 'c':
                    generate_c = 1;
                    run_script = 0;
                    break;
                case 'b':
                    generate_bytecode = 1;
                    run_script = 0;
                    break;
                case 'r':
                    run_script = 1;
                    break;
                case 'o':
                    if (i + 1 < argc) {
                        output_filename = argv[++i];
                    } else {
                        fprintf(stderr, "Error: -o option requires an argument\n");
                        return 1;
                    }
                    break;
                case 'h':
                    print_usage(argv[0]);
                    return 0;
                default:
                    fprintf(stderr, "Unknown option: %s\n", argv[i]);
                    print_usage(argv[0]);
                    return 1;
            }
        } else {
            // Input filename
            input_filename = argv[i];
        }
    }
    
    if (!input_filename) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Check file extension to determine if it's a FlipScript file or bytecode file
    const char* ext = strrchr(input_filename, '.');
    int is_bytecode = 0;
    
    if (ext && strcmp(ext, ".fsb") == 0) {
        is_bytecode = 1;
        // Can only run bytecode, not generate C or more bytecode
        generate_c = 0;
        generate_bytecode = 0;
    }
    
    Compiler* compiler = NULL;
    
    if (is_bytecode) {
        // Load bytecode from file
        compiler = load_bytecode_file(input_filename);
        if (!compiler) {
            return 1;
        }
    } else {
        // Read the source file
        FILE* file = fopen(input_filename, "r");
        if (!file) {
            fprintf(stderr, "Error: Could not open file: %s\n", input_filename);
            return 1;
        }
        
        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        // Read file content
        char* source = (char*)malloc(file_size + 1);
        fread(source, 1, file_size, file);
        source[file_size] = '\0';
        fclose(file);

        // After reading the file content
        fprintf(stderr, "DEBUG: Read %ld bytes from input file: %s\n", file_size, input_filename);
        if (file_size > 0) {
            fprintf(stderr, "DEBUG: First 100 chars: %.100s\n", source);
        } else {
            fprintf(stderr, "DEBUG: Input file is empty!\n");
        }
        
        // Initialize lexer, parser, and compiler
        Lexer* lexer = init_lexer(source);
        Parser* parser = init_parser(lexer);
        ASTNode* ast = parse_program(parser);
        fprintf(stderr, "DEBUG: Parsing complete. AST generated with type: %d\n", ast->type);
        compiler = init_compiler(ast);
        
        // Compile AST to bytecode
        compile_ast(compiler, ast);
    }
    
    // Process according to mode
    if (generate_c) {
        // Generate C code output
        const char* c_filename;
        if (output_filename) {
            c_filename = output_filename;
        } else {
            // Create default output filename
            c_filename = "output.c";
        }
        
        FILE* c_file = fopen(c_filename, "w");
        if (!c_file) {
            fprintf(stderr, "Error: Could not create output file: %s\n", c_filename);
            return 1;
        }
        
        printf("Generating C code to: %s\n", c_filename);
        generate_c_from_ast(compiler->ast, c_file, 0);
        fclose(c_file);
        printf("C code generation complete.\n");
    }
    
    if (generate_bytecode) {
        // Generate bytecode output
        const char* bytecode_filename;
        if (output_filename) {
            bytecode_filename = output_filename;
        } else {
            // Create default output filename
            char* default_filename = (char*)malloc(strlen(input_filename) + 5);
            strcpy(default_filename, input_filename);
            
            // Replace extension with .fsb or add it if no extension
            char* ext_pos = strrchr(default_filename, '.');
            if (ext_pos) {
                strcpy(ext_pos, ".fsb");
            } else {
                strcat(default_filename, ".fsb");
            }
            
            bytecode_filename = default_filename;
        }
        
        printf("Generating bytecode to: %s\n", bytecode_filename);
        write_bytecode_file(bytecode_filename, compiler);
        printf("Bytecode generation complete.\n");
        
        if (bytecode_filename != output_filename) {
            // We allocated memory for default_filename
            free((void*)bytecode_filename);
        }
    }
    
    if (run_script) {
        // Execute bytecode
        printf("Running script...\n");
        Runtime* runtime = init_runtime(compiler);
        execute_bytecode(runtime);
        printf("Execution complete.\n");
        
        // Print the top of the stack as result if there's anything
        if (runtime->stack_size > 0) {
            void* result = runtime->call_stack[runtime->stack_size - 1];
            printf("Result: %ld\n", (long)result);
        }
        
        // Clean up runtime
        free(runtime->variables);
        free(runtime->c_functions);
        free(runtime->call_stack);
        free(runtime);
    }
    
    // Clean up compiler
    // In a real implementation, you'd also free all memory used by the AST
    free(compiler->bytecode);
    
    for (size_t i = 0; i < compiler->constant_count; i++) {
        free(compiler->constants[i]);
    }
    free(compiler->constants);
    
    for (size_t i = 0; i < compiler->name_count; i++) {
        free(compiler->names[i]);
    }
    free(compiler->names);
    
    for (size_t i = 0; i < compiler->c_function_count; i++) {
        free(compiler->c_functions[i]);
    }
    free(compiler->c_functions);
    
    free(compiler);
    
    return 0;
}