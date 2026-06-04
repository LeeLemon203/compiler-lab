/**
 * Lab 6: Intermediate Code Generation (Three-Address Code / Quadruple)
 * 
 * Compile: gcc experiment6.c -o exp6
 * Run: ./exp6 test.c
 * 
 * Output: Quadruple (op, arg1, arg2, result)
 * 
 * 支持的文法：
 * - 类型：int, float, void
 * - 控制流：if, else, while, return
 * - 输入输出：input, print
 * - 运算符：+, -, *, /, <, <=, >, >=, ==, !=, &&, ||, !
 * - 界符：(, ), [, ], {, }, ,, ;
 * - 不支持：for, 双引号, 单引号, \, 注释
 * - main是ID不是关键字
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
 #define MAX_QUADS 1000
 #define MAX_TEMP_VARS 100
 #define MAX_FUNCTIONS 100
 
 /* ==================== Lexical Analysis ==================== */
 
 typedef enum {
     TOKEN_ID, TOKEN_NUM, TOKEN_FLOAT,
     TOKEN_INT, TOKEN_FLOAT_TYPE, TOKEN_VOID,
     TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_RETURN,
     TOKEN_INPUT, TOKEN_PRINT,
     TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV,
     TOKEN_ASSIGN, TOKEN_PLUS_ASSIGN, TOKEN_MINUS_ASSIGN,
     TOKEN_MUL_ASSIGN, TOKEN_DIV_ASSIGN,
     TOKEN_INC, TOKEN_DEC,
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
     int is_array;
     int array_size;
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
 
 /* ==================== Function Table ==================== */
 
 typedef struct {
     char name[MAX_SYMBOL_LEN];
     DataType return_type;
     int is_declared;
 } FunctionEntry;
 
 /* ==================== AST ==================== */
 
 typedef enum {
     AST_PROGRAM, AST_VAR_DECL, AST_ASSIGNMENT, AST_BINARY_OP,
     AST_VARIABLE, AST_CONSTANT, AST_IF_STMT, AST_WHILE_STMT, 
     AST_BLOCK, AST_RETURN_STMT, AST_FUNC_CALL, AST_ARRAY_ACCESS,
     AST_FUNC_DEF, AST_UNARY_OP, AST_PRINT_STMT
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
         struct {
             char op[4];
             struct ASTNode* operand;
         } unary;
         struct {
             char func_name[MAX_SYMBOL_LEN];
             struct ASTNode* args[MAX_CHILDREN];
             int arg_count;
         } call;
         struct {
             char array_name[MAX_SYMBOL_LEN];
             struct ASTNode* index;
         } array;
     } data;
     struct ASTNode* next;
 } ASTNode;
 
 /* ==================== Intermediate Code ==================== */
 
 typedef enum {
     QUAD_ADD, QUAD_SUB, QUAD_MUL, QUAD_DIV,
     QUAD_ASSIGN, QUAD_LABEL, QUAD_GOTO, QUAD_IF_GOTO,
     QUAD_RETURN, QUAD_CALL, QUAD_PARAM, QUAD_FUNC_START,
     QUAD_LT, QUAD_LE, QUAD_GT, QUAD_GE, QUAD_EQ, QUAD_NE,
     QUAD_AND, QUAD_OR, QUAD_NOT, QUAD_NEG
 } QuadOp;
 
 typedef struct {
     QuadOp op;
     char arg1[MAX_SYMBOL_LEN];
     char arg2[MAX_SYMBOL_LEN];
     char result[MAX_SYMBOL_LEN];
 } Quadruple;
 
 /* ==================== Global Variables ==================== */
 
 Token tokens[MAX_STACK];
 int token_count = 0;
 int token_pos = 0;
 
 SymbolTable sym_table;
 int error_count = 0;
 ASTNode* ast_root = NULL;
 
 Quadruple quads[MAX_QUADS];
 int quad_count = 0;
 int temp_counter = 0;
 int label_counter = 0;
 
 // Track current function return type for type checking
 DataType current_function_return_type = TYPE_VOID;
 
 // Function table
 FunctionEntry function_table[MAX_FUNCTIONS];
 int function_count = 0;
 
 // Forward declarations
 void lex_analyze(const char* filename);
 void parse_program(void);
 void parse_block(ASTNode* block_node);
 void parse_statement(ASTNode* parent);
 int parse_var_decl(ASTNode* parent, DataType var_type);
 ASTNode* parse_expression(void);
 ASTNode* parse_primary(void);
 char* generate_code(ASTNode* node);
 char* new_temp(void);
 char* new_label(void);
 void emit(QuadOp op, const char* arg1, const char* arg2, const char* result);
 QuadOp get_quad_op(const char* op);
 void print_quadruples(void);
 void print_three_address_code(void);
 int is_operator_token(TokenType type);
 void init_symbol_table(void);
 void enter_scope(void);
 void exit_scope(void);
 Symbol* lookup_symbol(const char* name);
 Symbol* add_symbol(const char* name, DataType type);
 void print_symbol_table(void);
 void add_function(const char* name, DataType return_type);
 int lookup_function(const char* name);
 void print_function_table(void);
 ASTNode* create_ast_node(ASTNodeType type);
 void add_child(ASTNode* parent, ASTNode* child);
 ASTNode* create_var_decl_node(const char* name, DataType type);
 ASTNode* create_variable_node(const char* name, DataType type);
 ASTNode* create_constant_node(int int_val, int is_int);
 ASTNode* create_float_node(float float_val);
 ASTNode* create_binary_node(const char* op, ASTNode* left, ASTNode* right);
 ASTNode* create_assignment_node(const char* var_name, ASTNode* expr);
 ASTNode* create_if_node(void);
 ASTNode* create_while_node(void);
 ASTNode* create_return_node(ASTNode* expr);
 ASTNode* create_func_call_node(const char* name);
 void add_func_arg(ASTNode* call_node, ASTNode* arg);
 ASTNode* create_unary_node(const char* op, ASTNode* operand);
 void print_ast(ASTNode* node, int indent);
 void gen_assignment(ASTNode* node);
 void gen_var_decl(ASTNode* node);
 void gen_if_stmt(ASTNode* node);
 void gen_while_stmt(ASTNode* node);
 void gen_return_stmt(ASTNode* node);
 void gen_func_call(ASTNode* node);
 char* gen_const(ASTNode* node);
 char* gen_var(ASTNode* node);
 char* gen_unary_op(ASTNode* node);
 char* gen_binary_op(ASTNode* node);
 
 /* ==================== Function Table Operations ==================== */
 
 void add_function(const char* name, DataType return_type) {
     // Check for redefinition
     for (int i = 0; i < function_count; i++) {
         if (strcmp(function_table[i].name, name) == 0) {
             printf("[Error] Function redefinition: %s\n", name);
             error_count++;
             return;
         }
     }
     if (function_count < MAX_FUNCTIONS) {
         strcpy(function_table[function_count].name, name);
         function_table[function_count].return_type = return_type;
         function_table[function_count].is_declared = 1;
         function_count++;
     }
 }
 
 int lookup_function(const char* name) {
     for (int i = 0; i < function_count; i++) {
         if (strcmp(function_table[i].name, name) == 0) {
             return 1;
         }
     }
     return 0;
 }
 
 void print_function_table() {
     printf("\n========== Function Table ==========\n");
     for (int i = 0; i < function_count; i++) {
         const char* type_str = function_table[i].return_type == TYPE_INT ? "int" : 
                                 function_table[i].return_type == TYPE_FLOAT ? "float" : "void";
         printf("  %s : %s\n", function_table[i].name, type_str);
     }
     if (function_count == 0) {
         printf("  (empty)\n");
     }
     printf("=====================================\n");
 }
 
 /* ==================== Symbol Table Operations ==================== */
 
 void init_symbol_table() {
     sym_table.current_scope = (Scope*)malloc(sizeof(Scope));
     sym_table.current_scope->symbols = NULL;
     sym_table.current_scope->level = 0;
     sym_table.current_scope->parent = NULL;
     sym_table.scope_level = 0;
 }
 
 void enter_scope() {
     Scope* new_scope = (Scope*)malloc(sizeof(Scope));
     new_scope->symbols = NULL;
     new_scope->level = sym_table.scope_level + 1;
     new_scope->parent = sym_table.current_scope;
     sym_table.current_scope = new_scope;
     sym_table.scope_level++;
 }
 
 void exit_scope() {
     if (sym_table.current_scope->parent == NULL) return;
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
     sym->is_array = 0;
     sym->array_size = 0;
     sym->next = sym_table.current_scope->symbols;
     sym_table.current_scope->symbols = sym;
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
             printf("  %s : %s%s%s\n", sym->name, type_str,
                    sym->is_array ? "[]" : "",
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
     node->data.constant.is_int = is_int;
     node->data_type = TYPE_INT;
     return node;
 }
 
 ASTNode* create_float_node(float float_val) {
     ASTNode* node = create_ast_node(AST_CONSTANT);
     node->data.constant.float_value = float_val;
     node->data.constant.is_int = 0;
     node->data_type = TYPE_FLOAT;
     return node;
 }
 
 ASTNode* create_binary_node(const char* op, ASTNode* left, ASTNode* right) {
     ASTNode* node = create_ast_node(AST_BINARY_OP);
     strcpy(node->data.binary.op, op);
     node->data.binary.left = left;
     node->data.binary.right = right;
     if (left && right) {
         if (left->data_type == TYPE_INT && right->data_type == TYPE_INT) {
             node->data_type = TYPE_INT;
         } else {
             node->data_type = TYPE_FLOAT;
         }
     } else {
         node->data_type = TYPE_INT;
     }
     return node;
 }
 
 ASTNode* create_unary_node(const char* op, ASTNode* operand) {
     ASTNode* node = create_ast_node(AST_UNARY_OP);
     strcpy(node->data.unary.op, op);
     node->data.unary.operand = operand;
     node->data_type = operand ? operand->data_type : TYPE_INT;
     return node;
 }
 
 ASTNode* create_assignment_node(const char* var_name, ASTNode* expr) {
     ASTNode* node = create_ast_node(AST_ASSIGNMENT);
     strcpy(node->data.name, var_name);
     add_child(node, expr);
     if (expr) {
         node->data_type = expr->data_type;
     }
     Symbol* sym = lookup_symbol(var_name);
     if (!sym) {
         printf("[Error] Undeclared variable: %s\n", var_name);
         error_count++;
     } else {
         sym->is_initialized = 1;
     }
     return node;
 }
 
 ASTNode* create_if_node(void) {
     return create_ast_node(AST_IF_STMT);
 }
 
 ASTNode* create_while_node(void) {
     return create_ast_node(AST_WHILE_STMT);
 }
 
 ASTNode* create_return_node(ASTNode* expr) {
     ASTNode* node = create_ast_node(AST_RETURN_STMT);
     if (expr) {
         add_child(node, expr);
     }
     return node;
 }
 
 ASTNode* create_func_call_node(const char* name) {
     ASTNode* node = create_ast_node(AST_FUNC_CALL);
     strcpy(node->data.call.func_name, name);
     node->data.call.arg_count = 0;
     return node;
 }
 
 void add_func_arg(ASTNode* call_node, ASTNode* arg) {
     if (call_node && arg && call_node->data.call.arg_count < MAX_CHILDREN) {
         call_node->data.call.args[call_node->data.call.arg_count++] = arg;
     }
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
             printf("VarDecl: %s\n", node->data.name);
             break;
         case AST_ASSIGNMENT:
             printf("Assignment: %s =\n", node->data.name);
             if (node->child_count > 0) print_ast(node->children[0], indent + 1);
             break;
         case AST_BINARY_OP:
             printf("BinaryOp: %s\n", node->data.binary.op);
             if (node->data.binary.left) print_ast(node->data.binary.left, indent + 1);
             if (node->data.binary.right) print_ast(node->data.binary.right, indent + 1);
             break;
         case AST_VARIABLE:
             printf("Variable: %s\n", node->data.name);
             break;
         case AST_CONSTANT:
             if (node->data.constant.is_int)
                 printf("Const: %d\n", node->data.constant.int_value);
             else
                 printf("Const: %g\n", node->data.constant.float_value);
             break;
         case AST_IF_STMT:
             printf("IfStmt\n");
             for (int i = 0; i < node->child_count; i++)
                 print_ast(node->children[i], indent + 1);
             break;
         case AST_WHILE_STMT:
             printf("WhileStmt\n");
             for (int i = 0; i < node->child_count; i++)
                 print_ast(node->children[i], indent + 1);
             break;
         case AST_RETURN_STMT:
             printf("Return\n");
             if (node->child_count > 0) print_ast(node->children[0], indent + 1);
             break;
         case AST_FUNC_CALL:
             printf("FuncCall: %s\n", node->data.call.func_name);
             break;
         case AST_BLOCK:
             printf("Block\n");
             for (int i = 0; i < node->child_count; i++)
                 print_ast(node->children[i], indent + 1);
             break;
         case AST_UNARY_OP:
             printf("UnaryOp: %s\n", node->data.unary.op);
             if (node->data.unary.operand) print_ast(node->data.unary.operand, indent + 1);
             break;
         case AST_PRINT_STMT:
             printf("Print\n");
             if (node->child_count > 0) print_ast(node->children[0], indent + 1);
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
     if (strcmp(str, "input") == 0) return TOKEN_INPUT;
     if (strcmp(str, "print") == 0) return TOKEN_PRINT;
     return TOKEN_ID;
 }
 
 int is_operator_token(TokenType type) {
     return (type == TOKEN_PLUS || type == TOKEN_MINUS || type == TOKEN_MUL || 
             type == TOKEN_DIV || type == TOKEN_LT || type == TOKEN_LE || 
             type == TOKEN_GT || type == TOKEN_GE || type == TOKEN_EQ || 
             type == TOKEN_NE || type == TOKEN_AND || type == TOKEN_OR);
 }
 
 TokenType get_operator_type(char ch, char next) {
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
     if (ch == '&' && next == '&') {
         return TOKEN_AND;
     }
     if (ch == '|' && next == '|') {
         return TOKEN_OR;
     }
     
     if (ch == '+') {
         if (next == '+') return TOKEN_INC;
         if (next == '=') return TOKEN_PLUS_ASSIGN;
         return TOKEN_PLUS;
     }
     if (ch == '-') {
         if (next == '-') return TOKEN_DEC;
         if (next == '=') return TOKEN_MINUS_ASSIGN;
         return TOKEN_MINUS;
     }
     if (ch == '*') {
         if (next == '=') return TOKEN_MUL_ASSIGN;
         return TOKEN_MUL;
     }
     if (ch == '/') {
         if (next == '=') return TOKEN_DIV_ASSIGN;
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
     token_count = 0;
     
     while ((ch = fgetc(fp)) != EOF) {
         if (isspace(ch)) {
             if (ch == '\n') line++;
             continue;
         }
         
         // Handle comments
         if (ch == '/') {
             next = fgetc(fp);
             if (next == '/') {
                 // Single line comment
                 while ((ch = fgetc(fp)) != EOF && ch != '\n');
                 if (ch == '\n') line++;
                 continue;
             } else if (next == '*') {
                 // Multi-line comment
                 int prev = 0;
                 while ((ch = fgetc(fp)) != EOF) {
                     if (ch == '\n') line++;
                     if (prev == '*' && ch == '/') break;
                     prev = ch;
                 }
                 continue;
             } else {
                 ungetc(next, fp);
             }
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
         
         // Number (integer or float)
         if (isdigit(ch)) {
             buf_pos = 0;
             buffer[buf_pos++] = ch;
             int is_float = 0;
             
             while (isdigit(ch = fgetc(fp)) || ch == '.') {
                 if (ch == '.') {
                     if (is_float) {
                         ungetc(ch, fp);
                         break;
                     }
                     is_float = 1;
                 }
                 buffer[buf_pos++] = ch;
             }
             buffer[buf_pos] = '\0';
             ungetc(ch, fp);
             
             Token token;
             token.line = line;
             if (is_float) {
                 token.type = TOKEN_FLOAT;
                 token.value.float_value = atof(buffer);
                 printf("  %s : FLOAT\n", buffer);
             } else {
                 token.type = TOKEN_NUM;
                 token.value.int_value = atoi(buffer);
                 printf("  %s : INTEGER\n", buffer);
             }
             strcpy(token.lexeme, buffer);
             tokens[token_count++] = token;
             continue;
         }
         
         // String literal - not supported, report error
         if (ch == '"') {
             printf("  \" : UNSUPPORTED (string literal not supported)\n");
             error_count++;
             while ((ch = fgetc(fp)) != EOF && ch != '"');
             continue;
         }
         
         // Character literal - not supported
         if (ch == '\'') {
             printf("  \' : UNSUPPORTED (char literal not supported)\n");
             error_count++;
             while ((ch = fgetc(fp)) != EOF && ch != '\'');
             continue;
         }
         
         // Backslash - not supported
         if (ch == '\\') {
             printf("  \\ : UNSUPPORTED (backslash not supported)\n");
             error_count++;
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
                 op_type == TOKEN_INC || op_type == TOKEN_DEC ||
                 op_type == TOKEN_PLUS_ASSIGN || op_type == TOKEN_MINUS_ASSIGN ||
                 op_type == TOKEN_MUL_ASSIGN || op_type == TOKEN_DIV_ASSIGN) {
                 token.lexeme[1] = next;
                 token.lexeme[2] = '\0';
                 printf("  %s : OPERATOR\n", token.lexeme);
             } else {
                 token.lexeme[1] = '\0';
                 printf("  %c : OPERATOR\n", ch);
                 ungetc(next, fp);
             }
             
             // Check for unsupported operators
             if (op_type == TOKEN_AND || op_type == TOKEN_OR) {
                 printf("  %s : UNSUPPORTED (boolean operators not supported)\n", token.lexeme);
                 error_count++;
             }
             
             tokens[token_count++] = token;
         } else {
             Token token;
             token.line = line;
             token.lexeme[0] = ch;
             token.lexeme[1] = '\0';
             ungetc(next, fp);
             
             switch (ch) {
                 case '(': token.type = TOKEN_LPAREN; printf("  ( : LPAREN\n"); break;
                 case ')': token.type = TOKEN_RPAREN; printf("  ) : RPAREN\n"); break;
                 case '[': token.type = TOKEN_LBRACKET; printf("  [ : LBRACKET\n"); break;
                 case ']': token.type = TOKEN_RBRACKET; printf("  ] : RBRACKET\n"); break;
                 case '{': token.type = TOKEN_LBRACE; printf("  { : LBRACE\n"); break;
                 case '}': token.type = TOKEN_RBRACE; printf("  } : RBRACE\n"); break;
                 case ',': token.type = TOKEN_COMMA; printf("  , : COMMA\n"); break;
                 case ';': token.type = TOKEN_SEMICOLON; printf("  ; : SEMICOLON\n"); break;
                 default: 
                     token.type = TOKEN_ERROR; 
                     printf("  %c : UNKNOWN\n", ch); 
                     break;
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
 
 ASTNode* parse_primary(void) {
     if (token_pos >= token_count) return NULL;
     
     ASTNode* node = NULL;
     
     // Handle negative number or unary minus
     if (tokens[token_pos].type == TOKEN_MINUS) {
         token_pos++;
         ASTNode* operand = parse_primary();
         if (operand) {
             if (operand->type == AST_CONSTANT) {
                 if (operand->data.constant.is_int) {
                     operand->data.constant.int_value = -operand->data.constant.int_value;
                     node = operand;
                 } else {
                     operand->data.constant.float_value = -operand->data.constant.float_value;
                     node = operand;
                 }
             } else {
                 node = create_unary_node("-", operand);
             }
         }
         return node;
     }
     
     if (tokens[token_pos].type == TOKEN_NUM) {
         node = create_constant_node(tokens[token_pos].value.int_value, 1);
         token_pos++;
     } else if (tokens[token_pos].type == TOKEN_FLOAT) {
         node = create_float_node(tokens[token_pos].value.float_value);
         token_pos++;
     } else if (tokens[token_pos].type == TOKEN_ID) {
         char name[MAX_SYMBOL_LEN];
         strcpy(name, tokens[token_pos].lexeme);
         token_pos++;
         
         // Check for array access
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACKET) {
             token_pos++;
             ASTNode* index = parse_expression();
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_RBRACKET) {
                 token_pos++;
             }
             // Check if variable is declared
             Symbol* sym = lookup_symbol(name);
             if (!sym) {
                 printf("[Error] Undeclared variable: %s\n", name);
                 error_count++;
             }
             node = create_variable_node(name, TYPE_INT);
         } else if (token_pos < token_count && tokens[token_pos].type == TOKEN_LPAREN) {
             // Function call in expression
             if (!lookup_function(name)) {
                 Symbol* sym = lookup_symbol(name);
                 if (sym) {
                     printf("[Error] %s is a variable, not a function\n", name);
                 } else {
                     printf("[Error] Undeclared function: %s\n", name);
                 }
                 error_count++;
             }
             node = create_func_call_node(name);
             token_pos++;
             while (token_pos < token_count && tokens[token_pos].type != TOKEN_RPAREN) {
                 if (tokens[token_pos].type == TOKEN_COMMA) {
                     token_pos++;
                     continue;
                 }
                 ASTNode* arg = parse_expression();
                 if (arg) {
                     add_func_arg(node, arg);
                 }
             }
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_RPAREN) {
                 token_pos++;
             }
         } else {
             Symbol* sym = lookup_symbol(name);
             if (!sym) {
                 printf("[Error] Undeclared variable: %s\n", name);
                 error_count++;
             }
             DataType type = sym ? sym->type : TYPE_INT;
             node = create_variable_node(name, type);
         }
     } else if (tokens[token_pos].type == TOKEN_LPAREN) {
         token_pos++;
         node = parse_expression();
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_RPAREN) {
             token_pos++;
         }
     } else if (tokens[token_pos].type == TOKEN_NOT) {
         token_pos++;
         ASTNode* operand = parse_primary();
         if (operand) {
             node = create_unary_node("!", operand);
         }
     }
     
     return node;
 }
 
 ASTNode* parse_expression(void) {
     ASTNode* left = parse_primary();
     if (!left) return NULL;
     
     // Check for assignment (for if conditions like if (a = 3))
     if (token_pos < token_count && tokens[token_pos].type == TOKEN_ASSIGN) {
         token_pos++;
         ASTNode* right = parse_expression();
         if (right) {
             return right;
         }
     }
     
     // Handle binary operators (loop for multiple operators)
     while (token_pos < token_count && is_operator_token(tokens[token_pos].type)) {
         char op[4];
         TokenType op_type = tokens[token_pos].type;
         
         switch (op_type) {
             case TOKEN_PLUS: strcpy(op, "+"); break;
             case TOKEN_MINUS: strcpy(op, "-"); break;
             case TOKEN_MUL: strcpy(op, "*"); break;
             case TOKEN_DIV: strcpy(op, "/"); break;
             case TOKEN_LT: strcpy(op, "<"); break;
             case TOKEN_LE: strcpy(op, "<="); break;
             case TOKEN_GT: strcpy(op, ">"); break;
             case TOKEN_GE: strcpy(op, ">="); break;
             case TOKEN_EQ: strcpy(op, "=="); break;
             case TOKEN_NE: strcpy(op, "!="); break;
             case TOKEN_AND: strcpy(op, "&&"); break;
             case TOKEN_OR: strcpy(op, "||"); break;
             default: strcpy(op, "?"); break;
         }
         token_pos++;
         
         ASTNode* right = parse_primary();
         if (right) {
             left = create_binary_node(op, left, right);
         }
     }
     
     return left;
 }
 
 // Parse a block of statements
 void parse_block(ASTNode* block_node) {
     int max_iterations = 10000;
     int iteration_count = 0;
     
     while (token_pos < token_count && tokens[token_pos].type != TOKEN_RBRACE) {
         if (++iteration_count > max_iterations) {
             printf("[Error] Block parsing timeout\n");
             error_count++;
             break;
         }
         parse_statement(block_node);
     }
     if (token_pos < token_count && tokens[token_pos].type == TOKEN_RBRACE) {
         token_pos++;
     }
 }
 
 // Parse a variable declaration
 int parse_var_decl(ASTNode* parent, DataType var_type) {
     if (token_pos < token_count && tokens[token_pos].type == TOKEN_ID) {
         const char* var_name = tokens[token_pos].lexeme;
         token_pos++;
         
         int is_array = 0;
         int array_size = 0;
         
         // Handle array
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACKET) {
             is_array = 1;
             token_pos++;
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_NUM) {
                 array_size = tokens[token_pos].value.int_value;
                 token_pos++;
             }
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_RBRACKET) {
                 token_pos++;
             }
         }
         
         // Add symbol first
         Symbol* sym = add_symbol(var_name, var_type);
         if (sym) {
             sym->is_array = is_array;
             sym->array_size = array_size;
         }
         
         // Initialization
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_ASSIGN) {
             token_pos++;
             ASTNode* init_expr = parse_expression();
             
             if (sym) {
                 sym->is_initialized = 1;
             }
             
             if (init_expr) {
                 ASTNode* assign_node = create_assignment_node(var_name, init_expr);
                 add_child(parent, assign_node);
             }
             return 1;
         }
         // Just declaration
         else {
             ASTNode* var_node = create_var_decl_node(var_name, var_type);
             add_child(parent, var_node);
             return 1;
         }
     }
     return 0;
 }
 
 // Parse a single statement
 void parse_statement(ASTNode* parent) {
     if (token_pos >= token_count) return;
     
     Token current = tokens[token_pos];
     
     // Variable declaration: int/float id [= expr];
     if (current.type == TOKEN_INT || current.type == TOKEN_FLOAT_TYPE) {
         DataType var_type = (current.type == TOKEN_INT) ? TYPE_INT : TYPE_FLOAT;
         token_pos++;
         parse_var_decl(parent, var_type);
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_SEMICOLON) {
             token_pos++;
         }
     }
     // Return statement
     else if (current.type == TOKEN_RETURN) {
         token_pos++;
         ASTNode* expr_node = NULL;
         if (token_pos < token_count && tokens[token_pos].type != TOKEN_SEMICOLON && 
             tokens[token_pos].type != TOKEN_RBRACE) {
             expr_node = parse_expression();
         }
         
         // Type checking for return statement
         if (current_function_return_type == TYPE_VOID && expr_node != NULL) {
             printf("[Error] Void function should not return a value\n");
             error_count++;
         } else if (current_function_return_type != TYPE_VOID && expr_node == NULL) {
             printf("[Error] Non-void function should return a value\n");
             error_count++;
         }
         
         ASTNode* return_node = create_return_node(expr_node);
         add_child(parent, return_node);
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_SEMICOLON) {
             token_pos++;
         }
     }
     // If statement
     else if (current.type == TOKEN_IF) {
         token_pos++;
         ASTNode* if_node = create_if_node();
         
         // Condition
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_LPAREN) {
             token_pos++;
             ASTNode* cond = parse_expression();
             if (cond) add_child(if_node, cond);
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_RPAREN) {
                 token_pos++;
             }
         }
         
         // Then block
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACE) {
             token_pos++;
             enter_scope();
             ASTNode* then_block = create_ast_node(AST_BLOCK);
             parse_block(then_block);
             add_child(if_node, then_block);
             exit_scope();
         }
         
         // Else block
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_ELSE) {
             token_pos++;
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACE) {
                 token_pos++;
                 enter_scope();
                 ASTNode* else_block = create_ast_node(AST_BLOCK);
                 parse_block(else_block);
                 add_child(if_node, else_block);
                 exit_scope();
             }
         }
         
         add_child(parent, if_node);
     }
     // While statement
     else if (current.type == TOKEN_WHILE) {
         token_pos++;
         ASTNode* while_node = create_while_node();
         
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_LPAREN) {
             token_pos++;
             ASTNode* cond = parse_expression();
             if (cond) add_child(while_node, cond);
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_RPAREN) {
                 token_pos++;
             }
         }
         
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACE) {
             token_pos++;
             enter_scope();
             ASTNode* body_block = create_ast_node(AST_BLOCK);
             parse_block(body_block);
             add_child(while_node, body_block);
             exit_scope();
         }
         
         add_child(parent, while_node);
     }
     // Print statement
     else if (current.type == TOKEN_PRINT) {
         token_pos++;
         
         // Check for opening parenthesis
         int has_paren = 0;
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_LPAREN) {
             has_paren = 1;
             token_pos++;
         }
         
         ASTNode* expr = NULL;
         if (token_pos < token_count && tokens[token_pos].type != TOKEN_ERROR) {
             // Check if variable is declared before parsing
             if (tokens[token_pos].type == TOKEN_ID) {
                 Symbol* sym = lookup_symbol(tokens[token_pos].lexeme);
                 if (!sym) {
                     printf("[Error] Undeclared variable: %s\n", tokens[token_pos].lexeme);
                     error_count++;
                 }
             }
             expr = parse_expression();
         }
         
         if (has_paren && token_pos < token_count && tokens[token_pos].type == TOKEN_RPAREN) {
             token_pos++;
         }
         
         if (expr) {
             ASTNode* print_node = create_ast_node(AST_PRINT_STMT);
             add_child(print_node, expr);
             add_child(parent, print_node);
         }
         
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_SEMICOLON) {
             token_pos++;
         }
     }
     // Block statement
     else if (current.type == TOKEN_LBRACE) {
         token_pos++;
         enter_scope();
         ASTNode* nested_block = create_ast_node(AST_BLOCK);
         parse_block(nested_block);
         add_child(parent, nested_block);
         exit_scope();
     }
     // Assignment or function call
     else if (current.type == TOKEN_ID) {
         // Check for 'for' keyword (not supported)
         if (strcmp(current.lexeme, "for") == 0) {
             printf("[Error] 'for' loop is not supported\n");
             error_count++;
             token_pos++;
             // Skip the entire for statement
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_LPAREN) {
                 int paren_count = 1;
                 token_pos++;
                 while (token_pos < token_count && paren_count > 0) {
                     if (tokens[token_pos].type == TOKEN_LPAREN) paren_count++;
                     if (tokens[token_pos].type == TOKEN_RPAREN) paren_count--;
                     token_pos++;
                 }
             }
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACE) {
                 int brace_count = 1;
                 token_pos++;
                 while (token_pos < token_count && brace_count > 0) {
                     if (tokens[token_pos].type == TOKEN_LBRACE) brace_count++;
                     if (tokens[token_pos].type == TOKEN_RBRACE) brace_count--;
                     token_pos++;
                 }
             }
             return;
         }
         
         char name[MAX_SYMBOL_LEN];
         strcpy(name, current.lexeme);
         token_pos++;
         
         // Function call
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_LPAREN) {
             // Check if function exists
             if (!lookup_function(name)) {
                 Symbol* sym = lookup_symbol(name);
                 if (sym) {
                     printf("[Error] %s is a variable, not a function\n", name);
                 } else {
                     printf("[Error] Undeclared function: %s\n", name);
                 }
                 error_count++;
             }
             
             token_pos++;
             ASTNode* call_node = create_func_call_node(name);
             while (token_pos < token_count && tokens[token_pos].type != TOKEN_RPAREN) {
                 if (tokens[token_pos].type == TOKEN_COMMA) {
                     token_pos++;
                     continue;
                 }
                 ASTNode* arg = parse_expression();
                 if (arg) add_func_arg(call_node, arg);
             }
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_RPAREN) {
                 token_pos++;
             }
             add_child(parent, call_node);
         }
         // Array assignment
         else if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACKET) {
             token_pos++;
             parse_expression(); // index
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_RBRACKET) {
                 token_pos++;
             }
             // Check for more dimensions (not supported)
             while (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACKET) {
                 printf("[Error] Multi-dimensional arrays not supported\n");
                 error_count++;
                 token_pos++;
                 parse_expression();
                 if (token_pos < token_count && tokens[token_pos].type == TOKEN_RBRACKET) {
                     token_pos++;
                 }
             }
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_ASSIGN) {
                 token_pos++;
                 ASTNode* expr = parse_expression();
                 if (expr) {
                     ASTNode* assign = create_assignment_node(name, expr);
                     add_child(parent, assign);
                 }
             }
         }
         // Normal assignment
         else if (token_pos < token_count && tokens[token_pos].type == TOKEN_ASSIGN) {
             token_pos++;
             ASTNode* expr = parse_expression();
             if (expr) {
                 ASTNode* assign = create_assignment_node(name, expr);
                 add_child(parent, assign);
             }
         }
         
         if (token_pos < token_count && tokens[token_pos].type == TOKEN_SEMICOLON) {
             token_pos++;
         }
     }
     // Skip standalone semicolons
     else if (current.type == TOKEN_SEMICOLON) {
         token_pos++;
     }
     // Skip unknown tokens
     else {
         token_pos++;
     }
 }
 
 void parse_program(void) {
     printf("\n========== SLR(1) Parsing ==========\n");
     
     token_pos = 0;
     ASTNode* program_node = create_ast_node(AST_PROGRAM);
     enter_scope(); // Global scope
     
     printf("\n%-8s %-30s %-25s\n", "Step", "Action", "Details");
     printf("----------------------------------------------------------------------\n");
     
     int step = 0;
     int max_iterations = 10000;
     int iteration_count = 0;
     
     while (token_pos < token_count && tokens[token_pos].type != TOKEN_EOF) {
         if (++iteration_count > max_iterations) {
             printf("[Error] Parsing timeout - too many iterations\n");
             error_count++;
             break;
         }
         
         Token current = tokens[token_pos];
         step++;
         
         // Function definition: type id (params) { body }
         if ((current.type == TOKEN_INT || current.type == TOKEN_FLOAT_TYPE || 
              current.type == TOKEN_VOID) && 
             token_pos + 1 < token_count && tokens[token_pos + 1].type == TOKEN_ID &&
             token_pos + 2 < token_count && tokens[token_pos + 2].type == TOKEN_LPAREN) {
             
             DataType return_type = (current.type == TOKEN_INT) ? TYPE_INT : 
                                    (current.type == TOKEN_FLOAT_TYPE) ? TYPE_FLOAT : TYPE_VOID;
             current_function_return_type = return_type; // Track current function return type
             token_pos++; // Skip return type
             
             const char* func_name = tokens[token_pos].lexeme;
             token_pos++; // Skip function name
             token_pos++; // Skip '('
             enter_scope(); // Function scope
             
             // Add function to function table
             add_function(func_name, return_type);
             
             // Parse parameters
             while (token_pos < token_count && tokens[token_pos].type != TOKEN_RPAREN) {
                 if (tokens[token_pos].type == TOKEN_INT || 
                     tokens[token_pos].type == TOKEN_FLOAT_TYPE) {
                     DataType param_type = (tokens[token_pos].type == TOKEN_INT) ? TYPE_INT : TYPE_FLOAT;
                     token_pos++;
                     if (token_pos < token_count && tokens[token_pos].type == TOKEN_ID) {
                         add_symbol(tokens[token_pos].lexeme, param_type);
                         token_pos++;
                     }
                     // Array parameter
                     if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACKET) {
                         token_pos++;
                         if (token_pos < token_count && tokens[token_pos].type == TOKEN_RBRACKET) {
                             token_pos++;
                         }
                     }
                 } else if (tokens[token_pos].type == TOKEN_COMMA || 
                           tokens[token_pos].type == TOKEN_SEMICOLON) {
                     token_pos++;
                 } else {
                     token_pos++;
                 }
             }
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_RPAREN) {
                 token_pos++; // Skip ')'
             }
             
             // Parse function body
             ASTNode* func_body = create_ast_node(AST_BLOCK);
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_LBRACE) {
                 token_pos++; // Skip '{'
                 parse_block(func_body);
             }
             add_child(program_node, func_body);
             exit_scope(); // Exit function scope
             
             printf("%-8d %-30s %-25s\n", step, "FuncDef", func_name);
         }
         // Global function call: id (args) ;
         else if (current.type == TOKEN_ID && 
                  token_pos + 1 < token_count && tokens[token_pos + 1].type == TOKEN_LPAREN) {
             const char* func_name = current.lexeme;
             
             // Check if function exists
             if (!lookup_function(func_name)) {
                 printf("[Error] Undeclared function: %s\n", func_name);
                 error_count++;
             }
             
             token_pos++; // Skip function name
             token_pos++; // Skip '('
             
             ASTNode* call_node = create_func_call_node(func_name);
             while (token_pos < token_count && tokens[token_pos].type != TOKEN_RPAREN) {
                 if (tokens[token_pos].type == TOKEN_COMMA) {
                     token_pos++;
                     continue;
                 }
                 ASTNode* arg = parse_expression();
                 if (arg) add_func_arg(call_node, arg);
             }
             if (token_pos < token_count && tokens[token_pos].type == TOKEN_RPAREN) {
                 token_pos++;
             }
             add_child(program_node, call_node);
             
             // Consume semicolons
             while (token_pos < token_count && tokens[token_pos].type == TOKEN_SEMICOLON) {
                 token_pos++;
             }
             printf("%-8d %-30s %-25s\n", step, "FuncCall", func_name);
         }
         // Skip other global tokens
         else {
             token_pos++;
         }
     }
     
     exit_scope(); // Exit global scope
     ast_root = program_node;
     printf("\nParsing complete\n");
 }
 
 /* ==================== Intermediate Code Generator ==================== */
 
 char* new_temp(void) {
     static char temp_buf[MAX_SYMBOL_LEN];
     sprintf(temp_buf, "t%d", temp_counter++);
     return temp_buf;
 }
 
 char* new_label(void) {
     static char label_buf[MAX_SYMBOL_LEN];
     sprintf(label_buf, "L%d", label_counter++);
     return label_buf;
 }
 
 void emit(QuadOp op, const char* arg1, const char* arg2, const char* result) {
     if (quad_count >= MAX_QUADS) {
         printf("[Error] Too many quadruples!\n");
         return;
     }
     quads[quad_count].op = op;
     strcpy(quads[quad_count].arg1, arg1 ? arg1 : "");
     strcpy(quads[quad_count].arg2, arg2 ? arg2 : "");
     strcpy(quads[quad_count].result, result ? result : "");
     quad_count++;
 }
 
 QuadOp get_quad_op(const char* op) {
     if (strcmp(op, "+") == 0) return QUAD_ADD;
     if (strcmp(op, "-") == 0) return QUAD_SUB;
     if (strcmp(op, "*") == 0) return QUAD_MUL;
     if (strcmp(op, "/") == 0) return QUAD_DIV;
     if (strcmp(op, "<") == 0) return QUAD_LT;
     if (strcmp(op, "<=") == 0) return QUAD_LE;
     if (strcmp(op, ">") == 0) return QUAD_GT;
     if (strcmp(op, ">=") == 0) return QUAD_GE;
     if (strcmp(op, "==") == 0) return QUAD_EQ;
     if (strcmp(op, "!=") == 0) return QUAD_NE;
     if (strcmp(op, "&&") == 0) return QUAD_AND;
     if (strcmp(op, "||") == 0) return QUAD_OR;
     if (strcmp(op, "!") == 0) return QUAD_NOT;
     if (strcmp(op, "-u") == 0) return QUAD_NEG;
     return QUAD_ASSIGN;
 }
 
 char* gen_const(ASTNode* node) {
     static char buf[2][MAX_SYMBOL_LEN];
     static int buf_idx = 0;
     char* current_buf = buf[buf_idx];
     buf_idx = (buf_idx + 1) % 2;
     
     if (node->data.constant.is_int) {
         sprintf(current_buf, "%d", node->data.constant.int_value);
     } else {
         sprintf(current_buf, "%g", node->data.constant.float_value);
     }
     return current_buf;
 }
 
 char* gen_var(ASTNode* node) {
     return node->data.name;
 }
 
 char* gen_unary_op(ASTNode* node) {
     char* operand_result = generate_code(node->data.unary.operand);
     char* temp = new_temp();
     
     if (strcmp(node->data.unary.op, "-") == 0) {
         emit(QUAD_NEG, operand_result, "", temp);
     } else if (strcmp(node->data.unary.op, "!") == 0) {
         emit(QUAD_NOT, operand_result, "", temp);
     }
     
     return temp;
 }
 
 char* gen_binary_op(ASTNode* node) {
     char* left_result = generate_code(node->data.binary.left);
     
     // Save left result immediately to prevent overwriting
     static char saved_left[MAX_SYMBOL_LEN];
     if (left_result) {
         strcpy(saved_left, left_result);
     }
     
     char* right_result = generate_code(node->data.binary.right);
     char* temp = new_temp();
     
     QuadOp op = get_quad_op(node->data.binary.op);
     emit(op, saved_left, right_result, temp);
     
     return temp;
 }
 
 void gen_assignment(ASTNode* node) {
     if (node->child_count > 0 && node->children[0]) {
         char* expr_result = generate_code(node->children[0]);
         if (expr_result) {
             emit(QUAD_ASSIGN, expr_result, "", node->data.name);
         }
     }
 }
 
 void gen_var_decl(ASTNode* node) {
     // Variable declarations don't generate code
 }
 
 void gen_if_stmt(ASTNode* node) {
     char* label_else = new_label();
     char* label_end = new_label();
     
     if (node->child_count >= 1 && node->children[0]) {
         char* cond_result = generate_code(node->children[0]);
         
         // If condition is false (== 0), jump to else
         emit(QUAD_IF_GOTO, cond_result, "0", label_else);
         
         // Then body
         if (node->child_count >= 2 && node->children[1]) {
             generate_code(node->children[1]);
         }
         
         // Skip else part if there is an else
         if (node->child_count >= 3 && node->children[2]) {
             emit(QUAD_GOTO, "", "", label_end);
         }
         
         emit(QUAD_LABEL, label_else, "", "");
         
         // Else body
         if (node->child_count >= 3 && node->children[2]) {
             generate_code(node->children[2]);
             emit(QUAD_LABEL, label_end, "", "");
         }
     }
 }
 
 void gen_while_stmt(ASTNode* node) {
     char* label_start = new_label();
     char* label_end = new_label();
     
     emit(QUAD_LABEL, label_start, "", "");
     
     if (node->child_count >= 1 && node->children[0]) {
         char* cond_result = generate_code(node->children[0]);
         
         emit(QUAD_IF_GOTO, cond_result, "0", label_end);
         
         if (node->child_count >= 2 && node->children[1]) {
             generate_code(node->children[1]);
         }
         
         emit(QUAD_GOTO, "", "", label_start);
         emit(QUAD_LABEL, label_end, "", "");
     }
 }
 
 void gen_return_stmt(ASTNode* node) {
     if (node->child_count > 0 && node->children[0]) {
         char* result = generate_code(node->children[0]);
         emit(QUAD_RETURN, result, "", "");
     } else {
         emit(QUAD_RETURN, "", "", "");
     }
 }
 
 void gen_func_call(ASTNode* node) {
     for (int i = 0; i < node->data.call.arg_count; i++) {
         if (node->data.call.args[i]) {
             char* arg_result = generate_code(node->data.call.args[i]);
             emit(QUAD_PARAM, arg_result, "", "");
         }
     }
     
     emit(QUAD_CALL, node->data.call.func_name, "", "");
 }
 
 char* generate_code(ASTNode* node) {
     if (!node) return NULL;
     
     switch (node->type) {
         case AST_CONSTANT:
             return gen_const(node);
             
         case AST_VARIABLE:
             return gen_var(node);
             
         case AST_BINARY_OP:
             return gen_binary_op(node);
             
         case AST_UNARY_OP:
             return gen_unary_op(node);
             
         case AST_ASSIGNMENT:
             gen_assignment(node);
             return node->data.name;
             
         case AST_VAR_DECL:
             gen_var_decl(node);
             return NULL;
             
         case AST_IF_STMT:
             gen_if_stmt(node);
             return NULL;
             
         case AST_WHILE_STMT:
             gen_while_stmt(node);
             return NULL;
             
         case AST_RETURN_STMT:
             gen_return_stmt(node);
             return NULL;
             
         case AST_FUNC_CALL:
             gen_func_call(node);
             return NULL;
             
         case AST_PRINT_STMT:
             if (node->child_count > 0 && node->children[0]) {
                 char* expr_result = generate_code(node->children[0]);
                 emit(QUAD_PARAM, expr_result, "", "");
                 emit(QUAD_CALL, "print", "", "");
             }
             return NULL;
             
         case AST_PROGRAM:
         case AST_BLOCK:
             for (int i = 0; i < node->child_count; i++) {
                 generate_code(node->children[i]);
             }
             return NULL;
             
         default:
             return NULL;
     }
 }
 
 void print_quadruples(void) {
     printf("\n========== Intermediate Code (Quadruples) ==========\n");
     
     if (quad_count == 0) {
         printf("No quadruples generated.\n");
         return;
     }
     
     for (int i = 0; i < quad_count; i++) {
         char op_str[10];
         
         switch (quads[i].op) {
             case QUAD_ADD: strcpy(op_str, "+"); break;
             case QUAD_SUB: strcpy(op_str, "-"); break;
             case QUAD_MUL: strcpy(op_str, "*"); break;
             case QUAD_DIV: strcpy(op_str, "/"); break;
             case QUAD_ASSIGN: strcpy(op_str, "="); break;
             case QUAD_LT: strcpy(op_str, "<"); break;
             case QUAD_LE: strcpy(op_str, "<="); break;
             case QUAD_GT: strcpy(op_str, ">"); break;
             case QUAD_GE: strcpy(op_str, ">="); break;
             case QUAD_EQ: strcpy(op_str, "=="); break;
             case QUAD_NE: strcpy(op_str, "!="); break;
             case QUAD_AND: strcpy(op_str, "&&"); break;
             case QUAD_OR: strcpy(op_str, "||"); break;
             case QUAD_NOT: strcpy(op_str, "!"); break;
             case QUAD_NEG: strcpy(op_str, "neg"); break;
             case QUAD_RETURN: strcpy(op_str, "return"); break;
             case QUAD_GOTO: strcpy(op_str, "j"); break;
             case QUAD_IF_GOTO: strcpy(op_str, "jz"); break;
             case QUAD_CALL: strcpy(op_str, "call"); break;
             case QUAD_PARAM: strcpy(op_str, "param"); break;
             case QUAD_LABEL: 
                 printf("%d. %s:\n", i + 1, strlen(quads[i].arg1) > 0 ? quads[i].arg1 : "?");
                 continue;
             default: strcpy(op_str, "?"); break;
         }
         
         printf("%d. (%s, %s, %s, %s)\n", i + 1, 
                op_str,
                strlen(quads[i].arg1) > 0 ? quads[i].arg1 : " ",
                strlen(quads[i].arg2) > 0 ? quads[i].arg2 : " ",
                strlen(quads[i].result) > 0 ? quads[i].result : " ");
     }
     printf("\nTotal quadruples: %d\n", quad_count);
 }
 
 void print_three_address_code(void) {
     printf("\n========== Three-Address Code ==========\n");
     
     for (int i = 0; i < quad_count; i++) {
         switch (quads[i].op) {
             case QUAD_ADD:
                 printf("%s = %s + %s\n", quads[i].result, quads[i].arg1, quads[i].arg2);
                 break;
             case QUAD_SUB:
                 printf("%s = %s - %s\n", quads[i].result, quads[i].arg1, quads[i].arg2);
                 break;
             case QUAD_MUL:
                 printf("%s = %s * %s\n", quads[i].result, quads[i].arg1, quads[i].arg2);
                 break;
             case QUAD_DIV:
                 printf("%s = %s / %s\n", quads[i].result, quads[i].arg1, quads[i].arg2);
                 break;
             case QUAD_ASSIGN:
                 printf("%s = %s\n", quads[i].result, quads[i].arg1);
                 break;
             case QUAD_RETURN:
                 if (quads[i].arg1[0])
                     printf("return %s\n", quads[i].arg1);
                 else
                     printf("return\n");
                 break;
             case QUAD_CALL:
                 printf("call %s\n", quads[i].arg1);
                 break;
             case QUAD_PARAM:
                 printf("param %s\n", quads[i].arg1);
                 break;
             case QUAD_NEG:
                 printf("%s = -%s\n", quads[i].result, quads[i].arg1);
                 break;
             case QUAD_NOT:
                 printf("%s = !%s\n", quads[i].result, quads[i].arg1);
                 break;
             case QUAD_LABEL:
                 printf("%s:\n", quads[i].arg1);
                 break;
             case QUAD_GOTO:
                 printf("goto %s\n", quads[i].result);
                 break;
             case QUAD_IF_GOTO:
                 printf("if %s == %s goto %s\n", quads[i].arg1, quads[i].arg2, quads[i].result);
                 break;
             default:
                 break;
         }
     }
     printf("========================================\n");
 }
 
 /* ==================== Main ==================== */
 
 int main(int argc, char** argv) {
     printf("========================================\n");
     printf("  Lab 6: Intermediate Code Generation\n");
     printf("========================================\n");
     
     init_symbol_table();
     const char* filename = (argc > 1) ? argv[1] : "test.c";
     lex_analyze(filename);
     parse_program();
     print_function_table();
     print_symbol_table();
     
     printf("\n========== Abstract Syntax Tree (AST) ==========\n");
     print_ast(ast_root, 0);
     
     printf("\n========== Generating Intermediate Code ==========\n");
     generate_code(ast_root);
     
     print_quadruples();
     print_three_address_code();
     
     printf("\n========== Intermediate Code Generation Result ==========\n");
     if (error_count == 0)
         printf("[OK] Code generation complete, no errors. Generated %d quadruples\n", quad_count);
     else
         printf("[FAIL] Code generation complete, %d errors found\n", error_count);
     
     return 0;
 }