/**
 * FlipScript - A Python-like language for Flipper Zero with C library binding
 * C Code Generator - Modified to generate FlipperZero application template
 */

#include "flipscript.h"
#include "flipscript_types.h"
#include <stdint.h> // Include for intptr_t
#include <ctype.h>  // Include for isdigit

// Forward declarations
void generate_c_from_ast(ASTNode* node, FILE* file, int indent_level);
void generate_string_expression(ASTNode* node, FILE* file);
void preprocess_ast_for_functions(ASTNode* node);
void generate_render_function(ASTNode* func_node, FILE* file);
void generate_input_function(ASTNode* func_node, FILE* file);
void extract_app_state_def(ASTNode* node);
void generate_c_header(FILE* file);
void generate_app_template(FILE* file);
void generate_string_utilities(FILE* file);

// Track app state definition
static char* app_state_definition = NULL;
static int has_main_function = 0;


const char* get_actual_c_function_name(const char* binding_name) {
    if (strcmp(binding_name, "display_draw_frame") == 0) return "canvas_draw_frame";
    if (strcmp(binding_name, "display_draw_str") == 0) return "canvas_draw_str";
    if (strcmp(binding_name, "display_clear") == 0) return "canvas_clear";
    if (strcmp(binding_name, "display_draw_circle") == 0) return "canvas_draw_circle";
    if (strcmp(binding_name, "display_draw_box") == 0) return "canvas_draw_box";
    if (strcmp(binding_name, "display_draw_disk") == 0) return "canvas_draw_disk";
    if (strcmp(binding_name, "delay_ms") == 0) return "furi_delay_ms";
    return binding_name;
}

void preprocess_ast_for_functions(ASTNode* node) {
    if (node == NULL) return;
    
    if (node->type == NODE_FUNCTION_DEF && strcmp(node->data.function_def.name, "main") == 0) {
        has_main_function = 1;
    }
    
    if (node->type == NODE_BLOCK || node->type == NODE_PROGRAM) {
        for (size_t i = 0; i < node->data.block.statement_count; i++) {
            preprocess_ast_for_functions(node->data.block.statements[i]);
        }
    }
}

void extract_app_state_def(ASTNode* node) {
    if (node == NULL) return;
    
    if (node->type == NODE_CLASS_DEF && strcmp(node->data.class_def.name, "AppState") == 0) {
        int capacity = 1024;
        char* fields = (char*)malloc(capacity);
        fields[0] = '\0';
        int len = 0;
        
        for (size_t i = 0; i < node->data.class_def.body->data.block.statement_count; i++) {
            ASTNode* field = node->data.class_def.body->data.block.statements[i];
            
            if (field->type == NODE_ASSIGNMENT) {
                const char* field_name = field->data.assignment.name;
                ASTNode* field_value = field->data.assignment.value;
                const char* field_type = "int";
                
                if (field_value->type == NODE_LITERAL || field_value->type == NODE_IDENTIFIER) {
                    const char* value = (field_value->type == NODE_LITERAL) ? field_value->data.literal.value : field_value->data.identifier.name;
                    if (value[0] == '"' || value[0] == '\'') field_type = "char*";
                    else if (strcmp(value, "True") == 0 || strcmp(value, "False") == 0) field_type = "bool";
                }
                
                int required = snprintf(NULL, 0, "    %s %s;\n", field_type, field_name);
                if (len + required + 1 > capacity) {
                    capacity *= 2;
                    fields = (char*)realloc(fields, capacity);
                }
                len += snprintf(fields + len, capacity - len, "    %s %s;\n", field_type, field_name);
            }
        }
        app_state_definition = fields;
    }
    
    if (node->type == NODE_BLOCK || node->type == NODE_PROGRAM) {
        for (size_t i = 0; i < node->data.block.statement_count; i++) {
            extract_app_state_def(node->data.block.statements[i]);
        }
    }
}

