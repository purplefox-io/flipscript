/**
 * FlipScript - A Python-like language for Flipper Zero with C library binding
 * Parser Implementation - Full Version
 */

#include "flipscript.h"
#include "flipscript_types.h"
#include <string.h>
#include <stdio.h>

// Forward declarations for parser functions
ASTNode* parse_statement(Parser* parser);
ASTNode* parse_block(Parser* parser);
ASTNode* parse_expression(Parser* parser);
ASTNode* create_node(NodeType type);
ASTNode* parse_class_definition(Parser* parser);
ASTNode* parse_if_statement(Parser* parser);

// Helper function to create an AST node for a C function binding
ASTNode* create_automatic_binding(const char* name, const char* c_function_name, int param_count) {
    ASTNode* node = create_node(NODE_C_BINDING);
    node->data.c_binding.name = strdup(name);
    node->data.c_binding.c_function_name = strdup(c_function_name);
    node->data.c_binding.parameters = (char**)malloc(param_count * sizeof(char*));
    node->data.c_binding.parameter_count = param_count;
    for (int i = 0; i < param_count; i++) {
        char param_name[16];
        snprintf(param_name, sizeof(param_name), "p%d", i);
        node->data.c_binding.parameters[i] = strdup(param_name);
    }
    return node;
}

// Define the mappings for native Flipper functions that can be imported
typedef struct {
    const char* flipscript_name;
    const char* c_name;
    int param_count;
} NativeFunctionMapping;

// FIX: Added mappings for box, circle, and disc drawing functions.
static const NativeFunctionMapping gui_mappings[] = {
    {"canvas_clear", "canvas_clear", 1},
    {"canvas_draw_str", "canvas_draw_str", 4},
    {"canvas_draw_frame", "canvas_draw_frame", 5},
    {"canvas_draw_box", "canvas_draw_box", 5},
    {"canvas_draw_circle", "canvas_draw_circle", 4},
    {"canvas_draw_disc", "canvas_draw_disc", 4},
    {"canvas_flush", "canvas_flush", 1},
    {NULL, NULL, 0}
};
static const NativeFunctionMapping furi_mappings[] = {
    {"delay_ms", "furi_delay_ms", 1},
    {NULL, NULL, 0}
};
static const NativeFunctionMapping notification_mappings[] = {
    {"notification_message", "notification_message", 2},
    {NULL, NULL, 0}
};


// Initialize parser
Parser* init_parser(Lexer* lexer) {
    Parser* parser = (Parser*)malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->current_token = get_next_token(lexer);
    return parser;
}

void advance(Parser* parser) {
    if(parser->current_token.value) free(parser->current_token.value);
    parser->current_token = get_next_token(parser->lexer);
}

ASTNode* create_node(NodeType type) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    node->type = type;
    return node;
}

