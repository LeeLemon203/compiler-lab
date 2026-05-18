#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

// ==================== DFA五元组结构体 ====================
struct DFA {
    set<char> alphabet;                          // 字符集
    set<int> states;                             // 状态集
    int start_state;                             // 开始状态
    set<int> accept_states;                      // 接受状态集
    map<pair<int, char>, int> transitions;       // 状态转换表: (当前状态, 字符) -> 下一状态
};

// ==================== 工具函数：按空格分割字符串 ====================
vector<string> split(const string& s) {
    vector<string> tokens;
    stringstream ss(s);
    string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// ==================== 从文件加载DFA ====================
DFA load_dfa(const string& filename) {
    DFA dfa;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cerr << "错误：无法打开文件 " << filename << endl;
        exit(1);
    }
    
    string line;
    int section = 0;  // 0=字符集, 1=状态集, 2=开始状态, 3=接受状态集, 4=转换表
    
    while (getline(file, line)) {
        // 去除首尾空格
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // 跳过空行和注释行
        if (line.empty() || line[0] == '/') {
            continue;
        }
        
        // 遇到分隔符则切换section
        if (line == "===") {
            section++;
            continue;
        }
        
        switch (section) {
            case 0: { // 字符集
                vector<string> tokens = split(line);
                for (const string& t : tokens) {
                    if (!t.empty()) {
                        dfa.alphabet.insert(t[0]);
                    }
                }
                break;
            }
            case 1: { // 状态集
                vector<string> tokens = split(line);
                for (const string& t : tokens) {
                    dfa.states.insert(stoi(t));
                }
                break;
            }
            case 2: { // 开始状态
                dfa.start_state = stoi(line);
                break;
            }
            case 3: { // 接受状态集
                vector<string> tokens = split(line);
                for (const string& t : tokens) {
                    dfa.accept_states.insert(stoi(t));
                }
                break;
            }
            case 4: { // 转换表
                vector<string> tokens = split(line);
                if (tokens.size() == 3) {
                    int from = stoi(tokens[0]);
                    char ch = tokens[1][0];
                    int to = stoi(tokens[2]);
                    dfa.transitions[{from, ch}] = to;
                }
                break;
            }
        }
    }
    
    file.close();
    return dfa;
}

// ==================== 功能1：检查DFA合法性 ====================
bool check_dfa(const DFA& dfa) {
    bool valid = true;
    
    cout << "\n========== DFA合法性检查 ==========" << endl;
    
    // 1. 检查开始状态是否唯一（本身就是单个值，只需检查是否在状态集中）
    cout << "1. 开始状态检查: ";
    if (dfa.states.find(dfa.start_state) == dfa.states.end()) {
        cout << "❌ 错误！开始状态 " << dfa.start_state << " 不在状态集中" << endl;
        valid = false;
    } else {
        cout << "✓ 开始状态 " << dfa.start_state << " 在状态集中" << endl;
    }
    
    // 2. 检查接受状态集是否为空
    cout << "2. 接受状态集检查: ";
    if (dfa.accept_states.empty()) {
        cout << "❌ 错误！接受状态集为空" << endl;
        valid = false;
    } else {
        cout << "✓ 接受状态集非空，共 " << dfa.accept_states.size() << " 个状态: ";
        for (int s : dfa.accept_states) {
            cout << s << " ";
        }
        cout << endl;
    }
    
    // 3. 检查接受状态是否都在状态集中
    cout << "3. 接受状态子集检查: ";
    for (int s : dfa.accept_states) {
        if (dfa.states.find(s) == dfa.states.end()) {
            cout << "❌ 错误！接受状态 " << s << " 不在状态集中" << endl;
            valid = false;
        }
    }
    if (valid) {
        cout << "✓ 所有接受状态都在状态集中" << endl;
    }
    
    // 4. 检查转换表的合法性
    cout << "4. 状态转换表检查: ";
    bool trans_valid = true;
    for (const auto& trans : dfa.transitions) {
        int from = trans.first.first;
        char ch = trans.first.second;
        int to = trans.second;
        
        if (dfa.states.find(from) == dfa.states.end()) {
            cout << "❌ 转换规则中状态 " << from << " 不在状态集中" << endl;
            trans_valid = false;
        }
        if (dfa.alphabet.find(ch) == dfa.alphabet.end()) {
            cout << "❌ 转换规则中字符 '" << ch << "' 不在字符集中" << endl;
            trans_valid = false;
        }
        if (dfa.states.find(to) == dfa.states.end()) {
            cout << "❌ 转换规则中目标状态 " << to << " 不在状态集中" << endl;
            trans_valid = false;
        }
    }
    if (trans_valid) {
        cout << "✓ 共 " << dfa.transitions.size() << " 条转换规则，全部合法" << endl;
    } else {
        valid = false;
    }
    
    // 5. 检查转换表的完整性（每个状态对每个字符是否都有定义）
    cout << "5. 转换表完整性检查: ";
    bool complete = true;
    for (int s : dfa.states) {
        for (char c : dfa.alphabet) {
            if (dfa.transitions.find({s, c}) == dfa.transitions.end()) {
                cout << "⚠ 状态" << s << "在字符'" << c << "'上缺少转换定义" << endl;
                complete = false;
            }
        }
    }
    if (complete) {
        cout << "✓ 转换表完整（每个状态对每个字符都有定义）" << endl;
    }
    
    if (valid) {
        cout << "\n✅ DFA合法性检查通过！" << endl;
    } else {
        cout << "\n❌ DFA合法性检查不通过！" << endl;
    }
    cout << "=====================================\n" << endl;
    
    return valid;
}