void generate_render_function(ASTNode* func_node, FILE* file) {
    if (func_node->type != NODE_FUNCTION_DEF) return;
    fprintf(file, "// User-defined render function\nvoid render(Canvas* canvas, AppState* app) {\n");
    for (size_t i = 0; i < func_node->data.function_def.body->data.block.statement_count; i++) {
        generate_c_from_ast(func_node->data.function_def.body->data.block.statements[i], file, 1);
    }
    fprintf(file, "}\n\n");
}

void generate_input_function(ASTNode* func_node, FILE* file) {
    if (func_node->type != NODE_FUNCTION_DEF) return;
    fprintf(file, "// User-defined input handler function\nvoid input(InputKey key, InputType type, AppState* app) {\n");
    for (size_t i = 0; i < func_node->data.function_def.body->data.block.statement_count; i++) {
        generate_c_from_ast(func_node->data.function_def.body->data.block.statements[i], file, 1);
    }
    fprintf(file, "}\n\n");
}

void generate_main_function(ASTNode* func_node, FILE* file) {
    if (func_node->type != NODE_FUNCTION_DEF) return;
    fprintf(file, "// User-defined main function\nvoid user_main(AppState* app) {\n");
    for (size_t i = 0; i < func_node->data.function_def.body->data.block.statement_count; i++) {
        generate_c_from_ast(func_node->data.function_def.body->data.block.statements[i], file, 1);
    }
    fprintf(file, "}\n\n");
}

void generate_c_header(FILE* file) {
    generate_app_template(file);
}

void generate_app_template(FILE* file) {
    fprintf(file, "#include <furi.h>\n");
    fprintf(file, "#include <furi_hal.h>\n");
    fprintf(file, "#include <gui/gui.h>\n");
    fprintf(file, "#include <gui/elements.h>\n");
    fprintf(file, "#include <input/input.h>\n");
    fprintf(file, "#include <stdlib.h>\n");
    fprintf(file, "#include <stdbool.h>\n");
    fprintf(file, "#include <stdio.h>\n");
    fprintf(file, "#include <string.h>\n");
    fprintf(file, "#include <math.h>\n");
    fprintf(file, "#include <stdint.h> // Include for intptr_t\n\n");
    
    fprintf(file, "#define SCREEN_WIDTH 128\n#define SCREEN_HEIGHT 64\n\n");
    fprintf(file, "typedef struct AppState AppState;\n\n");
    fprintf(file, "typedef enum { EventTypeTick, EventTypeKey } EventType;\n\n");
    fprintf(file, "typedef struct { EventType type; InputEvent input; } PluginEvent;\n\n");
    fprintf(file, "typedef struct AppState {\n");
    if (app_state_definition) fprintf(file, "%s", app_state_definition);
    else fprintf(file, "    int dummy;\n");
    fprintf(file, "} AppState;\n\n");
    fprintf(file, "typedef struct { FuriMutex* mutex; AppState* app; } AppContext;\n\n");
    
    fprintf(file, "static void render_callback(Canvas* const canvas, void* ctx);\n");
    fprintf(file, "static void input_callback(InputEvent* input_event, void* ctx);\n");
    fprintf(file, "void render(Canvas* canvas, AppState* app);\n");
    fprintf(file, "void input(InputKey key, InputType type, AppState* app);\n");
    fprintf(file, "static void initialize_app_state(AppState* app);\n");
    fprintf(file, "int print(const char* message);\n\n");
}

