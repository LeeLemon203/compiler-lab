/**
 * Lab 5: SLR-Guided Semantic Analysis Framework (Fixed)
 * 
 * Compile: gcc experiment5.c -o exp5
 * Run: ./exp5 test.c
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 
 #define MAX_SYMBOL_LEN 64
 #define MAX_SYMBOLS 100
 #define MAX_PRODUCTIONS 100
 #define MAX_ITEMS 200
 #define MAX_ITEM_SETS 100
 #define MAX_STACK 1000
 #define MAX_SCOPE_DEPTH 32
 #define MAX_CHILDREN 10
 
 /* ==================== Lexical Analysis ==================== */
 
 typedef enum {
     TOKEN_ID, TOKEN_NUM, TOKEN_FLOAT,
     TOKEN_INT, TOKEN_FLOAT_TYPE, TOKEN_VOID,
     TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_RETURN,
     TOKEN_INPUT, TOKEN_PRINT,
     TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV,
     TOKEN_ASSIGN, TOKEN_PLUS_ASSIGN, TOKEN_INC,
     TOKEN_LT, TOKEN_LE, TOKEN_EQ, TOKEN_GT, TOKEN_GE, TOKEN_NE,
     TOKEN_AND, TOKEN_OR, TOKEN_NOT,
     TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACKET, TOKEN_RBRACKET,
     TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_COMMA, TOKEN_SEMICOLON,
     TOKEN_EOF, TOKEN_ERROR
 } TokenType;
 
 typedef struct {
     TokenType type;
     char lexeme[MAX_SYMBOL_LEN];
     int line;
     union {
         int int_value;
         float float_value;
     } value;
 } Token;
 
 /* ==================== Symbol Table ==================== */
 
 typedef enum {
     TYPE_INT, TYPE_FLOAT, TYPE_VOID, TYPE_FUNCTION, TYPE_ARRAY, TYPE_ERROR
 } DataType;
 
 typedef struct Symbol {
     char name[MAX_SYMBOL_LEN];
     DataType type;
     int scope_level;
     int is_initialized;
     struct Symbol* next;
 } Symbol;
 
 typedef struct Scope {
     Symbol* symbols;
     int level;
     struct Scope* parent;
 } Scope;
 
 typedef struct {
     Scope* current_scope;
     int scope_level;
 } SymbolTable;
 
 /* ==================== AST ==================== */
 
 typedef enum {
     AST_PROGRAM, AST_VAR_DECL, AST_ASSIGNMENT, AST_BINARY_OP,
     AST_VARIABLE, AST_CONSTANT, AST_IF_STMT, AST_WHILE_STMT, AST_BLOCK
 } ASTNodeType;
 
 typedef struct ASTNode {
     ASTNodeType type;
     DataType data_type;
     struct ASTNode* children[MAX_CHILDREN];
     int child_count;
     union {
         char name[MAX_SYMBOL_LEN];
         struct {
             int int_value;
             float float_value;
             int is_int;
         } constant;
         struct {
             char op[4];
             struct ASTNode* left;
             struct ASTNode* right;
         } binary;
     } data;
     struct ASTNode* next;
 } ASTNode;
 
 /* ==================== Semantic Analyzer ==================== */
 
 typedef struct {
     ASTNode* ast;
     DataType type;
     char name[MAX_SYMBOL_LEN];
     int int_value;
     float float_value;
     char op[4];
 } Attribute;
 
 Token tokens[MAX_STACK];
 int token_count = 0;
 int token_pos = 0;
 
 Attribute semantic_stack[MAX_STACK];
 int semantic_top = -1;
 
 SymbolTable sym_table;
 int error_count = 0;
 ASTNode* ast_root = NULL;
 
 /* ==================== Symbol Table Operations ==================== */
 
 void init_symbol_table() {
     sym_table.current_scope = (Scope*)malloc(sizeof(Scope));
     sym_table.current_scope->symbols = NULL;
     sym_table.current_scope->level = 0;
     sym_table.current_scope->parent = NULL;
     sym_table.scope_level = 0;
     printf("[Init] Symbol table created\n");
 }
 
 void enter_scope() {
     Scope* new_scope = (Scope*)malloc(sizeof(Scope));
     new_scope->symbols = NULL;
     new_scope->level = sym_table.scope_level + 1;
     new_scope->parent = sym_table.current_scope;
     sym_table.current_scope = new_scope;
     sym_table.scope_level++;
     printf("[Semantic] Enter scope, level: %d\n", sym_table.scope_level);
 }
 
 void exit_scope() {
     Scope* old_scope = sym_table.current_scope;
     sym_table.current_scope = old_scope->parent;
     sym_table.scope_level--;
     Symbol* sym = old_scope->symbols;
     while (sym) {
         Symbol* next = sym->next;
         free(sym);
         sym = next;
     }
     free(old_scope);
     printf("[Semantic] Exit scope, current level: %d\n", sym_table.scope_level);
 }
 
 Symbol* lookup_symbol(const char* name) {
     Scope* scope = sym_table.current_scope;
     while (scope) {
         Symbol* sym = scope->symbols;
         while (sym) {
             if (strcmp(sym->name, name) == 0) return sym;
             sym = sym->next;
         }
         scope = scope->parent;
     }
     return NULL;
 }
 
 Symbol* add_symbol(const char* name, DataType type) {
     if (lookup_symbol(name)) {
         printf("[Error] Redeclaration: %s\n", name);
         error_count++;
         return NULL;
     }
     Symbol* sym = (Symbol*)malloc(sizeof(Symbol));
     strcpy(sym->name, name);
     sym->type = type;
     sym->scope_level = sym_table.scope_level;
     sym->is_initialized = 0;
     sym->next = sym_table.current_scope->symbols;
     sym_table.current_scope->symbols = sym;
     const char* type_str = (type == TYPE_INT) ? "int" : (type == TYPE_FLOAT) ? "float" : "void";
     printf("[Symbol] Added %s : %s (scope=%d)\n", name, type_str, sym_table.scope_level);
     return sym;
 }
 
 void print_symbol_table() {
     printf("\n========== Symbol Table ==========\n");
     Scope* scope = sym_table.current_scope;
     while (scope) {
         printf("Scope level %d:\n", scope->level);
         Symbol* sym = scope->symbols;
         while (sym) {
             const char* type_str = sym->type == TYPE_INT ? "int" : 
                                    sym->type == TYPE_FLOAT ? "float" : "void";
             printf("  %s : %s%s\n", sym->name, type_str, 
                    !sym->is_initialized ? " (uninitialized)" : "");
             sym = sym->next;
         }
         scope = scope->parent;
     }
     printf("=============================\n");
 }
 
 /* ==================== AST Operations ==================== */
 
 ASTNode* create_ast_node(ASTNodeType type) {
     ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
     memset(node, 0, sizeof(ASTNode));
     node->type = type;
     return node;
 }
 
 void add_child(ASTNode* parent, ASTNode* child) {
     if (parent && child && parent->child_count < MAX_CHILDREN) {
         parent->children[parent->child_count++] = child;
     }
 }
 
 ASTNode* create_var_decl_node(const char* name, DataType type) {
     ASTNode* node = create_ast_node(AST_VAR_DECL);
     strcpy(node->data.name, name);
     node->data_type = type;
     return node;
 }
 
 ASTNode* create_variable_node(const char* name, DataType type) {
     ASTNode* node = create_ast_node(AST_VARIABLE);
     strcpy(node->data.name, name);
     node->data_type = type;
     return node;
 }
 
 ASTNode* create_constant_node(int int_val, int is_int) {
     ASTNode* node = create_ast_node(AST_CONSTANT);
     node->data.constant.int_value = int_val;
     node->data.constant.is_int = 1;
     node->data_type = TYPE_INT;
     return node;
 }
 
 ASTNode* create_binary_node(const char* op, ASTNode* left, ASTNode* right) {
     ASTNode* node = create_ast_node(AST_BINARY_OP);
     strcpy(node->data.binary.op, op);
     node->data.binary.left = left;
     node->data.binary.right = right;
     if (left->data_type == TYPE_INT && right->data_type == TYPE_INT) {
         node->data_type = TYPE_INT;
     } else {
         node->data_type = TYPE_INT;
     }
     return node;
 }
 
 ASTNode* create_assignment_node(const char* var_name, ASTNode* expr) {
     ASTNode* node = create_ast_node(AST_ASSIGNMENT);
     strcpy(node->data.name, var_name);
     add_child(node, expr);
     node->data_type = expr->data_type;
     Symbol* sym = lookup_symbol(var_name);
     if (!sym) {
         printf("[Error] Undeclared variable: %s\n", var_name);
         error_count++;
     } else {
         sym->is_initialized = 1;
     }
     return node;
 }
 
 ASTNode* create_if_node() {
     return create_ast_node(AST_IF_STMT);
 }
 
 ASTNode* create_while_node() {
     return create_ast_node(AST_WHILE_STMT);
 }
 
 void print_ast(ASTNode* node, int indent) {
     if (!node) return;
     for (int i = 0; i < indent; i++) printf("  ");
     switch (node->type) {
         case AST_PROGRAM:
             printf("Program\n");
             for (int i = 0; i < node->child_count; i++)
                 print_ast(node->children[i], indent + 1);
             break;
         case AST_VAR_DECL:
             printf("VarDecl: %s : %s\n", node->data.name,
                    node->data_type == TYPE_INT ? "int" : "float");
             break;
         case AST_ASSIGNMENT:
             printf("Assignment: %s =\n", node->data.name);
             print_ast(node->children[0], indent + 1);
             break;
         case AST_BINARY_OP:
             printf("BinaryOp: %s\n", node->data.binary.op);
             print_ast(node->data.binary.left, indent + 1);
             print_ast(node->data.binary.right, indent + 1);
             break;
         case AST_VARIABLE:
             printf("Variable: %s\n", node->data.name);
             break;
         case AST_CONSTANT:
             printf("Const: %d\n", node->data.constant.int_value);
             break;
         case AST_IF_STMT:
             printf("IfStmt\n");
             break;
         case AST_WHILE_STMT:
             printf("WhileStmt\n");
             break;
         default:
             printf("Unknown\n");
             break;
     }
 }
 
 /* ==================== Lexical Analysis ==================== */
 
 int is_keyword(const char* str) {
     const char* keywords[] = {"int", "float", "void", "if", "else", "while", 
                                "return", "input", "print", NULL};
     for (int i = 0; keywords[i]; i++)
         if (strcmp(str, keywords[i]) == 0) return 1;
     return 0;
 }
 
 TokenType get_keyword_type(const char* str) {
     if (strcmp(str, "int") == 0) return TOKEN_INT;
     if (strcmp(str, "float") == 0) return TOKEN_FLOAT_TYPE;
     if (strcmp(str, "void") == 0) return TOKEN_VOID;
     if (strcmp(str, "if") == 0) return TOKEN_IF;
     if (strcmp(str, "else") == 0) return TOKEN_ELSE;
     if (strcmp(str, "while") == 0) return TOKEN_WHILE;
     if (strcmp(str, "return") == 0) return TOKEN_RETURN;
     return TOKEN_ID;
 }
 
 TokenType get_operator_type(char ch, char next) {
    // Relational operators
    if (ch == '>') {
        if (next == '=') return TOKEN_GE;
        return TOKEN_GT;
    }
    if (ch == '<') {
        if (next == '=') return TOKEN_LE;
        return TOKEN_LT;
    }
    if (ch == '=') {
        if (next == '=') return TOKEN_EQ;
        return TOKEN_ASSIGN;
    }
    if (ch == '!') {
        if (next == '=') return TOKEN_NE;
        return TOKEN_NOT;
    }
    
    // Logical operators
    if (ch == '&' && next == '&') return TOKEN_AND;
    if (ch == '|' && next == '|') return TOKEN_OR;
    
    // Arithmetic operators
    if (ch == '+') {
        if (next == '+') return TOKEN_INC;
        if (next == '=') return TOKEN_PLUS_ASSIGN;
        return TOKEN_PLUS;
    }
    if (ch == '-') {
        if (next == '-') return TOKEN_INC;
        if (next == '=') return TOKEN_PLUS_ASSIGN;
        return TOKEN_MINUS;
    }
    if (ch == '*') {
        if (next == '=') return TOKEN_PLUS_ASSIGN;  // *=
        return TOKEN_MUL;
    }
    if (ch == '/') {
        if (next == '=') return TOKEN_PLUS_ASSIGN;  // /=
        return TOKEN_DIV;
    }
    
    return TOKEN_ERROR;
}
 
 void lex_analyze(const char* filename) {
     FILE* fp = fopen(filename, "r");
     if (!fp) {
         printf("Error: Cannot open file %s\n", filename);
         return;
     }
     
     char ch, next;
     char buffer[MAX_SYMBOL_LEN];
     int buf_pos, line = 1;
     
     printf("\n========== Lexical Analysis ==========\n");
     
     while ((ch = fgetc(fp)) != EOF) {
         if (isspace(ch)) {
             if (ch == '\n') line++;
             continue;
         }
         
         // Identifier or keyword
         if (isalpha(ch) || ch == '_') {
             buf_pos = 0;
             buffer[buf_pos++] = ch;
             while (isalnum(ch = fgetc(fp)) || ch == '_')
                 buffer[buf_pos++] = ch;
             buffer[buf_pos] = '\0';
             ungetc(ch, fp);
             
             Token token;
             token.line = line;
             if (is_keyword(buffer)) {
                 token.type = get_keyword_type(buffer);
                 printf("  %s : KEYWORD\n", buffer);
             } else {
                 token.type = TOKEN_ID;
                 printf("  %s : IDENTIFIER\n", buffer);
             }
             strcpy(token.lexeme, buffer);
             tokens[token_count++] = token;
             continue;
         }
         
         // Number
         if (isdigit(ch)) {
             buf_pos = 0;
             buffer[buf_pos++] = ch;
             while (isdigit(ch = fgetc(fp)))
                 buffer[buf_pos++] = ch;
             buffer[buf_pos] = '\0';
             ungetc(ch, fp);
             
             Token token;
             token.line = line;
             token.type = TOKEN_NUM;
             token.value.int_value = atoi(buffer);
             printf("  %s : INTEGER (%d)\n", buffer, token.value.int_value);
             strcpy(token.lexeme, buffer);
             tokens[token_count++] = token;
             continue;
         }
         
         // Operators and delimiters
         next = fgetc(fp);
         TokenType op_type = get_operator_type(ch, next);
         
         if (op_type != TOKEN_ERROR) {
             Token token;
             token.line = line;
             token.type = op_type;
             token.lexeme[0] = ch;
             if (op_type == TOKEN_GE || op_type == TOKEN_LE || op_type == TOKEN_EQ ||
                 op_type == TOKEN_NE || op_type == TOKEN_AND || op_type == TOKEN_OR ||
                 op_type == TOKEN_INC || op_type == TOKEN_PLUS_ASSIGN) {
                 token.lexeme[1] = next;
                 token.lexeme[2] = '\0';
                 printf("  %s : OPERATOR\n", token.lexeme);
             } else {
                 token.lexeme[1] = '\0';
                 printf("  %c : OPERATOR\n", ch);
             }
             tokens[token_count++] = token;
         } else {
             // Single character tokens
             Token token;
             token.line = line;
             token.lexeme[0] = ch;
             token.lexeme[1] = '\0';
             ungetc(next, fp);
             
             switch (ch) {
                 case '(': token.type = TOKEN_LPAREN; printf("  ( : LPAREN\n"); break;
                 case ')': token.type = TOKEN_RPAREN; printf("  ) : RPAREN\n"); break;
                 case '{': token.type = TOKEN_LBRACE; printf("  { : LBRACE\n"); enter_scope(); break;
                 case '}': token.type = TOKEN_RBRACE; printf("  } : RBRACE\n"); exit_scope(); break;
                 case ',': token.type = TOKEN_COMMA; printf("  , : COMMA\n"); break;
                 case ';': token.type = TOKEN_SEMICOLON; printf("  ; : SEMICOLON\n"); break;
                 default: token.type = TOKEN_ERROR; printf("  %c : UNKNOWN\n", ch); break;
             }
             tokens[token_count++] = token;
         }
     }
     
     Token eof_token;
     eof_token.type = TOKEN_EOF;
     strcpy(eof_token.lexeme, "EOF");
     tokens[token_count++] = eof_token;
     
     fclose(fp);
     printf("\nLexical analysis complete, %d tokens\n", token_count);
 }
 
 /* ==================== Parser ==================== */
 
 void parse_program() {
     printf("\n========== SLR(1) Parsing ==========\n");
     
     token_pos = 0;
     ASTNode* program_node = create_ast_node(AST_PROGRAM);
     
     printf("\n%-8s %-30s %-25s\n", "Step", "Action", "Details");
     printf("----------------------------------------------------------\n");
     
     int step = 0;
     
     while (token_pos < token_count) {
         Token current = tokens[token_pos];
         step++;
         
         if (current.type == TOKEN_EOF) {
             printf("%-8d %-30s %-25s\n", step, "Accept", "EOF");
             break;
         }
         
         // Variable declaration: type id ;
         if (current.type == TOKEN_INT || current.type == TOKEN_FLOAT_TYPE) {
             DataType var_type = (current.type == TOKEN_INT) ? TYPE_INT : TYPE_FLOAT;
             token_pos++;
             
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_ID) {
                 const char* var_name = tokens[token_pos].lexeme;
                 token_pos++;
                 
                 if (token_pos < token_count && tokens[token_pos].type == TOKEN_SEMICOLON) {
                     token_pos++;
                     add_symbol(var_name, var_type);
                     ASTNode* var_node = create_var_decl_node(var_name, var_type);
                     add_child(program_node, var_node);
                     printf("%-8d %-30s %-25s\n", step, "VarDecl", var_name);
                 }
             }
         }
         // Assignment: id = expression ;
         else if (current.type == TOKEN_ID) {
             const char* var_name = current.lexeme;
             token_pos++;
             
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_ASSIGN) {
                 token_pos++;
                 
                 // Parse expression (simplified)
                 ASTNode* expr_node = NULL;
                 
                 if (token_pos < token_count) {
                     if (tokens[token_pos].type == TOKEN_NUM) {
                         expr_node = create_constant_node(tokens[token_pos].value.int_value, 1);
                         printf("%-8d %-30s %-25s\n", step, "Constant", tokens[token_pos].lexeme);
                         token_pos++;
                     }
                     else if (tokens[token_pos].type == TOKEN_ID) {
                         const char* ref_name = tokens[token_pos].lexeme;
                         Symbol* sym = lookup_symbol(ref_name);
                         DataType ref_type = sym ? sym->type : TYPE_ERROR;
                         expr_node = create_variable_node(ref_name, ref_type);
                         printf("%-8d %-30s %-25s\n", step, "VarRef", ref_name);
                         token_pos++;
                         
                         // Check for binary operator
                         if (token_pos < token_count && 
                             (tokens[token_pos].type == TOKEN_PLUS ||
                              tokens[token_pos].type == TOKEN_MINUS ||
                              tokens[token_pos].type == TOKEN_MUL ||
                              tokens[token_pos].type == TOKEN_DIV)) {
                             char op = tokens[token_pos].lexeme[0];
                             token_pos++;
                             
                             ASTNode* right_node = NULL;
                             if (token_pos < token_count && tokens[token_pos].type == TOKEN_NUM) {
                                 right_node = create_constant_node(tokens[token_pos].value.int_value, 1);
                                 printf("%-8d %-30s %-25s\n", step, "Constant", tokens[token_pos].lexeme);
                                 token_pos++;
                             } else if (token_pos < token_count && tokens[token_pos].type == TOKEN_ID) {
                                 right_node = create_variable_node(tokens[token_pos].lexeme, TYPE_INT);
                                 printf("%-8d %-30s %-25s\n", step, "VarRef", tokens[token_pos].lexeme);
                                 token_pos++;
                             }
                             
                             if (right_node) {
                                 char op_str[2] = {op, '\0'};
                                 expr_node = create_binary_node(op_str, expr_node, right_node);
                                 printf("%-8d %-30s %-25s\n", step, "BinaryOp", op_str);
                             }
                         }
                     }
                 }
                 
                 // Check for semicolon
                 if (token_pos < token_count && tokens[token_pos].type == TOKEN_SEMICOLON) {
                     token_pos++;
                 }
                 
                 if (expr_node) {
                     ASTNode* assign_node = create_assignment_node(var_name, expr_node);
                     add_child(program_node, assign_node);
                     printf("%-8d %-30s %-25s\n", step, "Assignment", var_name);
                 }
             } else {
                 token_pos++;
             }
         }
         // If statement
         else if (current.type == TOKEN_IF) {
             token_pos++;
             // Skip condition
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_LPAREN) {
                 token_pos++;
                 while (token_pos < token_count && tokens[token_pos].type != TOKEN_RPAREN)
                     token_pos++;
                 if (token_pos < token_count) token_pos++;
             }
             // Skip body
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACE) {
                 int brace_count = 1;
                 token_pos++;
                 while (token_pos < token_count && brace_count > 0) {
                     if (tokens[token_pos].type == TOKEN_LBRACE) brace_count++;
                     if (tokens[token_pos].type == TOKEN_RBRACE) brace_count--;
                     token_pos++;
                 }
             }
             ASTNode* if_node = create_if_node();
             add_child(program_node, if_node);
             printf("%-8d %-30s %-25s\n", step, "IfStmt", "");
         }
         // While statement
         else if (current.type == TOKEN_WHILE) {
             token_pos++;
             // Skip condition
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_LPAREN) {
                 token_pos++;
                 while (token_pos < token_count && tokens[token_pos].type != TOKEN_RPAREN)
                     token_pos++;
                 if (token_pos < token_count) token_pos++;
             }
             // Skip body
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACE) {
                 int brace_count = 1;
                 token_pos++;
                 while (token_pos < token_count && brace_count > 0) {
                     if (tokens[token_pos].type == TOKEN_LBRACE) brace_count++;
                     if (tokens[token_pos].type == TOKEN_RBRACE) brace_count--;
                     token_pos++;
                 }
             }
             ASTNode* while_node = create_while_node();
             add_child(program_node, while_node);
             printf("%-8d %-30s %-25s\n", step, "WhileStmt", "");
         }
         else {
             token_pos++;
         }
     }
     
     ast_root = program_node;
     printf("\nParsing complete\n");
 }
 
 /* ==================== Main ==================== */
 
 int main(int argc, char** argv) {
     printf("========================================\n");
     printf("  Lab 5: SLR-Guided Semantic Analysis\n");
     printf("========================================\n");
     
     init_symbol_table();
     const char* filename = (argc > 1) ? argv[1] : "test.c";
     lex_analyze(filename);
     parse_program();
     print_symbol_table();
     
     printf("\n========== Abstract Syntax Tree (AST) ==========\n");
     print_ast(ast_root, 0);
     
     printf("\n========== Semantic Analysis Result ==========\n");
     if (error_count == 0)
         printf("[OK] Semantic analysis complete, no errors\n");
     else
         printf("[FAIL] Semantic analysis complete, %d errors found\n", error_count);
     
     return 0;
 }