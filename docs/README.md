# 项目文档索引

本文档索引按 doc-saving 规则组织，所有技术文档均存放于此。

## 文档目录结构

```
docs/
├── architecture/          # 架构与模块文档
│   ├── project-requirements.md    # 项目需求与技术方案
│   └── time-system-overview.md    # 时间处理系统架构概述
│
├── code-explain/          # 代码讲解文档
│   └── files/
│       ├── timepoint-class.md     # TimePoint 类详解
│       └── timeparser-class.md    # TimeParser 类详解
│
├── refactor/              # 重构文档
│   ├── time-system-cleanup-v2.md  # 时间系统清理说明（版本 2.0）
│   ├── multimerge-checklist.md    # MultiMerge 项目检查清单
│   └── multimerge-org-summary.md  # MultiMerge 项目组织总结
│
├── decisions/             # 决策记录文档
│   └── data-merge-analysis.md     # 数据文件合并问题分析
│
└── contracts/             # 接口与约束文档
    └── multimerge-readme.md       # MultiMerge 项目使用说明
```

## 文档分类说明

### 1. 架构文档 (architecture/)

- **project-requirements.md** - 完整的项目需求与技术方案，包括功能需求、技术选型、架构设计
- **time-system-overview.md** - 时间处理系统整体架构，包括设计哲学、数据流、清理版说明

### 2. 代码讲解文档 (code-explain/)

- **timepoint-class.md** - TimePoint 类详细讲解，包括背景目的、核心逻辑、数据结构、潜在陷阱、工程实践建议
- **timeparser-class.md** - TimeParser 类详细讲解，包括单例模式、解析流程、错误处理、扩展性设计

### 3. 重构文档 (refactor/)

- **time-system-cleanup-v2.md** - 时间系统清理记录，详细说明了移除纳秒支持的代码变更、设计决策、兼容性说明
- **multimerge-checklist.md** - MultiMerge 项目组织完成检查清单（过程文档）
- **multimerge-org-summary.md** - MultiMerge 项目组织结构总结（过程文档）

### 4. 决策文档 (decisions/)

- **data-merge-analysis.md** - 数据文件合并需求分析，记录了核心问题、解决方案对比、技术选型理由

### 5. 接口文档 (contracts/)

- **multimerge-readme.md** - MultiMerge 项目使用说明，包括功能特性、编译要求、命令行参数、使用示例

## 文档更新记录

- **2026-03-16**: 首次按 doc-saving 规则整理所有文档，建立标准目录结构
- 所有文档从根目录和子目录迁移至 docs/ 下
- 文件名统一为小写 + 中划线格式

## 文档规范

所有文档遵循 doc-saving.md 规则：

- ✅ 背景与用途
- ✅ 核心逻辑分步讲解
- ✅ 关键数据结构/类关系
- ✅ 潜在坑点
- ✅ 工程实践建议
- ✅ 可维护性/性能/异常安全分析
- ✅ 一句话总结

## 快速查找

- 查找**架构设计** → `docs/architecture/`
- 查找**代码讲解** → `docs/code-explain/files/`
- 查找**重构记录** → `docs/refactor/`
- 查找**决策理由** → `docs/decisions/`
- 查找**使用说明** → `docs/contracts/`
- 查找**Bug 分析** → `docs/bugs/`
- 查找**性能优化** → `docs/perf/`