// Expression Parsing Logic
ASTNode* parse_factor(Parser* parser) {
    Token token = parser->current_token;
    switch (token.type) {
        case TOKEN_NUMBER:
        case TOKEN_STRING: {
            ASTNode* node = create_node(NODE_LITERAL);
            node->data.literal.value = strdup(token.value);
            advance(parser);
            return node;
        }
        case TOKEN_IDENTIFIER: {
            char* name = strdup(token.value);
            advance(parser);
            if (parser->current_token.type == TOKEN_LPAREN) {
                advance(parser);
                ASTNode* node = create_node(NODE_FUNCTION_CALL);
                node->data.function_call.name = name;
                node->data.function_call.arguments = (ASTNode**)malloc(10 * sizeof(ASTNode*));
                node->data.function_call.argument_count = 0;
                if (parser->current_token.type != TOKEN_RPAREN) {
                    do {
                        if(parser->current_token.type == TOKEN_COMMA) advance(parser);
                        node->data.function_call.arguments[node->data.function_call.argument_count++] = parse_expression(parser);
                    } while (parser->current_token.type == TOKEN_COMMA);
                }
                if (parser->current_token.type != TOKEN_RPAREN) {
                    fprintf(stderr, "Syntax error: expected ')' on line %d\n", parser->current_token.line);
                    exit(1);
                }
                advance(parser);
                return node;
            } else if (parser->current_token.type == TOKEN_DOT) {
                advance(parser);
                 if(parser->current_token.type != TOKEN_IDENTIFIER) {
                    fprintf(stderr, "Syntax error: expected field name after '.' on line %d\n", parser->current_token.line);
                    exit(1);
                }
                char* field_name = parser->current_token.value;
                char* combined_name = malloc(strlen(name) + strlen(field_name) + 2);
                sprintf(combined_name, "%s.%s", name, field_name);
                free(name);
                name = combined_name;
                advance(parser);
            }
            ASTNode* node = create_node(NODE_IDENTIFIER);
            node->data.identifier.name = name;
            return node;
        }
        case TOKEN_LPAREN: {
            advance(parser);
            ASTNode* node = parse_expression(parser);
            if (parser->current_token.type != TOKEN_RPAREN) {
                fprintf(stderr, "Syntax error: expected ')' on line %d\n", parser->current_token.line);
                exit(1);
            }
            advance(parser);
            return node;
        }
        default:
            fprintf(stderr, "Syntax error: unexpected token '%s' on line %d\n", token.value ? token.value : "NULL", token.line);
            exit(1);
    }
}

ASTNode* parse_term(Parser* parser) {
    ASTNode* node = parse_factor(parser);
    while (parser->current_token.type == TOKEN_MULTIPLY || parser->current_token.type == TOKEN_DIVIDE) {
        TokenType op = parser->current_token.type;
        advance(parser);
        ASTNode* right = parse_factor(parser);
        ASTNode* new_node = create_node(NODE_BINARY_OP);
        new_node->data.binary_op.left = node;
        new_node->data.binary_op.operator = op;
        new_node->data.binary_op.right = right;
        node = new_node;
    }
    return node;
}

ASTNode* parse_expression(Parser* parser) {
    ASTNode* node = parse_term(parser);
    while (parser->current_token.type == TOKEN_PLUS || parser->current_token.type == TOKEN_MINUS || 
           parser->current_token.type == TOKEN_EQUAL || parser->current_token.type == TOKEN_NOT_EQUAL ||
           parser->current_token.type == TOKEN_GREATER || parser->current_token.type == TOKEN_LESS) {
        TokenType op = parser->current_token.type;
        advance(parser);
        ASTNode* right = parse_term(parser);
        ASTNode* new_node = create_node(NODE_BINARY_OP);
        new_node->data.binary_op.left = node;
        new_node->data.binary_op.operator = op;
        new_node->data.binary_op.right = right;
        node = new_node;
    }
    return node;
}


// Statement-specific parsing functions
ASTNode* parse_def_statement(Parser* parser) {
    advance(parser); // Consume 'def'
    if (parser->current_token.type != TOKEN_IDENTIFIER) { /* error */ exit(1); }
    ASTNode* node = create_node(NODE_FUNCTION_DEF);
    node->data.function_def.name = strdup(parser->current_token.value);
    advance(parser);
    if (parser->current_token.type != TOKEN_LPAREN) { /* ... error ... */ exit(1); }
    advance(parser);
    node->data.function_def.parameters = (char**)malloc(10 * sizeof(char*));
    node->data.function_def.parameter_count = 0;
    if (parser->current_token.type != TOKEN_RPAREN) {
        do {
            if(parser->current_token.type == TOKEN_COMMA) advance(parser);
            node->data.function_def.parameters[node->data.function_def.parameter_count++] = strdup(parser->current_token.value);
            advance(parser);
        } while(parser->current_token.type == TOKEN_COMMA);
    }
    if (parser->current_token.type != TOKEN_RPAREN) { /* ... error ... */ exit(1); }
    advance(parser);
    if (parser->current_token.type != TOKEN_COLON) { /* ... error ... */ exit(1); }
    advance(parser);
    node->data.function_def.body = parse_block(parser);
    return node;
}