void generate_application_structure(FILE* file) {
    fprintf(file, "static void render_callback(Canvas* const canvas, void* ctx) {\n    AppContext* context = (AppContext*)ctx; furi_mutex_acquire(context->mutex, FuriWaitForever); render(canvas, context->app); furi_mutex_release(context->mutex); \n}\n\n");
    fprintf(file, "static void input_callback(InputEvent* input_event, void* ctx) {\n    FuriMessageQueue* event_queue = (FuriMessageQueue*)ctx; furi_assert(event_queue); PluginEvent event = {.type = EventTypeKey, .input = *input_event}; furi_message_queue_put(event_queue, &event, FuriWaitForever);\n}\n\n");
    
    fprintf(file, "int32_t app_main(void* p) {\n    UNUSED(p);\n\n");
    fprintf(file, "    AppState* app = malloc(sizeof(AppState));\n    if(!app) { FURI_LOG_E(\"flipscript\", \"Failed to allocate AppState\"); return 255; }\n\n");
    fprintf(file, "    initialize_app_state(app);\n\n");
    fprintf(file, "    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(PluginEvent));\n    if(!event_queue) { FURI_LOG_E(\"flipscript\", \"Failed to allocate event queue\"); free(app); return 255; }\n\n");
    fprintf(file, "    FuriMutex* app_mutex = furi_mutex_alloc(FuriMutexTypeNormal);\n    if(!app_mutex) { FURI_LOG_E(\"flipscript\", \"Failed to allocate app mutex\"); free(app); furi_message_queue_free(event_queue); return 255; }\n\n");
    fprintf(file, "    AppContext app_context = {.mutex = app_mutex, .app = app};\n\n");
    fprintf(file, "    ViewPort* view_port = view_port_alloc();\n    view_port_draw_callback_set(view_port, render_callback, &app_context);\n    view_port_input_callback_set(view_port, input_callback, event_queue);\n\n");
    fprintf(file, "    Gui* gui = furi_record_open(\"gui\");\n    gui_add_view_port(gui, view_port, GuiLayerFullscreen);\n\n");
    
    fprintf(file, "    PluginEvent event;\n    bool running = true;\n    while(running) {\n        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {\n            furi_mutex_acquire(app_mutex, FuriWaitForever);\n            if(event.type == EventTypeKey) {\n                input(event.input.key, event.input.type, app);\n                if(event.input.key == InputKeyBack && event.input.type == InputTypePress) running = false;\n            }\n            furi_mutex_release(app_mutex);\n        }\n");
    if (has_main_function) fprintf(file, "        furi_mutex_acquire(app_mutex, FuriWaitForever); user_main(app); furi_mutex_release(app_mutex);\n");
    fprintf(file, "        view_port_update(view_port);\n    }\n\n");
    
    fprintf(file, "    view_port_enabled_set(view_port, false);\n    gui_remove_view_port(gui, view_port);\n    furi_record_close(\"gui\");\n    view_port_free(view_port);\n    furi_message_queue_free(event_queue);\n    furi_mutex_free(app_mutex);\n    free(app);\n\n    return 0;\n}\n");
}

void generate_string_utilities(FILE* file) {
    fprintf(file, "// String utility functions\n");
    fprintf(file, "char* int_to_str(long value) { char* buffer = malloc(32); if(!buffer) return NULL; snprintf(buffer, 32, \"%%ld\", value); return buffer; }\n\n");
    fprintf(file, "char* str(long value) { return int_to_str(value); }\n\n");
    fprintf(file, "char* str_s(const char* value) { if(!value) return strdup(\"\"); return strdup(value); }\n\n");
    fprintf(file, "int range(int limit) { return limit; }\n\n");
    fprintf(file, "char* str_concat(const char* s1, const char* s2) { if(!s1) s1 = \"\"; if(!s2) s2 = \"\"; size_t len1 = strlen(s1); size_t len2 = strlen(s2); char* result = malloc(len1 + len2 + 1); if(!result) return NULL; strcpy(result, s1); strcat(result, s2); return result; }\n\n");
    fprintf(file, "int print(const char* message) { FURI_LOG_I(\"FlipScript\", \"%%s\", message); return 0; }\n\n");
}

void generate_string_expression(ASTNode* node, FILE* file) {
    if(node == NULL) {
        fprintf(file, "strdup(\"\")");
        return;
    }

    if (node->type == NODE_FUNCTION_CALL && strcmp(node->data.function_call.name, "str") == 0) {
        generate_c_from_ast(node, file, 0);
        return;
    }

    if (node->type == NODE_LITERAL) {
         if (node->data.literal.value[0] == '"' || node->data.literal.value[0] == '\'') {
            fprintf(file, "strdup(%s)", node->data.literal.value);
         } else {
            fprintf(file, "strdup(\"%s\")", node->data.literal.value);
         }
        return;
    }
    
    if (node->type == NODE_BINARY_OP && node->data.binary_op.operator == TOKEN_PLUS) {
        fprintf(file, "str_concat(");
        generate_string_expression(node->data.binary_op.left, file);
        fprintf(file, ", ");
        generate_string_expression(node->data.binary_op.right, file);
        fprintf(file, ")");
        return;
    }

    fprintf(file, "int_to_str((long)(");
    generate_c_from_ast(node, file, 0);
    fprintf(file, "))");
}


