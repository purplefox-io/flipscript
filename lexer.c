/**
 * FlipScript - A Python-like language for Flipper Zero with C library binding
 * Lexical Analyzer Implementation
 */

#include "flipscript.h"
#include "flipscript_types.h"

// Initialize lexer
Lexer* init_lexer(char* source) {
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->source_len = strlen(source);
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->indent_level = 0;
    return lexer;
}

// Check if character is a space (but not a newline)
int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

// Helper to create a token
Token create_token(TokenType type, char* value, int line, int column) {
    Token token;
    token.type = type;
    token.value = value ? strdup(value) : NULL;
    token.line = line;
    token.column = column;
    return token;
}

// Get the next token from source
Token get_next_token(Lexer* lexer) {
    fprintf(stderr, "DEBUG: Lexer looking at position %zu, character: '%c' (ASCII %d)\n", 
        lexer->pos, 
        lexer->pos < lexer->source_len ? lexer->source[lexer->pos] : '?', 
        lexer->pos < lexer->source_len ? (int)lexer->source[lexer->pos] : -1);
    // Skip whitespace
    while (lexer->pos < lexer->source_len && is_space(lexer->source[lexer->pos])) {
        lexer->pos++;
        lexer->column++;
    }
    
    // Check for EOF
    if (lexer->pos >= lexer->source_len) {
        fprintf(stderr, "DEBUG: EOF reached at position %zu\n", lexer->pos);
        return create_token(TOKEN_EOF, NULL, lexer->line, lexer->column);
    }
    
    
    char current_char = lexer->source[lexer->pos];

    if (current_char == '#') {
        // Skip comment until end of line
        fprintf(stderr, "DEBUG: Found comment at position %zu\n", lexer->pos);
        while (lexer->pos < lexer->source_len && lexer->source[lexer->pos] != '\n') {
            lexer->pos++;
            lexer->column++;
        }
        
        // Recurse to get the next token
        return get_next_token(lexer);
    }
    
    
    // Handle newlines and indentation
    if (current_char == '\n') {
        lexer->pos++;
        lexer->line++;
        lexer->column = 1;
        
        // Count spaces at start of next line for indentation
        int spaces = 0;
        while (lexer->pos < lexer->source_len && is_space(lexer->source[lexer->pos])) {
            spaces++;
            lexer->pos++;
            lexer->column++;
        }
        
        // Convert spaces to indentation level (assuming 4 spaces = 1 indent)
        int new_indent_level = spaces / 4;
        
        if (new_indent_level > lexer->indent_level) {
            lexer->indent_level++;
            return create_token(TOKEN_INDENT, NULL, lexer->line, lexer->column);
        } else if (new_indent_level < lexer->indent_level) {
            lexer->indent_level--;
            return create_token(TOKEN_DEDENT, NULL, lexer->line, lexer->column);
        } else {
            return create_token(TOKEN_NEWLINE, NULL, lexer->line, lexer->column);
        }
    }
    
    // Handle identifiers and keywords
    if (isalpha(current_char) || current_char == '_') {
        int start_pos = lexer->pos;
        while (lexer->pos < lexer->source_len && 
               (isalnum((unsigned char)lexer->source[lexer->pos]) || lexer->source[lexer->pos] == '_' ||
                isdigit((unsigned char)lexer->source[lexer->pos]) || lexer->source[lexer->pos] == '.')) {
            lexer->pos++;
            lexer->column++;
        }
        
        int length = lexer->pos - start_pos;
        char* value = (char*)malloc(length + 1);
        strncpy(value, &lexer->source[start_pos], length);
        value[length] = '\0';
        
        // Check for keywords
        if (strcmp(value, "if") == 0) {
            return create_token(TOKEN_IF, value, lexer->line, lexer->column - length);
        } else if (strcmp(value, "else") == 0) {
            return create_token(TOKEN_ELSE, value, lexer->line, lexer->column - length);
        } else if (strcmp(value, "elif") == 0) {
            return create_token(TOKEN_ELIF, value, lexer->line, lexer->column - length);
        } else if (strcmp(value, "while") == 0) {
            return create_token(TOKEN_WHILE, value, lexer->line, lexer->column - length);
        } else if (strcmp(value, "for") == 0) {
            return create_token(TOKEN_FOR, value, lexer->line, lexer->column - length);
        } else if (strcmp(value, "def") == 0) {
            return create_token(TOKEN_DEF, value, lexer->line, lexer->column - length);
        } else if (strcmp(value, "return") == 0) {
            return create_token(TOKEN_RETURN, value, lexer->line, lexer->column - length);
        } else if (strcmp(value, "import") == 0) {
            return create_token(TOKEN_IMPORT, value, lexer->line, lexer->column - length);
        } else if (strcmp(value, "cfunc") == 0) {
            return create_token(TOKEN_CFUNC, value, lexer->line, lexer->column - length);
        } else if (strcmp(value, "class") == 0) {
            return create_token(TOKEN_CLASS, value, lexer->line, lexer->column - length);
        } 
        else {
            return create_token(TOKEN_IDENTIFIER, value, lexer->line, lexer->column - length);
        }
    }
    
    // Handle numbers
    if (isdigit(current_char)) {
        int start_pos = lexer->pos;
        while (lexer->pos < lexer->source_len && 
               (isdigit((unsigned char)lexer->source[lexer->pos]) || lexer->source[lexer->pos] == '.')) {
            lexer->pos++;
            lexer->column++;
        }
        
        int length = lexer->pos - start_pos;
        char* value = (char*)malloc(length + 1);
        strncpy(value, &lexer->source[start_pos], length);
        value[length] = '\0';
        
        return create_token(TOKEN_NUMBER, value, lexer->line, lexer->column - length);
    }
    
    // Handle strings
    if (current_char == '"' || current_char == '\'') {
        char quote = current_char;
        lexer->pos++;
        lexer->column++;
        
        int start_pos = lexer->pos;
        while (lexer->pos < lexer->source_len && lexer->source[lexer->pos] != quote) {
            // Handle escaped characters
            if (lexer->source[lexer->pos] == '\\' && lexer->pos + 1 < lexer->source_len) {
                lexer->pos += 2;
                lexer->column += 2;
            } else {
                lexer->pos++;
                lexer->column++;
            }
        }
        
        int length = lexer->pos - start_pos;
        char* value = (char*)malloc(length + 1);
        strncpy(value, &lexer->source[start_pos], length);
        value[length] = '\0';
        
        // Skip closing quote
        if (lexer->pos < lexer->source_len) {
            lexer->pos++;
            lexer->column++;
        }
        
        return create_token(TOKEN_STRING, value, lexer->line, lexer->column - length - 2);
    }
    
    // Handle operators and other symbols
    switch (current_char) {
        case '+':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_PLUS, "+", lexer->line, lexer->column - 1);
        case '-':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_MINUS, "-", lexer->line, lexer->column - 1);
        case '*':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_MULTIPLY, "*", lexer->line, lexer->column - 1);
        case '/':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_DIVIDE, "/", lexer->line, lexer->column - 1);
        case '%':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_MODULO, "%", lexer->line, lexer->column - 1);

        case '=':
            lexer->pos++;
            lexer->column++;
            if (lexer->pos < lexer->source_len && lexer->source[lexer->pos] == '=') {
                lexer->pos++;
                lexer->column++;
                return create_token(TOKEN_EQUAL, "==", lexer->line, lexer->column - 2);
            }
            return create_token(TOKEN_ASSIGN, "=", lexer->line, lexer->column - 1);
        case '!':
            lexer->pos++;
            lexer->column++;
            if (lexer->pos < lexer->source_len && lexer->source[lexer->pos] == '=') {
                lexer->pos++;
                lexer->column++;
                return create_token(TOKEN_NOT_EQUAL, "!=", lexer->line, lexer->column - 2);
            }
            // Error: unexpected character
            return create_token(TOKEN_EOF, NULL, lexer->line, lexer->column - 1);
        case '>':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_GREATER, ">", lexer->line, lexer->column - 1);
        case '<':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_LESS, "<", lexer->line, lexer->column - 1);
        case '(':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_LPAREN, "(", lexer->line, lexer->column - 1);
        case ')':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_RPAREN, ")", lexer->line, lexer->column - 1);
        case '{':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_LBRACE, "{", lexer->line, lexer->column - 1);
        case '}':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_RBRACE, "}", lexer->line, lexer->column - 1);
        case ',':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_COMMA, ",", lexer->line, lexer->column - 1);
        case '.':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_DOT, ",", lexer->line, lexer->column - 1);
        case ';':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_SEMICOLON, ";", lexer->line, lexer->column - 1);
        case ':':
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_COLON, ":", lexer->line, lexer->column - 1);
        default:
            // Error: unexpected character
            lexer->pos++;
            lexer->column++;
            return create_token(TOKEN_EOF, NULL, lexer->line, lexer->column - 1);
    }
    fprintf(stderr, "DEBUG: Lexer");
}