ASTNode* parse_class_definition(Parser* parser) {
    advance(parser); // Consume 'class' token
    if (parser->current_token.type != TOKEN_IDENTIFIER) { /* error */ exit(1); }
    ASTNode* node = create_node(NODE_CLASS_DEF);
    node->data.class_def.name = strdup(parser->current_token.value);
    advance(parser);
    if (parser->current_token.type != TOKEN_COLON) { /* error */ exit(1); }
    advance(parser);
    node->data.class_def.body = parse_block(parser);
    return node;
}

ASTNode* parse_if_statement(Parser* parser) {
    advance(parser); // Consume 'if'
    ASTNode* node = create_node(NODE_IF);
    node->data.if_statement.condition = parse_expression(parser);
    if (parser->current_token.type != TOKEN_COLON) { /* error */ exit(1); }
    advance(parser);
    node->data.if_statement.if_block = parse_block(parser);

    node->data.if_statement.elif_clauses = NULL;
    node->data.if_statement.elif_count = 0;
    node->data.if_statement.else_block = NULL;

    while (parser->current_token.type == TOKEN_ELIF) {
        advance(parser); 
        node->data.if_statement.elif_count++;
        node->data.if_statement.elif_clauses = realloc(
            node->data.if_statement.elif_clauses,
            node->data.if_statement.elif_count * sizeof(*(node->data.if_statement.elif_clauses))
        );
        size_t current_elif_index = node->data.if_statement.elif_count - 1;
        node->data.if_statement.elif_clauses[current_elif_index].condition = parse_expression(parser);
        if (parser->current_token.type != TOKEN_COLON) { /* error */ exit(1); }
        advance(parser);
        node->data.if_statement.elif_clauses[current_elif_index].block = parse_block(parser);
    }

    if (parser->current_token.type == TOKEN_ELSE) {
        advance(parser);
        if (parser->current_token.type != TOKEN_COLON) { /* error */ exit(1); }
        advance(parser);
        node->data.if_statement.else_block = parse_block(parser);
    }

    return node;
}

ASTNode* parse_return_statement(Parser* parser) {
    advance(parser); // Consume 'return'
    ASTNode* node = create_node(NODE_RETURN);
    if(parser->current_token.type != TOKEN_NEWLINE && parser->current_token.type != TOKEN_EOF) {
        node->data.return_statement.value = parse_expression(parser);
    } else {
        node->data.return_statement.value = NULL;
    }
    return node;
}

ASTNode* parse_expression_statement(Parser* parser) {
    ASTNode* expr = parse_expression(parser);
    if (expr->type == NODE_IDENTIFIER && parser->current_token.type == TOKEN_ASSIGN) {
        advance(parser); // consume '='
        ASTNode* value = parse_expression(parser);
        ASTNode* assignment_node = create_node(NODE_ASSIGNMENT);
        assignment_node->data.assignment.name = expr->data.identifier.name;
        assignment_node->data.assignment.value = value;
        free(expr);
        return assignment_node;
    }
    return expr;
}

ASTNode* parse_statement(Parser* parser) {
    while (parser->current_token.type == TOKEN_NEWLINE) advance(parser);
    if (parser->current_token.type == TOKEN_EOF) return NULL;

    ASTNode* statement_node = NULL;
    switch (parser->current_token.type) {
        case TOKEN_DEF:     statement_node = parse_def_statement(parser); break;
        case TOKEN_RETURN:  statement_node = parse_return_statement(parser); break;
        case TOKEN_CLASS:   statement_node = parse_class_definition(parser); break;
        case TOKEN_IF:      statement_node = parse_if_statement(parser); break;
        case TOKEN_IMPORT:
            advance(parser);
            statement_node = create_node(NODE_IMPORT);
            statement_node->data.import.module_name = strdup(parser->current_token.value);
            advance(parser);
            break;
        default:
            statement_node = parse_expression_statement(parser);
            break;
    }

    if (parser->current_token.type == TOKEN_NEWLINE) advance(parser);
    return statement_node;
}

