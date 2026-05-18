/**
 * 实验三：LR(0) 项目集规范族构建
 * 输入：文法规则
 * 输出：LR(0) 项目集规范族，包含闭包和转移关系
 * 
 * 编译：gcc lr0.c -o lr0
 * 运行：./lr0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_PRODUCTIONS 100      // 最大产生式数量
#define MAX_SYMBOLS 50           // 最大符号数量
#define MAX_ITEMS 200            // 最大项目数量
#define MAX_ITEM_SETS 100        // 最大项目集数量
#define MAX_SYMBOL_LEN 64        // 符号最大长度（增大到64避免溢出）

// 产生式结构
typedef struct {
    char left[MAX_SYMBOL_LEN];           // 左部
    char right[MAX_SYMBOL_LEN][MAX_SYMBOL_LEN]; // 右部符号列表
    int right_len;                        // 右部长度
} Production;

// LR(0) 项目
typedef struct {
    int prod_id;          // 产生式编号
    int dot_pos;          // 圆点位置 (0 表示在最左边)
} Item;

// 项目集
typedef struct {
    Item items[MAX_ITEMS];    // 项目列表
    int item_count;           // 项目数量
    int id;                   // 项目集编号
    int transitions[MAX_SYMBOLS]; // 转移目标项目集编号
    char trans_symbols[MAX_SYMBOLS][MAX_SYMBOL_LEN]; // 转移符号
    int trans_count;          // 转移数量
} ItemSet;

// 全局变量
Production productions[MAX_PRODUCTIONS];  // 所有产生式
int prod_count = 0;                        // 产生式数量
char symbols[MAX_SYMBOLS][MAX_SYMBOL_LEN]; // 所有符号（终结符+非终结符）
int symbol_count = 0;                      // 符号数量
int is_terminal[MAX_SYMBOLS];              // 是否为终结符
int is_non_terminal[MAX_SYMBOLS];          // 是否为非终结符
char start_symbol[MAX_SYMBOL_LEN];         // 开始符号

ItemSet item_sets[MAX_ITEM_SETS];          // 项目集规范族
int item_set_count = 0;                    // 项目集数量

// 辅助函数：判断字符是否为字母
int is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// 获取符号在符号表中的索引
int get_symbol_index(const char* sym) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbols[i], sym) == 0) {
            return i;
        }
    }
    // 新符号，添加到符号表
    if (symbol_count < MAX_SYMBOLS) {
        strcpy(symbols[symbol_count], sym);
        // 判断是否为终结符（小写字母开头或单个符号）
        if (is_letter(sym[0]) && sym[0] >= 'a' && sym[0] <= 'z') {
            is_terminal[symbol_count] = 1;
            is_non_terminal[symbol_count] = 0;
        } else if (sym[0] == '\'' || sym[0] == '(' || sym[0] == ')' || 
                   sym[0] == '+' || sym[0] == '*' || sym[0] == '|') {
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

// 去除字符串首尾空格
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

// 解析文法规则
void parse_grammar(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("错误：无法打开文件 %s\n", filename);
        return;
    }
    
    char line[1024];
    int line_num = 0;
    
    printf("\n========== 加载文法规则 ==========\n");
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        line[strcspn(line, "\n")] = '\0';
        
        // 跳过空行和注释
        if (strlen(line) == 0 || line[0] == '#') continue;
        
        // 解析产生式：格式为 "A -> B C D" 或 "A -> B | C"
        char* arrow = strstr(line, "->");
        if (arrow == NULL) continue;
        
        // 提取左部
        char left[MAX_SYMBOL_LEN];
        strncpy(left, line, arrow - line);
        left[arrow - line] = '\0';
        trim(left);
        
        // 提取右部
        char* right_part = arrow + 2;
        
        // 处理右部中的 '|'（多个产生式）
        char* pipe = strchr(right_part, '|');
        if (pipe != NULL) {
            // 有多个产生式，分割处理
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
                
                // 分割右部符号
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
                
                // 添加产生式
                if (token_count > 0 || (token_count == 0 && strlen(right_buf) == 0)) {
                    strcpy(productions[prod_count].left, left);
                    productions[prod_count].right_len = token_count;
                    for (int i = 0; i < token_count; i++) {
                        strcpy(productions[prod_count].right[i], tokens[i]);
                    }
                    prod_count++;
                }
            }
        } else {
            // 单个产生式
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
        
        // 记录第一个产生式的左部为开始符号
        if (prod_count > 0 && strlen(start_symbol) == 0) {
            strcpy(start_symbol, left);
        }
    }
    
    fclose(file);
    
    printf("共加载 %d 条产生式\n", prod_count);
    for (int i = 0; i < prod_count; i++) {
        printf("  %d: %s -> ", i, productions[i].left);
        for (int j = 0; j < productions[i].right_len; j++) {
            printf("%s ", productions[i].right[j]);
        }
        printf("\n");
    }
    
    // 添加增广文法 S' -> S
    char augmented_left[MAX_SYMBOL_LEN];
    // 确保不溢出：使用 snprintf
    snprintf(augmented_left, sizeof(augmented_left), "%s'", start_symbol);
    strcpy(productions[prod_count].left, augmented_left);
    productions[prod_count].right_len = 1;
    strcpy(productions[prod_count].right[0], start_symbol);
    prod_count++;
    
    printf("\n增广文法：\n");
    printf("  %d: %s -> %s\n", prod_count - 1, augmented_left, start_symbol);
    
    // 构建符号表
    for (int i = 0; i < prod_count; i++) {
        get_symbol_index(productions[i].left);
        for (int j = 0; j < productions[i].right_len; j++) {
            get_symbol_index(productions[i].right[j]);
        }
    }
    
    printf("\n符号表 (%d个符号):\n", symbol_count);
    for (int i = 0; i < symbol_count; i++) {
        printf("  %s: %s\n", symbols[i], is_terminal[i] ? "终结符" : "非终结符");
    }
}

// 判断两个项目是否相同
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

// 检查项目是否已在项目集中
int item_in_set(ItemSet* set, Item item) {
    for (int i = 0; i < set->item_count; i++) {
        if (set->items[i].prod_id == item.prod_id && 
            set->items[i].dot_pos == item.dot_pos) {
            return 1;
        }
    }
    return 0;
}

// 计算项目集的闭包
void closure(ItemSet* set) {
    int changed = 1;
    
    while (changed) {
        changed = 0;
        
        for (int i = 0; i < set->item_count; i++) {
            Item item = set->items[i];
            Production* prod = &productions[item.prod_id];
            
            // 如果圆点不在最右边，检查圆点后的符号
            if (item.dot_pos < prod->right_len) {
                char* symbol = prod->right[item.dot_pos];
                
                // 查找以 symbol 为左部的产生式
                for (int j = 0; j < prod_count; j++) {
                    if (strcmp(productions[j].left, symbol) == 0) {
                        Item new_item;
                        new_item.prod_id = j;
                        new_item.dot_pos = 0;
                        
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

// 对项目集应用 Goto 转移
ItemSet goto_set(ItemSet* set, const char* symbol) {
    ItemSet result;
    result.item_count = 0;
    result.id = -1;
    result.trans_count = 0;
    
    // 找到所有圆点后是 symbol 的项目，将圆点右移
    for (int i = 0; i < set->item_count; i++) {
        Item item = set->items[i];
        Production* prod = &productions[item.prod_id];
        
        if (item.dot_pos < prod->right_len && 
            strcmp(prod->right[item.dot_pos], symbol) == 0) {
            Item new_item;
            new_item.prod_id = item.prod_id;
            new_item.dot_pos = item.dot_pos + 1;
            if (result.item_count < MAX_ITEMS) {
                result.items[result.item_count++] = new_item;
            }
        }
    }
    
    if (result.item_count > 0) {
        // 计算闭包
        closure(&result);
    }
    
    return result;
}

// 查找项目集是否已存在
int find_item_set(ItemSet* sets, int count, ItemSet* target) {
    for (int i = 0; i < count; i++) {
        if (items_equal(sets[i].items, sets[i].item_count, 
                        target->items, target->item_count)) {
            return i;
        }
    }
    return -1;
}

// 打印项目
void print_item(Item item) {
    Production* prod = &productions[item.prod_id];
    printf("%s -> ", prod->left);
    
    for (int i = 0; i < prod->right_len; i++) {
        if (i == item.dot_pos) {
            printf("·");
        }
        printf("%s", prod->right[i]);
        if (i < prod->right_len - 1) printf(" ");
    }
    if (item.dot_pos == prod->right_len) {
        printf("·");
    }
}

// 构建 LR(0) 项目集规范族
void build_lr0_items() {
    printf("\n========== 构建 LR(0) 项目集规范族 ==========\n");
    
    // 初始项目集 I0: 包含增广文法的初始项目 S' -> ·S
    ItemSet I0;
    I0.item_count = 0;
    I0.id = 0;
    I0.trans_count = 0;
    
    Item start_item;
    start_item.prod_id = prod_count - 1;  // 增广文法的产生式编号
    start_item.dot_pos = 0;
    I0.items[I0.item_count++] = start_item;
    
    // 计算闭包
    closure(&I0);
    
    // 添加到项目集族
    item_sets[item_set_count++] = I0;
    
    // 重复处理直到没有新项目集
    int changed = 1;
    while (changed) {
        changed = 0;
        
        // 对每个已有的项目集
        for (int i = 0; i < item_set_count; i++) {
            ItemSet* current = &item_sets[i];
            
            // 对每个符号尝试 Goto
            for (int s = 0; s < symbol_count; s++) {
                char* symbol = symbols[s];
                
                ItemSet next = goto_set(current, symbol);
                if (next.item_count > 0) {
                    // 查找是否已存在
                    int existing = find_item_set(item_sets, item_set_count, &next);
                    
                    if (existing == -1) {
                        // 新项目集
                        next.id = item_set_count;
                        next.trans_count = 0;
                        if (item_set_count < MAX_ITEM_SETS) {
                            item_sets[item_set_count++] = next;
                            existing = item_set_count - 1;
                            changed = 1;
                        }
                    } else {
                        existing = existing;
                    }
                    
                    // 记录转移
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
    
    printf("共生成 %d 个项目集\n", item_set_count);
}

// 打印 LR(0) 项目集规范族
void print_item_sets() {
    printf("\n========== LR(0) 项目集规范族 ==========\n\n");
    
    for (int i = 0; i < item_set_count; i++) {
        ItemSet* set = &item_sets[i];
        printf("I%d:\n", set->id);
        
        // 打印项目
        for (int j = 0; j < set->item_count; j++) {
            printf("    ");
            print_item(set->items[j]);
            printf("\n");
        }
        
        // 打印转移
        if (set->trans_count > 0) {
            printf("    转移:\n");
            for (int t = 0; t < set->trans_count; t++) {
                printf("        %s -> I%d\n", 
                       set->trans_symbols[t], 
                       set->transitions[t]);
            }
        }
        printf("\n");
    }
}

// 检查 LR(0) 冲突
void check_conflicts() {
    printf("\n========== LR(0) 冲突检查 ==========\n");
    
    int has_conflict = 0;
    
    for (int i = 0; i < item_set_count; i++) {
        ItemSet* set = &item_sets[i];
        
        int shift_count = 0;
        int reduce_count = 0;
        char reduce_prod[MAX_SYMBOL_LEN];
        
        for (int j = 0; j < set->item_count; j++) {
            Item item = set->items[j];
            Production* prod = &productions[item.prod_id];
            
            // 检查是否为归约项目（圆点在最后）
            if (item.dot_pos == prod->right_len) {
                reduce_count++;
                strcpy(reduce_prod, prod->left);
            }
            // 检查是否有移进项目（圆点后是终结符）
            else if (item.dot_pos < prod->right_len) {
                char* symbol = prod->right[item.dot_pos];
                int sym_idx = get_symbol_index(symbol);
                if (sym_idx >= 0 && is_terminal[sym_idx]) {
                    shift_count++;
                }
            }
        }
        
        // 检查冲突
        if (reduce_count > 1) {
            printf("  I%d: ⚠ 归约-归约冲突！有 %d 个归约项目\n", set->id, reduce_count);
            has_conflict = 1;
        }
        
        if (shift_count > 0 && reduce_count > 0) {
            printf("  I%d: ⚠ 移进-归约冲突！有 %d 个移进项目，%d 个归约项目\n", 
                   set->id, shift_count, reduce_count);
            has_conflict = 1;
        }
    }
    
    if (!has_conflict) {
        printf("  ✅ 无 LR(0) 冲突，该文法是 LR(0) 文法\n");
    } else {
        printf("  ❌ 存在 LR(0) 冲突，该文法不是 LR(0) 文法\n");
    }
}

// 打印图形化表示（文本形式）
void print_graphviz() {
    printf("\n========== Graphviz 图形表示 ==========\n");
    printf("// 复制以下内容到 Graphviz 中查看状态转换图\n\n");
    printf("digraph LR0 {\n");
    printf("    rankdir=LR;\n");
    printf("    node [shape=circle];\n\n");
    
    for (int i = 0; i < item_set_count; i++) {
        ItemSet* set = &item_sets[i];
        
        // 判断是否为接受状态（包含接受项目）
        int is_accept = 0;
        for (int j = 0; j < set->item_count; j++) {
            Item item = set->items[j];
            if (item.dot_pos == productions[item.prod_id].right_len &&
                strcmp(productions[item.prod_id].left, 
                       productions[prod_count-1].left) == 0) {
                is_accept = 1;
                break;
            }
        }
        
        if (is_accept) {
            printf("    I%d [shape=doublecircle];\n", set->id);
        }
        
        for (int t = 0; t < set->trans_count; t++) {
            printf("    I%d -> I%d [label=\"%s\"];\n", 
                   set->id, set->transitions[t], set->trans_symbols[t]);
        }
    }
    
    printf("}\n");
}

// 主函数
int main() {
    char filename[256];
    int mode;
    
    printf("========================================\n");
    printf("  实验三：LR(0) 项目集规范族构建\n");
    printf("========================================\n");
    
    // 选择输入方式
    printf("\n请选择输入方式：\n");
    printf("  1：使用默认文法（算术表达式）\n");
    printf("  2：从文件读取文法\n");
    printf("请输入选择 (1 或 2)：");
    
    scanf("%d", &mode);
    getchar();
    
    if (mode == 1) {
        // 默认文法：算术表达式
        printf("\n使用默认文法：\n");
        printf("  E -> E + T | T\n");
        printf("  T -> T * F | F\n");
        printf("  F -> ( E ) | id\n");
        
        // 直接定义产生式
        // E -> E + T
        strcpy(productions[prod_count].left, "E");
        productions[prod_count].right_len = 3;
        strcpy(productions[prod_count].right[0], "E");
        strcpy(productions[prod_count].right[1], "+");
        strcpy(productions[prod_count].right[2], "T");
        prod_count++;
        
        // E -> T
        strcpy(productions[prod_count].left, "E");
        productions[prod_count].right_len = 1;
        strcpy(productions[prod_count].right[0], "T");
        prod_count++;
        
        // T -> T * F
        strcpy(productions[prod_count].left, "T");
        productions[prod_count].right_len = 3;
        strcpy(productions[prod_count].right[0], "T");
        strcpy(productions[prod_count].right[1], "*");
        strcpy(productions[prod_count].right[2], "F");
        prod_count++;
        
        // T -> F
        strcpy(productions[prod_count].left, "T");
        productions[prod_count].right_len = 1;
        strcpy(productions[prod_count].right[0], "F");
        prod_count++;
        
        // F -> ( E )
        strcpy(productions[prod_count].left, "F");
        productions[prod_count].right_len = 3;
        strcpy(productions[prod_count].right[0], "(");
        strcpy(productions[prod_count].right[1], "E");
        strcpy(productions[prod_count].right[2], ")");
        prod_count++;
        
        // F -> id
        strcpy(productions[prod_count].left, "F");
        productions[prod_count].right_len = 1;
        strcpy(productions[prod_count].right[0], "id");
        prod_count++;
        
        strcpy(start_symbol, "E");
        
        // 添加增广文法
        char augmented_left[MAX_SYMBOL_LEN];
        snprintf(augmented_left, sizeof(augmented_left), "%s'", start_symbol);
        strcpy(productions[prod_count].left, augmented_left);
        productions[prod_count].right_len = 1;
        strcpy(productions[prod_count].right[0], start_symbol);
        prod_count++;
        
        // 构建符号表
        for (int i = 0; i < prod_count; i++) {
            get_symbol_index(productions[i].left);
            for (int j = 0; j < productions[i].right_len; j++) {
                get_symbol_index(productions[i].right[j]);
            }
        }
        
        printf("\n共加载 %d 条产生式\n", prod_count - 1);
        printf("增广文法：%s -> %s\n", augmented_left, start_symbol);
        
    } else {
        printf("请输入文法文件名：");
        fgets(filename, sizeof(filename), stdin);
        filename[strcspn(filename, "\n")] = '\0';
        parse_grammar(filename);
    }
    
    // 构建 LR(0) 项目集规范族
    build_lr0_items();
    
    // 打印项目集
    print_item_sets();
    
    // 检查冲突
    check_conflicts();
    
    // 打印 Graphviz 图形
    print_graphviz();
    
    return 0;
}