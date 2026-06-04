# 编译原理实验平台

## 项目简介

本项目是《编译器设计专题实验》的完整实现，包含六个实验的 C/C++ 代码及前端可视化展示。

- **实验一**：DFA 模拟器
- **实验二**：词法分析器 (Scanner)
- **实验三**：LR(0) 项目集规范族构建
- **实验四**：SLR(1) 文法分析
- **实验五**：语义分析 (符号表 + AST)
- **实验六**：中间代码生成 (三地址码/四元式)

---

## 项目架构

```
compiler-lab/
│
├── index.html                          # 主页面（前端可视化入口）
├── style.css                           # 样式文件
├── app.js                              # 页面切换逻辑
│
├── experiment1/                        # 实验一：DFA模拟器
│   ├── dfa.cpp                         # DFA核心实现 (C++)
│   └── dfa_in_1.txt                    # DFA配置文件
│
├── experiment2/                        # 实验二：词法分析器
│   ├── scanner.c                       # 词法分析器实现 (C)
│   ├── dfa.txt                         # DFA词法规则
│   └── test.src                        # 测试源代码
│
├── experiment3/                        # 实验三：LR(0)项目集
│   ├── lr0.c                           # LR(0)核心实现 (C)
│   └── grammer.txt                     # 文法规则文件
│
├── experiment4/                        # 实验四：SLR(1)文法分析
│   ├── slr1.c                          # SLR(1)核心实现 (C)
│   └── grammer.txt                     # 文法规则文件
│
├── experiment5/                        # 实验五：语义分析
│   ├── experiment5.c                   # 语义分析实现 (C)
│   └── test.c                          # 测试源代码
│
├── experiment6/                        # 实验六：中间代码生成
│   ├── experiment6.c                   # 中间代码生成实现 (C)
│   └── test.c                          # 测试源代码
│
└── README.md                           # 项目说明文档
```

---

## 实验一：DFA 模拟器

### 文件说明

| 文件 | 说明 |
|------|------|
| `experiment1/dfa.cpp` | DFA 核心实现，包含五元组加载、合法性检查、枚举接受字符串、字符串识别 |
| `experiment1/dfa_in_1.txt` | DFA 配置文件，格式为五元组 + 转换表 |

### dfa_in_1.txt 格式

```
a b                    # 字符集
===                    # 分隔符
1 2 3 4                # 状态集
===                    # 分隔符
1                      # 开始状态
===                    # 分隔符
4                      # 接受状态集
===                    # 分隔符
1 a 2                  # 转换表：当前状态 字符 下一状态
1 b 3
2 a 4
2 b 3
3 a 2
3 b 4
4 a 4
4 b 4
```

### 当前 DFA 说明

- **字符集**: {a, b}
- **状态集**: {1, 2, 3, 4}
- **开始状态**: 1
- **接受状态**: {4}
- **功能**: 识别包含连续两个相同字符 (aa 或 bb) 的字符串

### 功能说明

| 功能 | 对应函数 | 说明 |
|------|----------|------|
| 加载DFA | `load_dfa()` | 从文件读取五元组 |
| 合法性检查 | `check_dfa()` | 检查开始状态、接受状态、转换表完整性 |
| 枚举字符串 | `enumerate_strings()` | BFS枚举所有长度≤N的接受字符串 |
| 字符串识别 | `accept_string()` | 判断输入字符串是否被接受 |

### 编译运行

```bash
cd experiment1
g++ dfa.cpp -o dfa
./dfa
```

### 运行示例

```
========================================
  编译器设计实验1 - DFA模拟实现
========================================
加载DFA配置文件: dfa.txt

--- DFA信息 ---
字符集: a b
状态集: 1 2 3 4
开始状态: 1
接受状态集: 4
转换表: 8 条规则

========== DFA合法性检查 ==========
✅ DFA合法性检查通过！

╔══════════════════════════════════╗
║        DFA 模拟器 - 实验1        ║
╠══════════════════════════════════╣
║  1. 枚举长度≤N的规则字符串      ║
║  2. 判断输入字符串是否被接受    ║
║  3. 退出                        ║
╚══════════════════════════════════╝
```

---

## 实验二：词法分析器 (Scanner)

### 文件说明

| 文件 | 说明 |
|------|------|
| `experiment2/scanner.c` | 词法分析器实现，基于 DFA 识别 Token |
| `experiment2/dfa.txt` | DFA 词法规则文件（字符集、状态集、转换表） |
| `experiment2/test.src` | 测试源代码文件 |