void generate_c_from_ast(ASTNode* node, FILE* file, int indent_level) {
    if (node == NULL) return;

    if (node->type == NODE_IMPORT) return;
    
    char indent[100] = "";
    for (int i = 0; i < indent_level; i++) strcat(indent, "    ");
    
    switch (node->type) {
        case NODE_PROGRAM: {
            preprocess_ast_for_functions(node);
            extract_app_state_def(node);
            generate_c_header(file);
            
            // FIX: Generate string utilities FIRST to prevent implicit declaration errors.
            generate_string_utilities(file);
            
            ASTNode* render_function = NULL, *input_function = NULL, *main_function = NULL;
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                if (node->data.block.statements[i] && node->data.block.statements[i]->type == NODE_FUNCTION_DEF) {
                    const char* func_name = node->data.block.statements[i]->data.function_def.name;
                    if (strcmp(func_name, "render") == 0) render_function = node->data.block.statements[i];
                    else if (strcmp(func_name, "input") == 0) input_function = node->data.block.statements[i];
                    else if (strcmp(func_name, "main") == 0) main_function = node->data.block.statements[i];
                }
            }
            
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                if (node->data.block.statements[i] && node->data.block.statements[i]->type == NODE_FUNCTION_DEF) {
                    const char* func_name = node->data.block.statements[i]->data.function_def.name;
                    if (strcmp(func_name, "render") != 0 && strcmp(func_name, "input") != 0 && strcmp(func_name, "main") != 0) {
                        generate_c_from_ast(node->data.block.statements[i], file, indent_level);
                    }
                }
            }
            if(main_function) generate_main_function(main_function, file);
            
            if (render_function) generate_render_function(render_function, file);
            else fprintf(file, "void render(Canvas* canvas, AppState* app) { UNUSED(canvas); UNUSED(app); }\n\n");
            
            if (input_function) generate_input_function(input_function, file);
            else fprintf(file, "void input(InputKey key, InputType type, AppState* app) { UNUSED(key); UNUSED(type); UNUSED(app); }\n\n");
            
            fprintf(file, "static void initialize_app_state(AppState* app) {\n");
            fprintf(file, "    memset(app, 0, sizeof(AppState));\n");

            ASTNode* app_state_class = NULL;
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                if (node->data.block.statements[i] && node->data.block.statements[i]->type == NODE_CLASS_DEF && strcmp(node->data.block.statements[i]->data.class_def.name, "AppState") == 0) {
                    app_state_class = node->data.block.statements[i];
                    break;
                }
            }

            if(app_state_class) {
                ASTNode* class_body = app_state_class->data.class_def.body;
                for (size_t i = 0; i < class_body->data.block.statement_count; i++) {
                    ASTNode* field_assignment = class_body->data.block.statements[i];
                    if(field_assignment && field_assignment->type == NODE_ASSIGNMENT) {
                        fprintf(file, "    app->%s = ", field_assignment->data.assignment.name);
                        generate_c_from_ast(field_assignment->data.assignment.value, file, 0);
                        fprintf(file, ";\n");
                    }
                }
            }
            
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                ASTNode* stmt = node->data.block.statements[i];
                if (stmt && stmt->type != NODE_FUNCTION_DEF && stmt->type != NODE_C_BINDING && stmt->type != NODE_IMPORT && stmt->type != NODE_CLASS_DEF) {
                    generate_c_from_ast(stmt, file, 1);
                }
            }
            fprintf(file, "}\n\n");
            generate_application_structure(file);
            break;
        }
        case NODE_FUNCTION_DEF: {
            const char* func_name = node->data.function_def.name;

            if (strcmp(func_name, "render") == 0 || strcmp(func_name, "input") == 0) break;
            
            fprintf(file, "void* %s(", func_name);
            for (size_t i = 0; i < node->data.function_def.parameter_count; i++) {
                if(strcmp(node->data.function_def.parameters[i], "canvas") == 0) {
                    fprintf(file, "Canvas* canvas");
                } else if(strcmp(node->data.function_def.parameters[i], "app") == 0) {
                    fprintf(file, "AppState* app");
                } else {
                    fprintf(file, "void* %s", node->data.function_def.parameters[i]);
                }
                if (i < node->data.function_def.parameter_count - 1) fprintf(file, ", ");
            }
            fprintf(file, ") {\n");
            
            for (size_t i = 0; i < node->data.function_def.body->data.block.statement_count; i++) {
                generate_c_from_ast(node->data.function_def.body->data.block.statements[i], file, indent_level + 1);
            }
            
            int has_return = 0;
            if (node->data.function_def.body->data.block.statement_count > 0) {
                 ASTNode* last_stmt = node->data.function_def.body->data.block.statements[node->data.function_def.body->data.block.statement_count - 1];
                if(last_stmt && last_stmt->type == NODE_RETURN) has_return = 1;
            }
            if (!has_return) fprintf(file, "%sreturn NULL;\n", indent);
            fprintf(file, "}\n\n");
            break;
        }
        case NODE_BLOCK:
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                generate_c_from_ast(node->data.block.statements[i], file, indent_level);
            }
            break;
        
        case NODE_IF:
            fprintf(file, "%sif (", indent);
            generate_c_from_ast(node->data.if_statement.condition, file, 0);
            fprintf(file, ") {\n");
            generate_c_from_ast(node->data.if_statement.if_block, file, indent_level + 1);
            fprintf(file, "%s}", indent);
            for (size_t i = 0; i < node->data.if_statement.elif_count; i++) {
                fprintf(file, " else if (");
                generate_c_from_ast(node->data.if_statement.elif_clauses[i].condition, file, 0);
                fprintf(file, ") {\n");
                generate_c_from_ast(node->data.if_statement.elif_clauses[i].block, file, indent_level + 1);
                fprintf(file, "%s}", indent);
            }
            if (node->data.if_statement.else_block) {
                fprintf(file, " else {\n");
                generate_c_from_ast(node->data.if_statement.else_block, file, indent_level + 1);
                fprintf(file, "%s}", indent);
            }
            fprintf(file, "\n");
            break;
        case NODE_WHILE:
            fprintf(file, "%swhile (", indent);
            generate_c_from_ast(node->data.while_loop.condition, file, 0);
            fprintf(file, ") {\n");
            generate_c_from_ast(node->data.while_loop.block, file, indent_level + 1);
            fprintf(file, "%s}\n", indent);
            break;
        case NODE_ASSIGNMENT: {
            if (strncmp(node->data.assignment.name, "app.", 4) == 0) {
                fprintf(file, "%sapp->%s = ", indent, node->data.assignment.name + 4);
                generate_c_from_ast(node->data.assignment.value, file, 0);
                fprintf(file, ";\n");
            } else {
                fprintf(file, "%sintptr_t %s = (intptr_t)", indent, node->data.assignment.name);
                generate_c_from_ast(node->data.assignment.value, file, 0);
                fprintf(file, ";\n");
            }
            break;
        }
        case NODE_FUNCTION_CALL: {
            const char* func_name = node->data.function_call.name;

            if (strcmp(func_name, "print") == 0) {
                fprintf(file, "%s{\n", indent); 
                fprintf(file, "%s    char* temp_str = ", indent);
                if (node->data.function_call.argument_count > 0) {
                    generate_string_expression(node->data.function_call.arguments[0], file);
                } else {
                    fprintf(file, "strdup(\"\")");
                }
                fprintf(file, ";\n");
                fprintf(file, "%s    print(temp_str);\n", indent);
                fprintf(file, "%s    free(temp_str);\n", indent);
                fprintf(file, "%s}\n", indent);
            }
            else if (strcmp(func_name, "display_draw_str") == 0 || strcmp(func_name, "canvas_draw_str") == 0) {
                fprintf(file, "%s{\n", indent); 
                fprintf(file, "%s    char* text_to_draw = ", indent);
                generate_string_expression(node->data.function_call.arguments[3], file);
                fprintf(file, ";\n");
                
                fprintf(file, "%s    canvas_draw_str(", indent);
                for (size_t i = 0; i < 3; ++i) {
                    generate_c_from_ast(node->data.function_call.arguments[i], file, 0);
                    fprintf(file, ", ");
                }
                fprintf(file, "text_to_draw);\n");
                fprintf(file, "%s    free(text_to_draw);\n", indent);
                fprintf(file, "%s}\n", indent);
            } else if (strcmp(func_name, "str") == 0) {
                 fprintf(file, "%sint_to_str(", indent);
                 generate_c_from_ast(node->data.function_call.arguments[0], file, 0);
                 fprintf(file, ")");
            } else {
                fprintf(file, "%s%s(", indent, get_actual_c_function_name(func_name));
                for (size_t i = 0; i < node->data.function_call.argument_count; i++) {
                    generate_c_from_ast(node->data.function_call.arguments[i], file, 0);
                    if (i < node->data.function_call.argument_count - 1) fprintf(file, ", ");
                }
                fprintf(file, ")");
                if(indent_level > 0 && node->type != NODE_ASSIGNMENT) fprintf(file, ";\n");
            }
            break;
        }
        case NODE_BINARY_OP:
            fprintf(file, "(");
            generate_c_from_ast(node->data.binary_op.left, file, 0);
            switch(node->data.binary_op.operator) {
                case TOKEN_PLUS: fprintf(file, " + "); break;
                case TOKEN_MINUS: fprintf(file, " - "); break;
                case TOKEN_MULTIPLY: fprintf(file, " * "); break;
                case TOKEN_DIVIDE: fprintf(file, " / "); break;
                case TOKEN_MODULO: fprintf(file, " %% "); break;
                case TOKEN_EQUAL: fprintf(file, " == "); break;
                case TOKEN_NOT_EQUAL: fprintf(file, " != "); break;
                case TOKEN_GREATER: fprintf(file, " > "); break;
                case TOKEN_LESS: fprintf(file, " < "); break;
                default: break;
            }
            generate_c_from_ast(node->data.binary_op.right, file, 0);
            fprintf(file, ")");
            break;
        case NODE_LITERAL: {
            const char* value = node->data.literal.value;
            if (strcmp(value, "True") == 0) {
                fprintf(file, "true");
            } else if (strcmp(value, "False") == 0) {
                fprintf(file, "false");
            } else {
                fprintf(file, "%s", value);
            }
            break;
        }
        case NODE_IDENTIFIER: {
            const char* name = node->data.identifier.name;
            if (strcmp(name, "True") == 0) {
                fprintf(file, "true");
            } else if (strcmp(name, "False") == 0) {
                fprintf(file, "false");
            }
            else if (strncmp(name, "app.", 4) == 0) {
                 fprintf(file, "app->%s", name + 4);
            } else {
                 fprintf(file, "%s", name);
            }
            break;
        }
        case NODE_RETURN:
            fprintf(file, "%sreturn ", indent);
            if (node->data.return_statement.value) {
                if (node->data.return_statement.value->type == NODE_LITERAL) {
                    char* val = node->data.return_statement.value->data.literal.value;
                    if(val && (isdigit(val[0]) || (val[0] == '-' && isdigit(val[1])))) {
                         fprintf(file, "(void*)(intptr_t)");
                    }
                }
                generate_c_from_ast(node->data.return_statement.value, file, 0);
            } else {
                fprintf(file, "NULL");
            }
            fprintf(file, ";\n");
            break;
        case NODE_CLASS_DEF:
        case NODE_C_BINDING:
        case NODE_FOR:
            break; // Not implemented for generation yet
        default:
            fprintf(stderr, "Error: Unhandled node type %d in C generation\n", node->type);
            break;
    }
}