ASTNode* parse_block(Parser* parser) {
    ASTNode* node = create_node(NODE_BLOCK);
    node->data.block.statements = (ASTNode**)malloc(100 * sizeof(ASTNode*));
    node->data.block.statement_count = 0;
    size_t capacity = 100;
    if (parser->current_token.type == TOKEN_NEWLINE) advance(parser);
    if (parser->current_token.type != TOKEN_INDENT) { /* ... error ... */ exit(1); }
    advance(parser);
    while (parser->current_token.type != TOKEN_DEDENT && parser->current_token.type != TOKEN_EOF) {
        if(node->data.block.statement_count >= capacity) {
            capacity *= 2;
            node->data.block.statements = (ASTNode**)realloc(node->data.block.statements, capacity * sizeof(ASTNode*));
        }
        node->data.block.statements[node->data.block.statement_count++] = parse_statement(parser);
    }
    if (parser->current_token.type == TOKEN_DEDENT) advance(parser);
    return node;
}

ASTNode* parse_program(Parser* parser) {
    ASTNode* program_node = create_node(NODE_PROGRAM);
    size_t capacity = 100;
    program_node->data.block.statements = (ASTNode**)malloc(capacity * sizeof(ASTNode*));
    program_node->data.block.statement_count = 0;

    // Create a temporary list to hold user statements
    ASTNode** user_statements = (ASTNode**)malloc(capacity * sizeof(ASTNode*));
    size_t user_statement_count = 0;
    size_t user_capacity = capacity;

    // Pass 1: Parse all statements from the source file
    while (parser->current_token.type != TOKEN_EOF) {
        if(user_statement_count >= user_capacity) {
            user_capacity *= 2;
            user_statements = (ASTNode**)realloc(user_statements, user_capacity * sizeof(ASTNode*));
        }
        ASTNode* stmt = parse_statement(parser);
        if(stmt) {
            user_statements[user_statement_count++] = stmt;
        }
    }

    // Pass 2: Add C function bindings to the final AST first
    for (size_t i = 0; i < user_statement_count; i++) {
        ASTNode* stmt = user_statements[i];
        if (stmt && stmt->type == NODE_IMPORT) {
            const char* module_name = stmt->data.import.module_name;
            const NativeFunctionMapping* mappings = NULL;
            int mapping_count = 0;

            if (strcmp(module_name, "gui") == 0) {
                mappings = gui_mappings;
                while(mappings[mapping_count].flipscript_name) mapping_count++;
            } else if (strcmp(module_name, "furi") == 0) {
                mappings = furi_mappings;
                while(mappings[mapping_count].flipscript_name) mapping_count++;
            } else if (strcmp(module_name, "notification") == 0) {
                 mappings = notification_mappings;
                 while(mappings[mapping_count].flipscript_name) mapping_count++;
            }


            if (mappings) {
                 for(int j=0; j < mapping_count; j++) {
                     if(program_node->data.block.statement_count >= capacity) {
                        capacity *= 2;
                        program_node->data.block.statements = (ASTNode**)realloc(program_node->data.block.statements, capacity * sizeof(ASTNode*));
                     }
                     program_node->data.block.statements[program_node->data.block.statement_count++] = 
                        create_automatic_binding(mappings[j].flipscript_name, mappings[j].c_name, mappings[j].param_count);
                }
            }
        }
    }
    
    // Pass 3: Add all user statements (except imports) to the final AST
    for (size_t i = 0; i < user_statement_count; i++) {
        ASTNode* stmt = user_statements[i];
        if (stmt && stmt->type != NODE_IMPORT) {
             if(program_node->data.block.statement_count >= capacity) {
                capacity *= 2;
                program_node->data.block.statements = (ASTNode**)realloc(program_node->data.block.statements, capacity * sizeof(ASTNode*));
             }
             program_node->data.block.statements[program_node->data.block.statement_count++] = stmt;
        } else if (stmt) {
            // Free the import node, it's no longer needed
            free(stmt->data.import.module_name);
            free(stmt);
        }
    }
    
    free(user_statements);
    fprintf(stderr, "DEBUG: Parsing complete. Final AST has %zu statements.\n", program_node->data.block.statement_count);
    return program_node;
}