### 支持的 Token 类型

| Token | 说明 | 正规式 | 示例 |
|-------|------|--------|------|
| ID | 标识符 | letter (letter \| digit)* | int, a, num123 |
| INT | 整型常量 | digit+ | 10, 123, -100 |
| FLOAT | 浮点常量 | 小数形式/科学计数法 | 3.14, .5, 1e-3 |
| ASSIGN | 赋值符 | = | = |
| PLUS | 加法运算符 | + | + |
| MINUS | 减法运算符 | - | - |
| SCO | 分号 | ; | ; |
| DOT | 小数点 | . | . |

### 浮点数支持格式

1. **小数形式**: `digit+ . digit*`，如 `123.45`、`66.`
2. **小数点开头形式**: `. digit+`，如 `.45`、`.9`
3. **科学计数法**: `(digit+ | digit+.digit* | .digit+) (e|E)(+|-)? digit+`，如 `1e-3`、`12.3E+4`

### 输出格式

```
(ID, int)
(ID, a)
(ASSIGN, =)
(INT, 10)
(SCO, ;)
```

### 编译运行

```bash
cd experiment2
gcc scanner.c -o scanner
./scanner
```

---

## 实验三：LR(0) 项目集规范族

### 文件说明

| 文件 | 说明 |
|------|------|
| `experiment3/lr0.c` | LR(0) 核心实现，生成项目集规范族、GOTO图、冲突检测 |
| `experiment3/grammer.txt` | 文法规则文件 |

### grammer.txt 格式

```
# 算术表达式文法
E -> E + T
E -> T
T -> T * F
T -> F
F -> ( E )
F -> id
```

### 功能说明

| 功能 | 对应函数 | 说明 |
|------|----------|------|
| 解析文法 | `parse_grammar()` | 从文件读取文法规则 |
| 增广文法 | - | 自动添加 S' → S |
| 闭包计算 | `closure()` | 递归添加所有可达项目 |
| Goto 计算 | `goto_set()` | 计算符号转移后的项目集 |
| 构建项目集 | `build_lr0_items()` | 生成完整 LR(0) 项目集规范族 |
| 冲突检测 | `check_conflicts()` | 检测移进-归约和归约-归约冲突 |

### LR(0) 项目分类

| 类型 | 定义 | 示例 |
|------|------|------|
| 移进项目 | 圆点后是终结符 | E → E·+T |
| 待约项目 | 圆点后是非终结符 | E → ·T |
| 归约项目 | 圆点在最后 | E → E+T· |
| 接受项目 | 圆点在最后且左部是增广文法的开始符号 | E' → E· |

### 输出示例

```
I0:
    E' -> ·E
    E -> ·E + T
    E -> ·T
    T -> ·T * F
    T -> ·F
    F -> ·( E )
    F -> ·id
    转移:
        E -> I1
        T -> I2
        F -> I3
        ( -> I4
        id -> I5

I1:
    E' -> E·
    E -> E· + T
    转移:
        + -> I6
...
```

### 编译运行

```bash
cd experiment3
gcc lr0.c -o lr0
./lr0
```

---

## 实验四：SLR(1) 文法分析

### 文件说明

| 文件 | 说明 |
|------|------|
| `experiment4/slr1.c` | SLR(1) 核心实现，构建分析表、FIRST/FOLLOW集、语法分析 |
| `experiment4/grammer.txt` | 文法规则文件 |

### 功能说明

| 功能 | 对应函数 | 说明 |
|------|----------|------|
| 解析文法 | `parse_grammar()` | 从文件读取文法规则 |
| 增广文法 | - | 自动添加 S' → S |
| 闭包计算 | `closure()` | 递归添加所有可达项目 |
| Goto 计算 | `goto_set()` | 计算符号转移后的项目集 |
| 构建项目集 | `build_lr0_items()` | 生成 LR(0) 项目集规范族 |
| FIRST集计算 | `compute_first()` | 计算所有非终结符的FIRST集 |
| FOLLOW集计算 | `compute_follow()` | 计算所有非终结符的FOLLOW集 |
| 构建分析表 | `build_slr1_table()` | 构建 SLR(1) ACTION/GOTO 表 |
| 语法分析 | `parse_input()` | 基于 SLR(1) 分析表进行语法分析 |

### FIRST/FOLLOW 集输出示例

