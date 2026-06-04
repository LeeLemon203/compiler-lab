/**
 * Lab 3+4: LR(0) + SLR(1) Parser
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <ctype.h>
 
 #define MAX_PRODUCTIONS 100
 #define MAX_SYMBOLS 50
 #define MAX_ITEMS 200
 #define MAX_ITEM_SETS 100
 #define MAX_SYMBOL_LEN 64
 #define MAX_STACK 1000
 
 typedef struct {
     char left[MAX_SYMBOL_LEN];
     char right[MAX_SYMBOL_LEN][MAX_SYMBOL_LEN];
     int right_len;
 } Production;
 
 typedef struct {
     int prod_id;
     int dot_pos;
 } Item;
 
 typedef struct {
     Item items[MAX_ITEMS];
     int item_count;
     int id;
     int transitions[MAX_SYMBOLS];
     char trans_symbols[MAX_SYMBOLS][MAX_SYMBOL_LEN];
     int trans_count;
 } ItemSet;
 
 Production productions[MAX_PRODUCTIONS];
 int prod_count = 0;
 char symbols[MAX_SYMBOLS][MAX_SYMBOL_LEN];
 int symbol_count = 0;
 int is_terminal[MAX_SYMBOLS];
 int is_non_terminal[MAX_SYMBOLS];
 char start_symbol[MAX_SYMBOL_LEN];
 char augmented_start[MAX_SYMBOL_LEN];
 
 ItemSet item_sets[MAX_ITEM_SETS];
 int item_set_count = 0;
 
 int first_set[MAX_SYMBOLS][MAX_SYMBOLS];
 int first_count[MAX_SYMBOLS];
 int follow_set[MAX_SYMBOLS][MAX_SYMBOLS];
 int follow_count[MAX_SYMBOLS];
 
 char action_table[MAX_ITEM_SETS][MAX_SYMBOLS][20];
 int goto_table[MAX_ITEM_SETS][MAX_SYMBOLS];
 
 int is_letter(char c) {
     return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
 }
 
 void trim(char* str) {
     char* start = str;
     while (*start == ' ' || *start == '\t') start++;
     
     char* end = start + strlen(start) - 1;
     while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
         end--;
     }
     *(end + 1) = '\0';
     
     if (start != str) {
         memmove(str, start, strlen(start) + 1);
     }
 }
 
 int get_symbol_index(const char* sym) {
     for (int i = 0; i < symbol_count; i++) {
         if (strcmp(symbols[i], sym) == 0) {
             return i;
         }
     }
     if (symbol_count < MAX_SYMBOLS) {
         strcpy(symbols[symbol_count], sym);
         if (strlen(sym) == 1) {
             char c = sym[0];
             if (c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')' || c == '$') {
                 is_terminal[symbol_count] = 1;
                 is_non_terminal[symbol_count] = 0;
             } else if (c >= 'a' && c <= 'z') {
                 is_terminal[symbol_count] = 1;
                 is_non_terminal[symbol_count] = 0;
             } else if (c >= 'A' && c <= 'Z') {
                 is_terminal[symbol_count] = 0;
                 is_non_terminal[symbol_count] = 1;
             } else {
                 is_terminal[symbol_count] = 1;
                 is_non_terminal[symbol_count] = 0;
             }
         } else if (strcmp(sym, "id") == 0 || strcmp(sym, "num") == 0) {
             is_terminal[symbol_count] = 1;
             is_non_terminal[symbol_count] = 0;
         } else {
             is_terminal[symbol_count] = 0;
             is_non_terminal[symbol_count] = 1;
         }
         symbol_count++;
         return symbol_count - 1;
     }
     return -1;
 }
 
 void parse_grammar(const char* filename) {
     FILE* file = fopen(filename, "r");
     if (file == NULL) {
         printf("Error: Cannot open file %s\n", filename);
         return;
     }
     
     char line[1024];
     printf("\n========== Loading Grammar ==========\n");
     
     while (fgets(line, sizeof(line), file)) {
         line[strcspn(line, "\n")] = '\0';
         if (strlen(line) == 0 || line[0] == '#') continue;
         
         char* arrow = strstr(line, "->");
         if (arrow == NULL) continue;
         
         char left[MAX_SYMBOL_LEN];
         strncpy(left, line, arrow - line);
         left[arrow - line] = '\0';
         trim(left);
         
         char* right_part = arrow + 2;
         
         char* pipe = strchr(right_part, '|');
         if (pipe != NULL) {
             char* start = right_part;
             while (start != NULL && *start != '\0') {
                 char* next_pipe = strchr(start, '|');
                 char right_buf[MAX_SYMBOL_LEN * 10];
                 
                 if (next_pipe != NULL) {
                     strncpy(right_buf, start, next_pipe - start);
                     right_buf[next_pipe - start] = '\0';
                     start = next_pipe + 1;
                 } else {
                     strcpy(right_buf, start);
                     start = NULL;
                 }
                 trim(right_buf);
                 
                 char* tokens[MAX_SYMBOLS];
                 int token_count = 0;
                 char* token = strtok(right_buf, " \t");
                 while (token != NULL && token_count < MAX_SYMBOLS) {
                     trim(token);
                     if (strlen(token) > 0) {
                         tokens[token_count++] = token;
                     }
                     token = strtok(NULL, " \t");
                 }
                 
                 if (token_count > 0) {
                     strcpy(productions[prod_count].left, left);
                     productions[prod_count].right_len = token_count;
                     for (int i = 0; i < token_count; i++) {
                         strcpy(productions[prod_count].right[i], tokens[i]);
                     }
                     prod_count++;
                 }
             }
         } else {
             char* tokens[MAX_SYMBOLS];
             int token_count = 0;
             char* token = strtok(right_part, " \t");
             while (token != NULL && token_count < MAX_SYMBOLS) {
                 trim(token);
                 if (strlen(token) > 0) {
                     tokens[token_count++] = token;
                 }
                 token = strtok(NULL, " \t");
             }
             
             if (token_count > 0) {
                 strcpy(productions[prod_count].left, left);
                 productions[prod_count].right_len = token_count;
                 for (int i = 0; i < token_count; i++) {
                     strcpy(productions[prod_count].right[i], tokens[i]);
                 }
                 prod_count++;
             }
         }
         
         if (strlen(start_symbol) == 0) {
             strcpy(start_symbol, left);
         }
     }
     
     fclose(file);
     
     printf("Loaded %d productions\n", prod_count);
     for (int i = 0; i < prod_count; i++) {
         printf("  %d: %s -> ", i, productions[i].left);
         for (int j = 0; j < productions[i].right_len; j++) {
             printf("%s ", productions[i].right[j]);
         }
         printf("\n");
     }
     
     snprintf(augmented_start, sizeof(augmented_start), "%s'", start_symbol);
     strcpy(productions[prod_count].left, augmented_start);
     productions[prod_count].right_len = 1;
     strcpy(productions[prod_count].right[0], start_symbol);
     prod_count++;
     
     printf("\nAugmented grammar:\n");
     printf("  %d: %s -> %s\n", prod_count - 1, augmented_start, start_symbol);
     
     for (int i = 0; i < prod_count; i++) {
         get_symbol_index(productions[i].left);
         for (int j = 0; j < productions[i].right_len; j++) {
             get_symbol_index(productions[i].right[j]);
         }
     }
     get_symbol_index("$");
     
     printf("\nSymbol table (%d symbols):\n", symbol_count);
     for (int i = 0; i < symbol_count; i++) {
         printf("  %s: %s\n", symbols[i], is_terminal[i] ? "terminal" : "non-terminal");
     }
 }
 
 int items_equal(Item* items1, int count1, Item* items2, int count2) {
     if (count1 != count2) return 0;
     for (int i = 0; i < count1; i++) {
         int found = 0;
         for (int j = 0; j < count2; j++) {
             if (items1[i].prod_id == items2[j].prod_id && 
                 items1[i].dot_pos == items2[j].dot_pos) {
                 found = 1;
                 break;
             }
         }
         if (!found) return 0;
     }
     return 1;
 }
 
 int item_in_set(ItemSet* set, Item item) {
     for (int i = 0; i < set->item_count; i++) {
         if (set->items[i].prod_id == item.prod_id && 
             set->items[i].dot_pos == item.dot_pos) {
             return 1;
         }
     }
     return 0;
 }
 
 void closure(ItemSet* set) {
     int changed = 1;
     
     while (changed) {
         changed = 0;
         
         for (int i = 0; i < set->item_count; i++) {
             Item item = set->items[i];
             Production* prod = &productions[item.prod_id];
             
             if (item.dot_pos < prod->right_len) {
                 char* symbol = prod->right[item.dot_pos];
                 
                 for (int j = 0; j < prod_count; j++) {
                     if (strcmp(productions[j].left, symbol) == 0) {
                         Item new_item = {j, 0};
                         if (!item_in_set(set, new_item)) {
                             if (set->item_count < MAX_ITEMS) {
                                 set->items[set->item_count++] = new_item;
                                 changed = 1;
                             }
                         }
                     }
                 }
             }
         }
     }
 }
 
 ItemSet goto_set(ItemSet* set, const char* symbol) {
     ItemSet result;
     result.item_count = 0;
     result.id = -1;
     result.trans_count = 0;
     
     for (int i = 0; i < set->item_count; i++) {
         Item item = set->items[i];
         Production* prod = &productions[item.prod_id];
         
         if (item.dot_pos < prod->right_len && 
             strcmp(prod->right[item.dot_pos], symbol) == 0) {
             Item new_item = {item.prod_id, item.dot_pos + 1};
             if (result.item_count < MAX_ITEMS) {
                 result.items[result.item_count++] = new_item;
             }
         }
     }
     
     if (result.item_count > 0) {
         closure(&result);
     }
     
     return result;
 }
 
 int find_item_set(ItemSet* sets, int count, ItemSet* target) {
     for (int i = 0; i < count; i++) {
         if (items_equal(sets[i].items, sets[i].item_count, 
                         target->items, target->item_count)) {
             return i;
         }
     }
     return -1;
 }
 
 void print_item(Item item) {
     Production* prod = &productions[item.prod_id];
     printf("%s -> ", prod->left);
     
     for (int i = 0; i <= prod->right_len; i++) {
         if (i == item.dot_pos) {
             printf(".");
         }
         if (i < prod->right_len) {
             printf("%s", prod->right[i]);
             if (i < prod->right_len - 1) printf(" ");
         }
     }
 }
 
 void build_lr0_items() {
     printf("\n========== Building LR(0) Items ==========\n");
     
     ItemSet I0;
     I0.item_count = 0;
     I0.id = 0;
     I0.trans_count = 0;
     
     Item start_item = {prod_count - 1, 0};
     I0.items[I0.item_count++] = start_item;
     closure(&I0);
     
     item_sets[item_set_count++] = I0;
     
     int changed = 1;
     while (changed) {
         changed = 0;
         
         for (int i = 0; i < item_set_count; i++) {
             ItemSet* current = &item_sets[i];
             
             for (int s = 0; s < symbol_count; s++) {
                 char* symbol = symbols[s];
                 
                 ItemSet next = goto_set(current, symbol);
                 if (next.item_count > 0) {
                     int existing = find_item_set(item_sets, item_set_count, &next);
                     
                     if (existing == -1) {
                         next.id = item_set_count;
                         next.trans_count = 0;
                         if (item_set_count < MAX_ITEM_SETS) {
                             item_sets[item_set_count++] = next;
                             existing = item_set_count - 1;
                             changed = 1;
                         }
                     }
                     
                     int found = 0;
                     for (int t = 0; t < current->trans_count; t++) {
                         if (strcmp(current->trans_symbols[t], symbol) == 0) {
                             found = 1;
                             break;
                         }
                     }
                     if (!found && current->trans_count < MAX_SYMBOLS) {
                         strcpy(current->trans_symbols[current->trans_count], symbol);
                         current->transitions[current->trans_count] = existing;
                         current->trans_count++;
                     }
                 }
             }
         }
     }
     
     printf("Generated %d item sets\n", item_set_count);
 }
 
 void print_item_sets() {
     printf("\n========== LR(0) Item Sets ==========\n\n");
     
     for (int i = 0; i < item_set_count; i++) {
         ItemSet* set = &item_sets[i];
         printf("I%d:\n", set->id);
         
         for (int j = 0; j < set->item_count; j++) {
             printf("    ");
             print_item(set->items[j]);
             printf("\n");
         }
         
         if (set->trans_count > 0) {
             printf("    Transitions:\n");
             for (int t = 0; t < set->trans_count; t++) {
                 printf("        %s -> I%d\n", 
                        set->trans_symbols[t], 
                        set->transitions[t]);
             }
         }
         printf("\n");
     }
 }
 
 int is_nullable(int sym_idx) {
     if (is_terminal[sym_idx]) return 0;
     for (int i = 0; i < prod_count; i++) {
         if (strcmp(productions[i].left, symbols[sym_idx]) == 0) {
             if (productions[i].right_len == 0) return 1;
         }
     }
     return 0;
 }
 
 void compute_first() {
     printf("\n========== Computing FIRST Sets ==========\n");
     
     for (int i = 0; i < symbol_count; i++) {
         first_count[i] = 0;
         for (int j = 0; j < symbol_count; j++) {
             first_set[i][j] = 0;
         }
     }
     
     for (int i = 0; i < symbol_count; i++) {
         if (is_terminal[i]) {
             first_set[i][first_count[i]++] = i;
         }
     }
     
     int changed = 1;
     while (changed) {
         changed = 0;
         
         for (int i = 0; i < prod_count; i++) {
             Production* prod = &productions[i];
             int left_idx = get_symbol_index(prod->left);
             
             for (int j = 0; j < prod->right_len; j++) {
                 int sym_idx = get_symbol_index(prod->right[j]);
                 
                 for (int k = 0; k < first_count[sym_idx]; k++) {
                     int term = first_set[sym_idx][k];
                     int exists = 0;
                     for (int m = 0; m < first_count[left_idx]; m++) {
                         if (first_set[left_idx][m] == term) {
                             exists = 1;
                             break;
                         }
                     }
                     if (!exists) {
                         first_set[left_idx][first_count[left_idx]++] = term;
                         changed = 1;
                     }
                 }
                 
                 if (!is_nullable(sym_idx)) {
                     break;
                 }
             }
         }
     }
     
     for (int i = 0; i < symbol_count; i++) {
         if (is_non_terminal[i]) {
             printf("  FIRST(%s) = { ", symbols[i]);
             for (int j = 0; j < first_count[i]; j++) {
                 printf("%s ", symbols[first_set[i][j]]);
             }
             printf("}\n");
         }
     }
 }
 
 void compute_follow() {
     printf("\n========== Computing FOLLOW Sets ==========\n");
     
     for (int i = 0; i < symbol_count; i++) {
         follow_count[i] = 0;
         for (int j = 0; j < symbol_count; j++) {
             follow_set[i][j] = 0;
         }
     }
     
     int start_idx = get_symbol_index(start_symbol);
     int dollar_idx = get_symbol_index("$");
     follow_set[start_idx][follow_count[start_idx]++] = dollar_idx;
     
     int changed = 1;
     while (changed) {
         changed = 0;
         
         for (int i = 0; i < prod_count; i++) {
             Production* prod = &productions[i];
             int left_idx = get_symbol_index(prod->left);
             
             for (int j = 0; j < prod->right_len; j++) {
                 int sym_idx = get_symbol_index(prod->right[j]);
                 
                 if (is_non_terminal[sym_idx]) {
                     int all_nullable = 1;
                     
                     for (int k = j + 1; k < prod->right_len; k++) {
                         int next_idx = get_symbol_index(prod->right[k]);
                         
                         for (int m = 0; m < first_count[next_idx]; m++) {
                             int term = first_set[next_idx][m];
                             int exists = 0;
                             for (int n = 0; n < follow_count[sym_idx]; n++) {
                                 if (follow_set[sym_idx][n] == term) {
                                     exists = 1;
                                     break;
                                 }
                             }
                             if (!exists) {
                                 follow_set[sym_idx][follow_count[sym_idx]++] = term;
                                 changed = 1;
                             }
                         }
                         
                         if (!is_nullable(next_idx)) {
                             all_nullable = 0;
                             break;
                         }
                     }
                     
                     if (all_nullable || j + 1 == prod->right_len) {
                         for (int k = 0; k < follow_count[left_idx]; k++) {
                             int term = follow_set[left_idx][k];
                             int exists = 0;
                             for (int m = 0; m < follow_count[sym_idx]; m++) {
                                 if (follow_set[sym_idx][m] == term) {
                                     exists = 1;
                                     break;
                                 }
                             }
                             if (!exists) {
                                 follow_set[sym_idx][follow_count[sym_idx]++] = term;
                                 changed = 1;
                             }
                         }
                     }
                 }
             }
         }
     }
     
     for (int i = 0; i < symbol_count; i++) {
         if (is_non_terminal[i]) {
             printf("  FOLLOW(%s) = { ", symbols[i]);
             for (int j = 0; j < follow_count[i]; j++) {
                 printf("%s ", symbols[follow_set[i][j]]);
             }
             printf("}\n");
         }
     }
 }
 
 void build_slr1_table() {
     printf("\n========== Building SLR(1) Parse Table ==========\n");
     
     for (int i = 0; i < MAX_ITEM_SETS; i++) {
         for (int j = 0; j < MAX_SYMBOLS; j++) {
             action_table[i][j][0] = '\0';
             goto_table[i][j] = -1;
         }
     }
     
     int conflict_count = 0;
     
     for (int i = 0; i < item_set_count; i++) {
         ItemSet* set = &item_sets[i];
         
         for (int j = 0; j < set->item_count; j++) {
             Item item = set->items[j];
             Production* prod = &productions[item.prod_id];
             
             if (item.dot_pos < prod->right_len) {
                 char* next_sym = prod->right[item.dot_pos];
                 int sym_idx = get_symbol_index(next_sym);
                 
                 if (is_terminal[sym_idx]) {
                     for (int t = 0; t < set->trans_count; t++) {
                         if (strcmp(set->trans_symbols[t], next_sym) == 0) {
                             int next_state = set->transitions[t];
                             
                             if (action_table[i][sym_idx][0] != '\0') {
                                 printf("  Conflict: I%d, symbol %s, existing %s, new s%d\n",
                                        i, next_sym, action_table[i][sym_idx], next_state);
                                 conflict_count++;
                             }
                             
                             snprintf(action_table[i][sym_idx], 20, "s%d", next_state);
                             break;
                         }
                     }
                 }
             } else {
                 if (strcmp(prod->left, augmented_start) == 0 && 
                     prod->right_len == 1 &&
                     strcmp(prod->right[0], start_symbol) == 0) {
                     int dollar_idx = get_symbol_index("$");
                     if (action_table[i][dollar_idx][0] != '\0') {
                         printf("  Conflict: I%d, symbol $\n", i);
                         conflict_count++;
                     }
                     strcpy(action_table[i][dollar_idx], "acc");
                 } else {
                     int left_idx = get_symbol_index(prod->left);
                     
                     for (int k = 0; k < follow_count[left_idx]; k++) {
                         int term_idx = follow_set[left_idx][k];
                         
                         if (action_table[i][term_idx][0] != '\0') {
                             printf("  Conflict: I%d, symbol %s, existing %s, new r%d\n",
                                    i, symbols[term_idx], action_table[i][term_idx], item.prod_id);
                             conflict_count++;
                         }
                         
                         snprintf(action_table[i][term_idx], 20, "r%d", item.prod_id);
                     }
                 }
             }
         }
         
         for (int t = 0; t < set->trans_count; t++) {
             char* sym = set->trans_symbols[t];
             int sym_idx = get_symbol_index(sym);
             if (is_non_terminal[sym_idx]) {
                 goto_table[i][sym_idx] = set->transitions[t];
             }
         }
     }
     
     if (conflict_count > 0) {
         printf("\n[WARNING] %d conflicts found, grammar is NOT SLR(1)\n", conflict_count);
     } else {
         printf("\n[OK] No conflicts, grammar is SLR(1)\n");
     }
 }
 
 void print_slr1_table() {
     printf("\n========== SLR(1) Parse Table ==========\n");
     
     int term_indices[MAX_SYMBOLS];
     int term_count = 0;
     int dollar_idx = get_symbol_index("$");
     
     for (int i = 0; i < symbol_count; i++) {
         if (is_terminal[i] && i != dollar_idx) {
             term_indices[term_count++] = i;
         }
     }
     term_indices[term_count++] = dollar_idx;
     
     printf("\nACTION Table:\n");
     printf("     ");
     for (int i = 0; i < term_count; i++) {
         printf("%10s", symbols[term_indices[i]]);
     }
     printf("\n");
     printf("     ");
     for (int i = 0; i < term_count; i++) {
         printf("----------");
     }
     printf("\n");
     
     for (int i = 0; i < item_set_count; i++) {
         printf("I%-3d ", i);
         for (int j = 0; j < term_count; j++) {
             int term = term_indices[j];
             if (action_table[i][term][0] != '\0') {
                 printf("%10s", action_table[i][term]);
             } else {
                 printf("%10s", "");
             }
         }
         printf("\n");
     }
     
     int nonterm_indices[MAX_SYMBOLS];
     int nonterm_count = 0;
     for (int i = 0; i < symbol_count; i++) {
         if (is_non_terminal[i] && strcmp(symbols[i], augmented_start) != 0) {
             nonterm_indices[nonterm_count++] = i;
         }
     }
     
     if (nonterm_count > 0) {
         printf("\nGOTO Table:\n");
         printf("     ");
         for (int i = 0; i < nonterm_count; i++) {
             printf("%10s", symbols[nonterm_indices[i]]);
         }
         printf("\n");
         printf("     ");
         for (int i = 0; i < nonterm_count; i++) {
             printf("----------");
         }
         printf("\n");
         
         for (int i = 0; i < item_set_count; i++) {
             printf("I%-3d ", i);
             for (int j = 0; j < nonterm_count; j++) {
                 int nt = nonterm_indices[j];
                 if (goto_table[i][nt] != -1) {
                     printf("%10d", goto_table[i][nt]);
                 } else {
                     printf("%10s", "");
                 }
             }
             printf("\n");
         }
     }
 }
 
 void parse_input() {
     printf("\n========== SLR(1) Parsing Demo ==========\n");
     
     char input[1024];
     printf("Enter input string (space separated, e.g., id + id * id):\n");
     getchar();
     fgets(input, sizeof(input), stdin);
     input[strcspn(input, "\n")] = '\0';
     
     char* tokens[MAX_SYMBOLS];
     int token_count = 0;
     char* token = strtok(input, " ");
     while (token != NULL && token_count < MAX_SYMBOLS) {
         tokens[token_count++] = token;
         token = strtok(NULL, " ");
     }
     
     tokens[token_count++] = "$";
     
     int state_stack[MAX_STACK];
     char symbol_stack[MAX_STACK][MAX_SYMBOL_LEN];
     int stack_top = 0;
     state_stack[0] = 0;
     
     int pos = 0;
     int step = 1;
     
     printf("\n%-8s %-30s %-20s %-25s %s\n", "Step", "State Stack", "Symbol Stack", "Input", "Action");
     printf("----------------------------------------------------------------\n");
     
     while (1) {
         int state = state_stack[stack_top];
         char* current = tokens[pos];
         int sym_idx = get_symbol_index(current);
         
         if (sym_idx == -1) {
             printf("\n[ERROR] Unknown symbol '%s'\n", current);
             return;
         }
         
         char state_str[256] = "";
         for (int i = 0; i <= stack_top; i++) {
             char buf[10];
             snprintf(buf, sizeof(buf), "%d ", state_stack[i]);
             strcat(state_str, buf);
         }
         
         char symbol_str[256] = "";
         for (int i = 0; i < stack_top; i++) {
             strcat(symbol_str, symbol_stack[i]);
             strcat(symbol_str, " ");
         }
         
         char input_str[256] = "";
         for (int i = pos; i < token_count; i++) {
             strcat(input_str, tokens[i]);
             strcat(input_str, " ");
         }
         
         if (action_table[state][sym_idx][0] == '\0') {
             printf("\n[ERROR] At state I%d, undefined input symbol '%s'\n", state, current);
             return;
         }
         
         char* action = action_table[state][sym_idx];
         
         printf("%-8d %-30s %-20s %-25s %s\n", step, state_str, symbol_str, input_str, action);
         
         if (strcmp(action, "acc") == 0) {
             printf("\n[SUCCESS] Parsing completed!\n");
             return;
         }
         else if (action[0] == 's') {
             int next_state = atoi(action + 1);
             stack_top++;
             state_stack[stack_top] = next_state;
             strcpy(symbol_stack[stack_top - 1], current);
             pos++;
         }
         else if (action[0] == 'r') {
             int prod_id = atoi(action + 1);
             Production* prod = &productions[prod_id];
             
             for (int i = 0; i < prod->right_len; i++) {
                 if (stack_top > 0) {
                     stack_top--;
                 }
             }
             
             int new_state = state_stack[stack_top];
             int left_idx = get_symbol_index(prod->left);
             if (goto_table[new_state][left_idx] == -1) {
                 printf("\n[ERROR] At state I%d, undefined GOTO(%s)\n", new_state, prod->left);
                 return;
             }
             
             stack_top++;
             state_stack[stack_top] = goto_table[new_state][left_idx];
             strcpy(symbol_stack[stack_top - 1], prod->left);
         }
         
         step++;
     }
 }
 
 int main() {
     int mode;
     
     printf("========================================\n");
     printf("  Lab 3+4: LR(0) + SLR(1) Parser\n");
     printf("========================================\n");
     
     printf("\nSelect input method:\n");
     printf("  1: Use default grammar (arithmetic expressions)\n");
     printf("  2: Read grammar from file\n");
     printf("Enter choice (1 or 2): ");
     
     scanf("%d", &mode);
     
     if (mode == 1) {
         prod_count = 0;
         symbol_count = 0;
         
         strcpy(productions[prod_count].left, "E");
         productions[prod_count].right_len = 3;
         strcpy(productions[prod_count].right[0], "E");
         strcpy(productions[prod_count].right[1], "+");
         strcpy(productions[prod_count].right[2], "T");
         prod_count++;
         
         strcpy(productions[prod_count].left, "E");
         productions[prod_count].right_len = 1;
         strcpy(productions[prod_count].right[0], "T");
         prod_count++;
         
         strcpy(productions[prod_count].left, "T");
         productions[prod_count].right_len = 3;
         strcpy(productions[prod_count].right[0], "T");
         strcpy(productions[prod_count].right[1], "*");
         strcpy(productions[prod_count].right[2], "F");
         prod_count++;
         
         strcpy(productions[prod_count].left, "T");
         productions[prod_count].right_len = 1;
         strcpy(productions[prod_count].right[0], "F");
         prod_count++;
         
         strcpy(productions[prod_count].left, "F");
         productions[prod_count].right_len = 3;
         strcpy(productions[prod_count].right[0], "(");
         strcpy(productions[prod_count].right[1], "E");
         strcpy(productions[prod_count].right[2], ")");
         prod_count++;
         
         strcpy(productions[prod_count].left, "F");
         productions[prod_count].right_len = 1;
         strcpy(productions[prod_count].right[0], "id");
         prod_count++;
         
         strcpy(start_symbol, "E");
         
         snprintf(augmented_start, sizeof(augmented_start), "%s'", start_symbol);
         strcpy(productions[prod_count].left, augmented_start);
         productions[prod_count].right_len = 1;
         strcpy(productions[prod_count].right[0], start_symbol);
         prod_count++;
         
         for (int i = 0; i < prod_count; i++) {
             get_symbol_index(productions[i].left);
             for (int j = 0; j < productions[i].right_len; j++) {
                 get_symbol_index(productions[i].right[j]);
             }
         }
         get_symbol_index("$");
         
         printf("\nLoaded %d productions\n", prod_count - 1);
         printf("Augmented grammar: %s -> %s\n", augmented_start, start_symbol);
         
     } else {
         char filename[256];
         printf("Enter grammar filename: ");
         scanf("%s", filename);
         parse_grammar(filename);
     }
     
     build_lr0_items();
     print_item_sets();
     
     printf("\n========== LR(0) Conflict Check ==========\n");
     int lr0_conflict = 0;
     for (int i = 0; i < item_set_count; i++) {
         ItemSet* set = &item_sets[i];
         int reduce_count = 0;
         int shift_count = 0;
         
         for (int j = 0; j < set->item_count; j++) {
             Item item = set->items[j];
             Production* prod = &productions[item.prod_id];
             
             if (item.dot_pos == prod->right_len) {
                 reduce_count++;
             } else if (item.dot_pos < prod->right_len) {
                 char* sym = prod->right[item.dot_pos];
                 int idx = get_symbol_index(sym);
                 if (idx >= 0 && is_terminal[idx]) {
                     shift_count++;
                 }
             }
         }
         
         if (reduce_count > 1) {
             printf("  I%d: WARNING - Reduce-Reduce conflict!\n", set->id);
             lr0_conflict = 1;
         }
         if (shift_count > 0 && reduce_count > 0) {
             printf("  I%d: WARNING - Shift-Reduce conflict!\n", set->id);
             lr0_conflict = 1;
         }
     }
     if (!lr0_conflict) {
         printf("  [OK] No LR(0) conflicts\n");
     } else {
         printf("  [WARNING] LR(0) conflicts exist, need SLR(1)\n");
     }
     
     compute_first();
     compute_follow();
     build_slr1_table();
     print_slr1_table();
     
     FILE* out = fopen("slr1_table.txt", "w");
     if (out) {
         fprintf(out, "SLR(1) Parse Table\n");
         fprintf(out, "==================\n\n");
         
         int term_indices[MAX_SYMBOLS], term_count = 0;
         int dollar_idx = get_symbol_index("$");
         for (int i = 0; i < symbol_count; i++) {
             if (is_terminal[i] && i != dollar_idx) term_indices[term_count++] = i;
         }
         term_indices[term_count++] = dollar_idx;
         
         fprintf(out, "ACTION Table:\n");
         for (int i = 0; i < item_set_count; i++) {
             fprintf(out, "I%d: ", i);
             for (int j = 0; j < term_count; j++) {
                 if (action_table[i][term_indices[j]][0]) {
                     fprintf(out, "%s ", action_table[i][term_indices[j]]);
                 }
             }
             fprintf(out, "\n");
         }
         fclose(out);
         printf("\n[OK] Parse table saved to slr1_table.txt\n");
     }
     
     parse_input();
     
     return 0;
 }