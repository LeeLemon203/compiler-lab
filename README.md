# 编译原理实验平台

<div align="center">

![GitHub repo size](https://img.shields.io/github/repo-size/LeeLemon203/compiler-lab)
![GitHub language count](https://img.shields.io/github/languages/count/LeeLemon203/compiler-lab)
![GitHub last commit](https://img.shields.io/github/last-commit/LeeLemon203/compiler-lab)
![HTML5](https://img.shields.io/badge/HTML5-E34F26?logo=html5&logoColor=white)
![CSS3](https://img.shields.io/badge/CSS3-1572B6?logo=css3&logoColor=white)
![JavaScript](https://img.shields.io/badge/JavaScript-F7DF1E?logo=javascript&logoColor=black)

**编译器设计专题实验 - 实验三：LR(0) 项目集规范族构建与可视化**

[![Live Demo](https://img.shields.io/badge/Live%20Demo-GitHub%20Pages-blue)](https://LeeLemon203.github.io/compiler-lab/)
[![Report](https://img.shields.io/badge/Report-实验报告-green)](./docs/实验三报告.md)

</div>

---

## 📋 目录

- [项目简介](#项目简介)
- [功能特性](#功能特性)
- [技术栈](#技术栈)
- [项目结构](#项目结构)
- [快速开始](#快速开始)
- [使用指南](#使用指南)
- [文法格式说明](#文法格式说明)
- [核心算法](#核心算法)
- [运行结果示例](#运行结果示例)
- [冲突分析](#冲突分析)
- [实验总结](#实验总结)
- [Git 版本管理](#git-版本管理)
- [部署到 GitHub Pages](#部署到-github-pages)
- [常见问题](#常见问题)
- [后续工作](#后续工作)
- [许可证](#许可证)
- [联系方式](#联系方式)

---

## 项目简介

本项目是**编译器设计专题实验**的配套可视化平台，专注于**实验三：LR(0) 项目集规范族构建与可视化**。

### 实验背景

LR(0) 分析法是一种自底向上的语法分析方法，通过构建项目集规范族来识别文法的可归约句柄。本项目通过可视化方式，将晦涩难懂的算法原理转化为直观的图形界面，帮助理解 LR(0) 分析器的构造过程。

### 实验目标

1. ✅ 理解 LR(0) 项目集规范族的构建原理
2. ✅ 掌握增广文法的添加方法
3. ✅ 掌握闭包（Closure）和转移（Goto）的计算方法
4. ✅ 能够判断文法是否为 LR(0) 文法
5. ✅ 实现项目集规范族的可视化展示

---

## 功能特性

### 核心功能

| 功能 | 描述 | 状态 |
|------|------|------|
| 📝 文法输入 | 支持扩展巴科斯范式（EBNF）文法输入 | ✅ |
| 📁 文件上传 | 支持 .txt 文本文档上传，自动解析 | ✅ |
| 🔧 增广文法 | 自动添加 S' → S 增广文法 | ✅ |
| 📊 项目集生成 | 生成完整的 LR(0) 项目集规范族 | ✅ |
| 🔄 闭包计算 | 自动计算每个项目集的闭包 | ✅ |
| ➡️ Goto 计算 | 自动计算状态转移关系 | ✅ |
| 🎨 可视化图形 | 使用 vis-network 展示状态转移图 | ✅ |
| ⚠️ 冲突检测 | 检测移进-归约和归约-归约冲突 | ✅ |
| 💾 结果保存 | 支持结果保存为文本文件 | ✅ |

### 界面特性

- 🎨 **现代化 UI**：基于 Tailwind CSS 构建，界面明亮清晰
- 📱 **响应式设计**：支持桌面端和移动端访问
- 🖱️ **交互式图形**：支持拖拽、缩放状态转移图
- 📋 **侧边栏导航**：多页面切换，结构清晰
- 🌙 **代码高亮**：文法输入区支持等宽字体

---

## 技术栈

| 技术 | 用途 | 版本 |
|------|------|------|
| HTML5 | 页面结构 | - |
| Tailwind CSS | 样式框架 | CDN |
| JavaScript | 核心算法 | ES6+ |
| vis-network | 图形可视化 | 9.1.2 |
| Git | 版本控制 | - |
| GitHub Pages | 部署托管 | - |

### CDN 依赖

```html
<!-- Tailwind CSS -->
<script src="https://cdn.tailwindcss.com"></script>

<!-- vis-network 图形库 -->
<script src="https://unpkg.com/vis-network@9.1.2/dist/vis-network.min.js"></script>