```
========== Computing FIRST Sets ==========
  FIRST(E) = { ( id }
  FIRST(T) = { ( id }
  FIRST(F) = { ( id }

========== Computing FOLLOW Sets ==========
  FOLLOW(E) = { $ + ) }
  FOLLOW(T) = { $ + ) * }
  FOLLOW(F) = { $ + ) * }
```

### SLR(1) 分析表输出示例

```
ACTION Table:
        +         *         (         )         id        $
     --------------------------------------------------------
I0                         s4                 s5          
I1    s6                           r1                 r1  
I2    r2    s7                    r2                 r2  
I3    r4    r4                    r4                 r4  
I4                         s4                 s5          
I5    r6    r6                    r6                 r6  
I6                         s4                 s5          
I7                         s4                 s5          
I8    s6                           r0                 r0  
I9    r3    r3                    r3                 r3  
I10   r5    r5                    r5                 r5  

GOTO Table:
        E         T         F
     -------------------------
I0    1         2         3
I4    8         2         3
I6    9         3
I7         10
```

### 语法分析演示

```
Enter input string (space separated, e.g., id + id * id):
id + id * id

Step     State Stack                     Symbol Stack           Input                     Action
----------------------------------------------------------------
1        0                                                     id + id * id $             s5
2        0 5                             id                    + id * id $               r6
3        0 3                             F                     + id * id $               r4
4        0 2                             T                     + id * id $               r2
5        0 1                             E                     + id * id $               s6
6        0 1 6                           E +                   id * id $                 s5
7        0 1 6 5                         E + id                * id $                    r6
8        0 1 6 3                         E + F                 * id $                    r4
9        0 1 6 9                         E + T                 * id $                    s7
10       0 1 6 9 7                       E + T *               id $                      s5
11       0 1 6 9 7 5                     E + T * id            $                         r6
12       0 1 6 9 7 10                    E + T * F             $                         r3
13       0 1 6 9                         E + T                 $                         r1
14       0 1                             E                     $                         acc

[SUCCESS] Parsing completed!
```

### 编译运行

```bash
cd experiment4
gcc slr1.c -o slr1
./slr1
```

---

## 实验五：语义分析

### 文件说明

| 文件 | 说明 |
|------|------|
| `experiment5/experiment5.c` | 语义分析实现，包含词法分析、语法分析、符号表管理、类型检查、AST构建 |
| `experiment5/test.c` | 测试源代码文件 |

### 支持的语法特性

| 特性 | 说明 | 示例 |
|------|------|------|
| 变量声明 | int / float | `int x;` |
| 赋值语句 | = | `x = 10;` |
| 表达式 | + - * / | `z = x + y;` |
| if语句 | if (条件) { ... } | `if (x > 5) { x = x - 5; }` |
| while语句 | while (条件) { ... } | `while (x < 100) { x = x + 10; }` |
| 作用域 | { ... } | 块级作用域支持 |

### 符号表管理

```c
typedef struct Symbol {
    char name[MAX_SYMBOL_LEN];
    DataType type;           // TYPE_INT, TYPE_FLOAT, TYPE_VOID
    int scope_level;         // 作用域层级
    int is_initialized;      // 是否已初始化
    struct Symbol* next;
} Symbol;

typedef struct Scope {
    Symbol* symbols;
    int level;
    struct Scope* parent;
} Scope;
```

### 抽象语法树 (AST) 节点类型

```c
typedef enum {
    AST_PROGRAM, AST_VAR_DECL, AST_ASSIGNMENT, AST_BINARY_OP,
    AST_VARIABLE, AST_CONSTANT, AST_IF_STMT, AST_WHILE_STMT, 
    AST_BLOCK, AST_RETURN_STMT, AST_FUNC_CALL, AST_ARRAY_ACCESS,
    AST_FUNC_DEF, AST_UNARY_OP, AST_PRINT_STMT
} ASTNodeType;
```

### 输出示例

```
========== Symbol Table ==========
Scope level 0:
  x : int (uninitialized)
  y : int (uninitialized)
  z : float (uninitialized)

========== Abstract Syntax Tree (AST) ==========
Program
  VarDecl: x
  VarDecl: y
  VarDecl: z
  Assignment: x =
    Const: 10
  Assignment: y =
    Const: 20
  Assignment: z =
    BinaryOp: +
      Variable: x
      Variable: y
  IfStmt
    BinaryOp: >
      Variable: x
      Const: 5
    Block
      Assignment: x =
        BinaryOp: -
          Variable: x
          Const: 5
  WhileStmt
    BinaryOp: <
      Variable: x
      Const: 100
    Block
      Assignment: x =
        BinaryOp: +
          Variable: x
          Const: 10

[OK] Semantic analysis complete, no errors
```