// ==================== 功能2：枚举长度≤N的所有规则字符串（BFS） ====================
vector<string> enumerate_strings(const DFA& dfa, int N) {
    vector<string> result;
    set<string> result_set;  // 用于去重结果
    
    // 队列元素: (当前状态, 当前字符串)
    queue<pair<int, string>> q;
    q.push({dfa.start_state, ""});
    
    cout << "\n========== 开始枚举长度≤" << N << "的字符串 ==========" << endl;
    cout << "枚举过程（按BFS顺序）：" << endl;
    
    while (!q.empty()) {
        auto [current_state, current_str] = q.front();
        q.pop();
        
        int len = current_str.length();
        
        // 如果当前状态是接受状态，且结果集中没有当前字符串
        if (dfa.accept_states.find(current_state) != dfa.accept_states.end()) {
            if (result_set.find(current_str) == result_set.end()) {
                string output = current_str.empty() ? "(空串/ε)" : current_str;
                cout << "  ✓ 接受: \"" << output << "\" (到达状态 " << current_state << ")" << endl;
                result.push_back(current_str);
                result_set.insert(current_str);
            }
        }
        
        // 如果已达到长度上限N，不再继续扩展
        if (len >= N) {
            continue;
        }
        
        // 遍历字符集，扩展下一状态
        for (char c : dfa.alphabet) {
            auto it = dfa.transitions.find({current_state, c});
            if (it != dfa.transitions.end()) {
                int next_state = it->second;
                string next_str = current_str + c;
                q.push({next_state, next_str});
            }
        }
    }
    
    cout << "\n共枚举到 " << result.size() << " 个规则字符串" << endl;
    cout << "===========================================\n" << endl;
    
    return result;
}

