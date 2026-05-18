/**
 * 实验二：基于DFA的词法分析器
 * 从 dfa.txt 读取词法规则，使用 DFA 状态机识别 Token
 * 输出格式：(Token类型, 原始单词)
 * 
 * 编译：gcc scanner.c -o scanner
 * 运行：./scanner
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// ==================== 数据结构定义 ====================

#define MAX_STATES 100
#define MAX_ALPHABET 128
#define MAX_TOKEN_LEN 256

// DFA结构体
typedef struct {
    char alphabet[MAX_ALPHABET];           // 字符集
    int alphabet_size;                      // 字符集大小
    int states[MAX_STATES];                 // 状态集
    int state_count;                        // 状态数量
    int start_state;                        // 开始状态
    int accept_states[MAX_STATES];          // 接受状态集
    int accept_count;                       // 接受状态数量
    int accept_type[MAX_STATES];            // 每个接受状态对应的Token类型
    int transition[MAX_STATES][MAX_ALPHABET]; // 转换表
    int transition_defined[MAX_STATES][MAX_ALPHABET]; // 是否定义
} DFA;

// Token类型枚举
typedef enum {
    TOKEN_ID,       // 标识符
    TOKEN_INT,      // 整型常量
    TOKEN_FLOAT,    // 浮点常量
    TOKEN_SCO,      // 分号 ;
    TOKEN_ASSIGN,   // 等号 =
    TOKEN_PLUS,     // 加号 +
    TOKEN_MINUS,    // 减法 -
    TOKEN_DOT,      // 小数点 .
    TOKEN_ERROR     // 错误/未知
} TokenType;

// Token类型对应的字符串名称
const char* token_names[] = {
    "ID", "INT", "FLOAT", "SCO", "ASSIGN", 
    "PLUS", "MINUS", "DOT", "ERROR"
};

// 全局变量
DFA dfa;
FILE* output_file = NULL;
int char_to_index[256];

// ==================== DFA加载函数 ====================

void init_dfa(DFA* dfa) {
    dfa->alphabet_size = 0;
    dfa->state_count = 0;
    dfa->start_state = -1;
    dfa->accept_count = 0;
    
    for (int i = 0; i < MAX_STATES; i++) {
        dfa->accept_type[i] = TOKEN_ERROR;
        for (int j = 0; j < MAX_ALPHABET; j++) {
            dfa->transition[i][j] = -1;
            dfa->transition_defined[i][j] = 0;
        }
    }
    
    for (int i = 0; i < 256; i++) {
        char_to_index[i] = -1;
    }
}

int load_dfa(const char* filename, DFA* dfa) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("错误：无法打开DFA文件 %s\n", filename);
        return 0;
    }
    
    init_dfa(dfa);
    
    char line[1024];
    int section = 0;
    int line_num = 0;
    int alphabet_loaded = 0;  // 标记字符集是否已加载
    
    printf("\n正在加载 DFA 规则文件: %s\n", filename);
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        line[strcspn(line, "\n")] = '\0';
        
        // 跳过空行和注释
        if (strlen(line) == 0 || line[0] == '#') continue;
        
        if (strcmp(line, "===") == 0) {
            section++;
            continue;
        }
        
        switch (section) {
            case 0: { // 字符集 - 只加载第一个非注释行
                if (!alphabet_loaded) {
                    alphabet_loaded = 1;
                    char* token = strtok(line, " \t");
                    while (token != NULL && dfa->alphabet_size < MAX_ALPHABET) {
                        if (strlen(token) == 1) {
                            char c = token[0];
                            dfa->alphabet[dfa->alphabet_size] = c;
                            char_to_index[(int)c] = dfa->alphabet_size;
                            dfa->alphabet_size++;
                        }
                        token = strtok(NULL, " \t");
                    }
                }
                break;
            }
            case 1: { // 状态集
                char* token = strtok(line, " \t");
                while (token != NULL && dfa->state_count < MAX_STATES) {
                    dfa->states[dfa->state_count++] = atoi(token);
                    token = strtok(NULL, " \t");
                }
                break;
            }
            case 2: { // 开始状态
                dfa->start_state = atoi(line);
                break;
            }
            case 3: { // 接受状态集
                char* token = strtok(line, " \t");
                while (token != NULL && dfa->accept_count < MAX_STATES) {
                    dfa->accept_states[dfa->accept_count++] = atoi(token);
                    token = strtok(NULL, " \t");
                }
                break;
            }
            case 4: { // 转换表
                char from_str[32], ch_str[32], to_str[32];
                if (sscanf(line, "%s %s %s", from_str, ch_str, to_str) == 3) {
                    int from = atoi(from_str);
                    char ch = ch_str[0];
                    int to = atoi(to_str);
                    
                    int ch_idx = char_to_index[(int)ch];
                    if (ch_idx >= 0) {
                        int from_idx = -1;
                        for (int i = 0; i < dfa->state_count; i++) {
                            if (dfa->states[i] == from) {
                                from_idx = i;
                                break;
                            }
                        }
                        if (from_idx >= 0) {
                            dfa->transition[from_idx][ch_idx] = to;
                            dfa->transition_defined[from_idx][ch_idx] = 1;
                        }
                    }
                }
                break;
            }
        }
    }
    
    fclose(file);
    
    printf("  字符集 (%d): ", dfa->alphabet_size);
    for (int i = 0; i < dfa->alphabet_size && i < 30; i++) {
        printf("%c ", dfa->alphabet[i]);
    }
    if (dfa->alphabet_size > 30) printf("...");
    printf("\n");
    printf("  状态数: %d\n", dfa->state_count);
    printf("  开始状态: %d\n", dfa->start_state);
    printf("  接受状态数: %d\n", dfa->accept_count);
    
    // 设置接受状态的Token类型映射
    for (int i = 0; i < dfa->accept_count; i++) {
        int state = dfa->accept_states[i];
        if (state == 1) dfa->accept_type[i] = TOKEN_ID;
        else if (state == 2) dfa->accept_type[i] = TOKEN_PLUS;
        else if (state == 3) dfa->accept_type[i] = TOKEN_MINUS;
        else if (state == 4) dfa->accept_type[i] = TOKEN_ASSIGN;
        else if (state == 5) dfa->accept_type[i] = TOKEN_SCO;
        else if (state == 6) dfa->accept_type[i] = TOKEN_DOT;
        else if (state == 7) dfa->accept_type[i] = TOKEN_INT;
        else if (state == 8) dfa->accept_type[i] = TOKEN_FLOAT;
        else if (state == 11) dfa->accept_type[i] = TOKEN_FLOAT;
        else if (state == 12) dfa->accept_type[i] = TOKEN_FLOAT;
        else dfa->accept_type[i] = TOKEN_ID;
    }
    
    printf("  DFA加载成功！\n");
    return 1;
}

// ==================== 辅助函数 ====================

int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// ==================== 基于DFA的词法分析 ====================

TokenType recognize_token_with_dfa(const char* input, int* pos, char* token) {
    int len = strlen(input);
    
    // 跳过空白符
    while (*pos < len && is_whitespace(input[*pos])) {
        (*pos)++;
    }
    
    if (*pos >= len) {
        return TOKEN_ERROR;
    }
    
    // 找到开始状态的索引
    int current_state_idx = -1;
    for (int i = 0; i < dfa.state_count; i++) {
        if (dfa.states[i] == dfa.start_state) {
            current_state_idx = i;
            break;
        }
    }
    
    if (current_state_idx == -1) {
        token[0] = input[*pos];
        token[1] = '\0';
        (*pos)++;
        return TOKEN_ERROR;
    }
    
    int start_pos = *pos;
    int current_state_value = dfa.start_state;
    int last_accept_pos = -1;
    TokenType last_accept_type = TOKEN_ERROR;
    
    while (*pos < len) {
        char c = input[*pos];
        
        // 如果是空白符，停止
        if (is_whitespace(c)) {
            break;
        }
        
        int ch_idx = char_to_index[(int)c];
        
        // 如果字符不在字符集中，停止
        if (ch_idx < 0) {
            break;
        }
        
        // 检查是否有定义转换
        if (!dfa.transition_defined[current_state_idx][ch_idx]) {
            break;
        }
        
        // 状态转移
        int next_state_value = dfa.transition[current_state_idx][ch_idx];
        
        // 找到下一状态的索引
        int next_state_idx = -1;
        for (int i = 0; i < dfa.state_count; i++) {
            if (dfa.states[i] == next_state_value) {
                next_state_idx = i;
                break;
            }
        }
        
        if (next_state_idx == -1) {
            break;
        }
        
        current_state_idx = next_state_idx;
        current_state_value = next_state_value;
        (*pos)++;
        
        // 检查当前状态是否为接受状态
        for (int i = 0; i < dfa.accept_count; i++) {
            if (dfa.accept_states[i] == current_state_value) {
                last_accept_pos = *pos;
                last_accept_type = dfa.accept_type[i];
                strncpy(token, input + start_pos, last_accept_pos - start_pos);
                token[last_accept_pos - start_pos] = '\0';
            }
        }
    }
    
    if (last_accept_pos > start_pos) {
        *pos = last_accept_pos;
        strncpy(token, input + start_pos, last_accept_pos - start_pos);
        token[last_accept_pos - start_pos] = '\0';
        return last_accept_type;
    }
    
    // 没有识别到有效token
    if (start_pos < len) {
        token[0] = input[start_pos];
        token[1] = '\0';
        (*pos) = start_pos + 1;
    }
    return TOKEN_ERROR;
}

void print_token(TokenType type, const char* token) {
    printf("(%s, %s)\n", token_names[type], token);
    if (output_file) {
        fprintf(output_file, "(%s, %s)\n", token_names[type], token);
    }
}

// ==================== 模式2 ====================

void mode2() {
    char line[2048];
    getchar();
    
    printf("请输入一行语句：");
    if (fgets(line, sizeof(line), stdin) == NULL) return;
    line[strcspn(line, "\n")] = '\0';
    
    if (output_file) {
        fprintf(output_file, "# 模式2 - 整句词法分析\n");
        fprintf(output_file, "# 输入语句: %s\n", line);
        fprintf(output_file, "# ================================\n\n");
    }
    
    printf("\n词法分析结果：\n");
    
    int pos = 0;
    char token[MAX_TOKEN_LEN];
    TokenType type;
    
    while (pos < (int)strlen(line)) {
        type = recognize_token_with_dfa(line, &pos, token);
        if (type != TOKEN_ERROR) {
            print_token(type, token);
        } else if (strlen(token) > 0) {
            printf("(ERROR, %s)\n", token);
            if (output_file) fprintf(output_file, "(ERROR, %s)\n", token);
        }
    }
}

// ==================== 模式3 ====================

void mode3() {
    char input_filename[256];
    char output_filename[256];
    char line[4096];
    
    getchar();
    
    printf("请输入源代码文件名：");
    if (fgets(input_filename, sizeof(input_filename), stdin) == NULL) return;
    input_filename[strcspn(input_filename, "\n")] = '\0';
    
    printf("请输入输出文件名（直接回车使用 token_output.txt）：");
    if (fgets(output_filename, sizeof(output_filename), stdin) == NULL) return;
    output_filename[strcspn(output_filename, "\n")] = '\0';
    
    if (strlen(output_filename) == 0) {
        strcpy(output_filename, "token_output.txt");
    }
    
    FILE* input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        printf("错误：无法打开输入文件 %s\n", input_filename);
        return;
    }
    
    output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        printf("错误：无法创建输出文件 %s\n", output_filename);
        fclose(input_file);
        return;
    }
    
    printf("正在分析文件: %s\n", input_filename);
    printf("Token流将保存到: %s\n", output_filename);
    
    fprintf(output_file, "# 词法分析输出文件\n");
    fprintf(output_file, "# 源文件: %s\n", input_filename);
    fprintf(output_file, "# ================================\n\n");
    
    int line_num = 0;
    int total_tokens = 0;
    
    while (fgets(line, sizeof(line), input_file)) {
        line_num++;
        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) == 0) continue;
        
        int pos = 0;
        char token[MAX_TOKEN_LEN];
        TokenType type;
        
        while (pos < (int)strlen(line)) {
            type = recognize_token_with_dfa(line, &pos, token);
            if (type != TOKEN_ERROR) {
                print_token(type, token);
                total_tokens++;
            } else if (strlen(token) > 0) {
                printf("(ERROR, %s)\n", token);
                if (output_file) fprintf(output_file, "(ERROR, %s)\n", token);
                total_tokens++;
            }
        }
    }
    
    printf("\n分析完成！共处理 %d 行，识别 %d 个Token\n", line_num, total_tokens);
    printf("Token流已保存到: %s\n", output_filename);
    
    fclose(input_file);
    fclose(output_file);
    output_file = NULL;
}

// ==================== 主函数 ====================

int main() {
    int mode;
    char save_to_file[10];
    char output_filename[256];
    
    printf("========================================\n");
    printf("    词法分析器 Scanner (实验二)\n");
    printf("    基于 DFA 规则文件\n");
    printf("========================================\n");
    
    // 加载 DFA 规则文件
    if (!load_dfa("dfa.txt", &dfa)) {
        printf("无法加载 DFA 文件，程序退出\n");
        return 1;
    }
    
    printf("\n是否将结果保存到文件？(y/n)：");
    if (scanf("%s", save_to_file) == 1) {
        if (save_to_file[0] == 'y' || save_to_file[0] == 'Y') {
            printf("请输入输出文件名（直接回车使用 token_output.txt）：");
            if (scanf("%s", output_filename) != 1 || strlen(output_filename) == 0) {
                strcpy(output_filename, "token_output.txt");
            }
            output_file = fopen(output_filename, "w");
            if (output_file) {
                printf("结果将同时保存到文件: %s\n", output_filename);
                fprintf(output_file, "# 词法分析输出文件\n");
                fprintf(output_file, "# ================================\n\n");
            }
        }
    }
    
    printf("\n请选择运行模式：\n");
    printf("  模式2：整句词法分析\n");
    printf("  模式3：从文件读取源代码并分析\n");
    printf("请输入模式 (2 或 3)：");
    
    if (scanf("%d", &mode) != 1) {
        printf("输入错误，程序退出\n");
        if (output_file) fclose(output_file);
        return 1;
    }
    
    printf("\n");
    
    switch(mode) {
        case 2:
            mode2();
            break;
        case 3:
            mode3();
            break;
        default:
            printf("无效模式\n");
            if (output_file) fclose(output_file);
            return 1;
    }
    
    if (output_file) {
        fclose(output_file);
        printf("\n词法分析结果已保存到文件！\n");
    }
    
    printf("\n词法分析完成！\n");
    return 0;
}