### 编译运行

```bash
cd experiment5
gcc experiment5.c -o exp5
./exp5 test.c
```

---

## 实验六：中间代码生成

### 文件说明

| 文件 | 说明 |
|------|------|
| `experiment6/experiment6.c` | 中间代码生成实现，生成三地址码和四元式 |
| `experiment6/test.c` | 测试源代码文件 |

### 四元式 (Quadruple) 格式

```
(op, arg1, arg2, result)
```

### 三地址码 (Three-Address Code) 格式

```
result = arg1 op arg2
```

### 支持的运算符

| 类型 | 运算符 |
|------|--------|
| 算术运算符 | +, -, *, / |
| 关系运算符 | <, <=, >, >=, ==, != |
| 逻辑运算符 | &&, \|\|, ! |
| 赋值运算符 | =, +=, -=, *=, /= |
| 自增/自减 | ++, -- |

### 控制流处理

| 语句 | 生成的代码 |
|------|-----------|
| if (cond) { ... } | 条件跳转 + 标签 |
| if (cond) { ... } else { ... } | 条件跳转 + 无条件跳转 + 标签 |
| while (cond) { ... } | 标签 + 条件跳转 + 循环体 + 回跳 |

### 输出示例

```
========== Intermediate Code (Quadruples) ==========
1. (=, 10, , x)
2. (=, 20, , y)
3. (+, x, y, t0)
4. (=, t0, , z)
5. (if x > 5, , , goto L100)
6. (-, x, 5, t1)
7. (=, t1, , x)
8. (label, L100, , )
9. (label, L200, , )
10. (if x < 100, , , goto L300)
11. (+, x, 10, t2)
12. (=, t2, , x)
13. (goto, , , L200)
14. (label, L300, , )

========== Three-Address Code ==========
x = 10
y = 20
t0 = x + y
z = t0
if x > 5 goto L100
t1 = x - 5
x = t1
L100:
L200:
if x < 100 goto L300
t2 = x + 10
x = t2
goto L200
L300:

[OK] Code generation complete, no errors. Generated 14 quadruples
```

### 编译运行

```bash
cd experiment6
gcc experiment6.c -o exp6
./exp6 test.c
```

---

## 前端可视化

本项目同时提供 Web 前端界面，用于可视化展示六个实验的核心功能：

- **实验一**：DFA 状态转移图、合法性检查结果、枚举字符串、字符串测试
- **实验二**：Token 流输出
- **实验三**：产生式列表、项目集规范族、GOTO 图、冲突检查
- **实验四**：LR(0)项目集、FIRST/FOLLOW集、SLR分析表、GOTO图、语法分析演示
- **实验五**：符号表、抽象语法树(AST)、类型检查结果
- **实验六**：四元式、三地址码、代码生成结果

### 运行前端

```bash
# 方法一：直接打开
open index.html

# 方法二：使用 Python HTTP 服务器
python3 -m http.server 8080

# 浏览器访问
http://localhost:8080
```

---

## 实验对比总览

| 实验 | 名称 | 输入 | 输出 | 核心算法 |
|------|------|------|------|----------|
| 1 | DFA模拟器 | DFA五元组 | 接受字符串集合 | BFS枚举、状态转移 |
| 2 | 词法分析器 | 源代码 | Token流 | DFA状态机 |
| 3 | LR(0)项目集 | 文法规则 | 项目集、GOTO图 | 闭包、Goto函数 |
| 4 | SLR(1)分析 | 文法规则 | 分析表、语法树 | FIRST/FOLLOW、SLR表 |
| 5 | 语义分析 | 源代码 | 符号表、AST | 作用域、类型检查 |
| 6 | 中间代码生成 | 源代码 | 四元式、三地址码 | 语法制导翻译 |

---

## Git 仓库

### 克隆仓库

```bash
git clone https://github.com/LeeLemon203/compiler-lab.git
cd compiler-lab
```

### 查看文件结构

```bash
tree
```

### 提交更新

```bash
git add .
git commit -m "更新说明"
git push
```

---

## 环境要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Linux / macOS / Windows (WSL) |
| C/C++ 编译器 | GCC / G++ 4.8+ |
| Python (前端) | Python 3.x (可选) |
| 浏览器 | Chrome / Firefox / Edge 最新版 |

---