// ==================== 功能3：判断输入字符串是否被DFA接受 ====================
bool accept_string(const DFA& dfa, const string& input) {
    cout << "\n========== 字符串识别过程 ==========" << endl;
    cout << "输入字符串: \"" << input << "\"" << endl;
    
    int current_state = dfa.start_state;
    cout << "初始状态: " << current_state << endl;
    
    // 处理空串
    if (input.empty()) {
        cout << "输入为空串" << endl;
        bool accepted = (dfa.accept_states.find(current_state) != dfa.accept_states.end());
        if (accepted) {
            cout << "当前状态 " << current_state << " 是接受状态 → ✅ 接受" << endl;
        } else {
            cout << "当前状态 " << current_state << " 不是接受状态 → ❌ 拒绝" << endl;
        }
        cout << "=====================================\n" << endl;
        return accepted;
    }
    
    // 逐字符处理
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        cout << "读入字符 '" << c << "' (第" << (i+1) << "个): ";
        
        auto it = dfa.transitions.find({current_state, c});
        if (it == dfa.transitions.end()) {
            cout << "状态" << current_state << "没有字符'" << c << "'的转换 → ❌ 拒绝（无此转换）" << endl;
            cout << "=====================================\n" << endl;
            return false;
        }
        
        current_state = it->second;
        cout << "状态 " << (it->first.first) << " → 状态 " << current_state << endl;
    }
    
    // 判断最终状态是否在接受状态集中
    bool accepted = (dfa.accept_states.find(current_state) != dfa.accept_states.end());
    cout << "\n最终状态: " << current_state;
    if (accepted) {
        cout << " 是接受状态 → ✅ 接受" << endl;
    } else {
        cout << " 不是接受状态 → ❌ 拒绝" << endl;
    }
    cout << "=====================================\n" << endl;
    
    return accepted;
}

// ==================== 菜单交互 ====================
void show_menu() {
    cout << "\n╔══════════════════════════════════╗" << endl;
    cout << "║        DFA 模拟器 - 实验1        ║" << endl;
    cout << "╠══════════════════════════════════╣" << endl;
    cout << "║  1. 枚举长度≤N的规则字符串      ║" << endl;
    cout << "║  2. 判断输入字符串是否被接受    ║" << endl;
    cout << "║  3. 退出                        ║" << endl;
    cout << "╚══════════════════════════════════╝" << endl;
    cout << "请选择操作: ";
}

// ==================== 主函数 ====================
int main() {
    string filename = "dfa.txt";
    
    cout << "========================================" << endl;
    cout << "  编译器设计实验1 - DFA模拟实现" << endl;
    cout << "========================================" << endl;
    cout << "加载DFA配置文件: " << filename << endl;
    
    // 加载DFA
    DFA dfa = load_dfa(filename);
    
    // 显示DFA信息
    cout << "\n--- DFA信息 ---" << endl;
    cout << "字符集: ";
    for (char c : dfa.alphabet) cout << c << " ";
    cout << "\n状态集: ";
    for (int s : dfa.states) cout << s << " ";
    cout << "\n开始状态: " << dfa.start_state << endl;
    cout << "接受状态集: ";
    for (int s : dfa.accept_states) cout << s << " ";
    cout << "\n转换表: " << dfa.transitions.size() << " 条规则" << endl;
    for (const auto& t : dfa.transitions) {
        cout << "  δ(" << t.first.first << ", '" << t.first.second << "') = " << t.second << endl;
    }
    cout << "--------------------" << endl;
    
    // 检查DFA合法性
    bool valid = check_dfa(dfa);
    if (!valid) {
        cout << "DFA不合法，程序终止。请检查配置文件后重新运行。" << endl;
        return 1;
    }
    
    // 交互循环
    while (true) {
        show_menu();
        int choice;
        cin >> choice;
        cin.ignore();  // 清除缓冲区中的换行符
        
        switch (choice) {
            case 1: {
                cout << "请输入最大长度 N: ";
                int N;
                cin >> N;
                cin.ignore();
                if (N < 0) {
                    cout << "N必须为非负整数！" << endl;
                    break;
                }
                vector<string> strings = enumerate_strings(dfa, N);
                cout << "\n结果列表: ";
                if (strings.empty()) {
                    cout << "(空)" << endl;
                } else {
                    cout << "{ ";
                    for (size_t i = 0; i < strings.size(); i++) {
                        cout << (strings[i].empty() ? "ε" : "\"" + strings[i] + "\"");
                        if (i != strings.size() - 1) cout << ", ";
                    }
                    cout << " }" << endl;
                }
                break;
            }
            case 2: {
                cout << "请输入待判定的字符串（不要包含空格）: ";
                string input;
                cin >> input;
                cin.ignore();
                accept_string(dfa, input);
                break;
            }
            case 3: {
                cout << "程序退出，感谢使用！" << endl;
                return 0;
            }
            default: {
                cout << "无效选项，请重新选择！" << endl;
                break;
            }
        }
    }
    
    return 0;
}
