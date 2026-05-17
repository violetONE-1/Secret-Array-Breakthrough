# 密阵突围 — C++ 游戏架构设计文档

---

## 1. 技术栈与构建方案 (Tech Stack & Build System)

### 1.1 GUI 框架推荐：SFML 2.6+ (Simple and Fast Multimedia Library)

**推荐理由：**

SFML 是当前最适合本项目需求的轻量级 C++ 多媒体库，原因如下：

| 考量维度 | 适配分析 |
|---------|---------|
| **2D 渲染能力** | 原生 `sf::RectangleShape`、`sf::Text`、`sf::Sprite` 可直接映射到 25×25 网格中的每个格子，绘制、高亮、动画均无需手写底层管线 |
| **事件系统** | `sf::Event` 统一封装键盘按键、鼠标点击/移动、窗口关闭等，配合 `sf::Mouse::getPosition()` 可轻松将像素坐标转换为网格坐标 |
| **轻量级** | 核心模块（graphics + window + system）合计 < 8 MB，编译快、链接简单，不引入 Qt 级别的 MOC 预处理和庞大依赖树 |
| **C++ 风格 API** | 面向对象设计（RAII 资源管理、移动语义），与 C++17/20 标准库自然融合，不会出现宏定义污染或全局状态 |
| **跨平台** | Windows / macOS / Linux API 完全一致，一套代码三平台编译 |
| **社区与文档** | 官方教程覆盖全部模块，社区活跃，中文教程丰富，适合教学场景 |

> **降级方案：** 若在学习过程中暂不引入第三方库，可先使用 **Windows 控制台 + `conio.h` / `windows.h` 光标定位 API** 实现纯文本版网格界面，满足"GUI 为加分项"的考核要求。SFML 在阶段 2 引入即可。

### 1.2 构建系统：CMake 3.20+

采用 CMake 作为跨平台构建系统，最小版本 3.20（以利用 `CXX_STANDARD` 等现代特性）。

**根 `CMakeLists.txt` 结构：**

```cmake
cmake_minimum_required(VERSION 3.20)
project(MatrixBreakthrough VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 可选项：是否启用 SFML GUI（默认 ON，初学可关）
option(ENABLE_GUI "Build with SFML graphical interface" ON)

# 头文件路径
target_include_directories(matrix_breakthrough PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 源文件（按模块分目录）
file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(matrix_breakthrough ${SOURCES})

if(ENABLE_GUI)
    find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
    target_link_libraries(matrix_breakthrough PRIVATE sfml-graphics sfml-window sfml-system)
    target_compile_definitions(matrix_breakthrough PRIVATE HAS_GUI)
endif()
```

**编译命令：**
```bash
# 终端版
cmake -B build -DENABLE_GUI=OFF && cmake --build build

# GUI 版
cmake -B build -DENABLE_GUI=ON  && cmake --build build
```

---

## 2. 架构模式 (Architecture Pattern)

### 2.1 选用：Model-View-Controller (MVC)

**判定依据：**

"密阵突围"是一款 **回合制、状态驱动型益智游戏**，每一帧的行为完全由"当前网格状态 + 用户操作"决定。MVC 的三层分离精确匹配这种场景：

| 对比维度 | MVC | ECS (Entity-Component-System) |
|---------|-----|------|
| 适用场景 | UI 密集型应用、回合制游戏 | 实时模拟（大量实体并发更新） |
| 实体数量 | 625 个固定格子（25×25），无动态创建/销毁 | 数千至上万动态实体 |
| 状态复杂度 | 每个格子仅 3~4 个字段（字母、数字、空/非空、选中） | 每个实体可挂载数十个 Component |
| 实现复杂度 | 低：三大层各司其职，学生可在 1000+ 行内完成 | 高：需要 EntityManager、ComponentPool、System 调度器 |
| **结论** | ✅ 推荐 | ❌ 过度设计 |

### 2.2 MVC 三层定义

```
┌──────────────────────────────────────────────────────────┐
│                     CONTROLLER 层                         │
│              GameController（主控制器）                     │
│              InputHandler（输入解析）                       │
│              AIPlayer（AI 对手）                           │
│                                                           │
│  职责：接收原始输入 → 转换为操作 → 调用 Model 更新         │
│       → 管理游戏阶段切换 → 驱动 View 重绘                  │
└──────┬────────────────────────────────────┬──────────────┘
       │ 写入 / 调用                        │ 只读查询
       ▼                                    ▼
┌─────────────────────┐        ┌──────────────────────────┐
│     MODEL 层         │        │        VIEW 层            │
│                      │        │                           │
│  Cell / Grid         │ ◄────  │  IRenderer（抽象接口）     │
│  MergeRule（静态）    │ 通知   │   ├── ConsoleRenderer    │
│  GameState / Move    │ 变化   │   └── SFMLRenderer       │
│  Puzzle / PuzzleMgr  │        │                           │
│  Player / ScoreRecord│        │  职责：接收 Model 只读引用 │
│  Leaderboard         │        │       → 绘制为可视界面     │
│  ReplayBuffer        │        │       → 从不修改 Model    │
│  FileManager         │        │                           │
│                      │        │                           │
│  职责：持有全部数据   │        └──────────────────────────┘
│       → 实现纯游戏逻辑│
│       → 不依赖 View  │
└─────────────────────┘
```

**核心约定（必须遵守）：**

1. View **绝不直接修改** Model 的任何成员
2. Controller **绝不直接调用** View 的任何绘制函数（仅通过 `IRenderer` 接口）
3. Model 层不 `#include` View 层或 Controller 层的任何头文件

---

## 3. 核心类设计与职责 (Core Classes & Responsibilities)

### 3.1 类总览

| # | 类名 | 所在层 | 文件 | 职责一句话 |
|---|------|-------|------|-----------|
| 1 | `Cell` | Model | `core/Cell.hpp` | 单个格子：字母、数字、空/非空、选中状态 |
| 2 | `Grid` | Model | `core/Grid.hpp` | 二维网格容器：创建、读写、查找合法操作、序列化 |
| 3 | `GameState` | Model | `core/GameState.hpp` | 一局游戏的瞬时快照：盘面、步数、计时、胜负 |
| 4 | `MergeRule` | Model | `core/MergeRule.hpp` | 静态工具类：合并合法性校验、字母/数字相加算法 |
| 5 | `Move` | Model | `core/Move.hpp` | 单步操作记录：源坐标、目标坐标、方向、合并结果 |
| 6 | `Puzzle` | Model | `model/Puzzle.hpp` | 一道题面：ID、名称、初始网格、可用起点 |
| 7 | `PuzzleManager` | Model | `model/PuzzleManager.hpp` | 题库管理：加载预置题面、保存自定义题面、按 ID 检索 |
| 8 | `Player` | Model | `model/Player.hpp` | 玩家信息：姓名、选中的 5 个起始格子 |
| 9 | `ScoreRecord` | Model | `model/ScoreRecord.hpp` | 单次答题记录：用时、步数、正确率、综合得分 |
| 10 | `Leaderboard` | Model | `model/Leaderboard.hpp` | 排行榜：多维度排序、从文件加载、追加与保存 |
| 11 | `ReplayBuffer` | Model | `model/ReplayBuffer.hpp` | 回放缓存：录制 Move 队列、导出文本文件、从文件加载重放 |
| 12 | `FileManager` | Model | `model/FileManager.hpp` | 文件 I/O 工具：读写文本、检查存在、创建目录 |
| 13 | `GameController` | Controller | `controller/GameController.hpp` | 主控制器：游戏主循环、流程编排、模块调度中枢 |
| 14 | `InputHandler` | Controller | `controller/InputHandler.hpp` | 输入解析：鼠标坐标→网格行列、键盘方向键→操作意图 |
| 15 | `AIPlayer` | Controller | `controller/AIPlayer.hpp` | AI 对手：遍历合法操作、评估盘面、选择策略（Lv.2+） |
| 16 | `IRenderer` | View | `view/IRenderer.hpp` | 渲染器抽象接口（纯虚类） |
| 17 | `ConsoleRenderer` | View | `view/ConsoleRenderer.hpp` | 终端渲染：ASCII 网格、控制台光标控制、颜色码 |
| 18 | `SFMLRenderer` | View | `view/SFMLRenderer.hpp` | GUI 渲染：SFML 窗口、鼠标交互、动画帧 (Lv.2+) |

### 3.2 各类职责详解

---

#### 3.2.1 Cell — 格子类

**职责：** 网格中最小的数据单元，存储一个格子的字母、数字及显示状态。

**关键数据成员：**

| 成员 | 类型 | 说明 |
|------|------|------|
| `letter` | `char` | 字母，取值范围 'A'~'Z' |
| `number` | `int` | 数字，取值范围 0~9 |
| `empty` | `bool` | 该格是否已被合并清空 |
| `selected` | `bool` | 是否被玩家当前选中 |

**关键成员函数：**

| 函数 | 说明 |
|------|------|
| `Cell(char letter, int number)` | 构造有内容的格子 |
| `static Cell emptyCell()` | 静态工厂，返回空格子 |
| `sameLetter(Cell& other) → bool` | 比较字母是否相同 |
| `sameNumber(Cell& other) → bool` | 比较数字是否相同 |
| `toString() → std::string` | 转为 "A1"、" . " 等显示字符串 |

---

#### 3.2.2 Grid — 网格类

**职责：** 管理 rows×cols 的二维 Cell 数组，提供网格级操作。

**关键数据成员：**

| 成员 | 类型 | 说明 |
|------|------|------|
| `rows` | `int` | 行数，默认 25 |
| `cols` | `int` | 列数，默认 25 |
| `cells` | `std::vector<std::vector<Cell>>` | 二维格子数组 |

**关键成员函数：**

| 函数 | 说明 |
|------|------|
| `Grid(int rows, int cols)` | 构造指定尺寸的空网格 |
| `at(r, c) → Cell&` | 访问指定坐标的格子（带边界检查） |
| `setAt(r, c, Cell)` | 设置指定坐标的格子 |
| `mergeCells(sr, sc, dr, dc)` | 合并两格：目标格更新为结果，源格置空 |
| `findAllValidMoves() → std::vector<Move>` | 扫描全盘所有合法合并操作 |
| `hasAnyValidMove() → bool` | 判断盘面是否已死局（无路可走） |
| `serialize() → std::string` | 序列化为题面文件格式 |
| `static Grid deserialize(const std::string&)` | 从字符串恢复网格 |

---

#### 3.2.3 GameState — 游戏状态类

**职责：** 聚合一局游戏的全部运行时状态，是 Controller 查询 Model 的统一入口。

**关键数据成员：**

| 成员 | 类型 | 说明 |
|------|------|------|
| `grid` | `Grid` | 当前盘面 |
| `puzzleId` | `std::string` | 当前题面 ID |
| `playerStarts` | `std::vector<Position>` | 玩家选择的 5 个起始点坐标 |
| `stepsTaken` | `int` | 已执行步数 |
| `startTime` | `std::chrono::steady_clock::time_point` | 首次操作的时间戳 |
| `moveHistory` | `std::vector<Move>` | 操作历史列表 |
| `gameOver` | `bool` | 游戏是否已结束 |

**关键成员函数：**

| 函数 | 说明 |
|------|------|
| `recordMove(Move)` | 记录一步操作，增加步数计数器 |
| `elapsedSeconds() → double` | 自首次操作以来的已用秒数 |
| `calculateAccuracy() → double` | 正确步数 / 总步数 |
| `checkGameOver() → bool` | 判断是否无合法操作或主动提交 |

---

#### 3.2.4 MergeRule — 合并规则类（纯静态）

**职责：** 封装"密阵突围"的全部合并规则算法，不持有任何状态。

**关键成员函数：**

| 函数 | 说明 |
|------|------|
| `static canMerge(Cell& a, Cell& b) → bool` | 判断 a 与 b 是否可合并（相邻 + 字母或数字有一项相同且另一项不同） |
| `static mergeLetters(char a, char b) → char` | 合并字母：A+B→C, Z+A→A（循环进位，超过 Z 折返 A） |
| `static mergeNumbers(int a, int b) → int` | 合并数字：1+2→3, 9+8→7（取个位） |
| `static getMergedCell(Cell& a, Cell& b) → Cell` | 返回合并后的新 Cell（字母不同则相加，数字不同则相加） |

**核心规则伪代码：**
```
canMerge(a, b):
    if a 为空 or b 为空 → false
    if a.letter == b.letter and a.number != b.number → true
    if a.letter != b.letter and a.number == b.number → true
    if a.letter == b.letter and a.number == b.number → false (合并后不变, 无效操作)
    return false
```

---

#### 3.2.5 Move — 操作记录类

**职责：** 记录单步合并操作的完整信息，供回放和正确率统计使用。

**关键数据成员：**

| 成员 | 类型 | 说明 |
|------|------|------|
| `srcRow`, `srcCol` | `int` | 被移动格子（邻格）的坐标 |
| `dstRow`, `dstCol` | `int` | 中心格子坐标 |
| `direction` | `Direction` | 合并方向 (UP/DOWN/LEFT/RIGHT) |
| `resultLetter` | `char` | 合并后的字母 |
| `resultNumber` | `int` | 合并后的数字 |

---

#### 3.2.6 GameController — 主控制器

**职责：** 游戏的中枢，持有全部 Model 和 View 对象，编排整个游戏生命周期。

**关键数据成员：**

| 成员 | 类型 | 说明 |
|------|------|------|
| `gameState` | `std::unique_ptr<GameState>` | 当前游戏状态 |
| `puzzleManager` | `std::unique_ptr<PuzzleManager>` | 题库管理 |
| `leaderboard` | `std::unique_ptr<Leaderboard>` | 排行榜 |
| `replayBuffer` | `std::unique_ptr<ReplayBuffer>` | 回放录制 |
| `renderer` | `std::unique_ptr<IRenderer>` | 渲染器（多态） |
| `inputHandler` | `std::unique_ptr<InputHandler>` | 输入处理 |
| `currentPlayer` | `Player` | 当前玩家 |
| `phase` | `GamePhase` | 游戏阶段枚举 |
| `running` | `bool` | 主循环是否继续 |

**关键成员函数：**

| 函数 | 说明 |
|------|------|
| `run()` | 启动游戏主循环：事件处理→更新→绘制 |
| `selectPuzzle(string id)` | 从题库加载题面并初始化 GameState |
| `executeMerge(sr, sc, dr, dc)` | 校验→合并→记录Move→检查胜负 |
| `submitAnswer()` | 结算成绩→更新排行榜→显示结果 |
| `loadReplay(string path)` | 加载回放文件并进入回放模式 (Lv.2+) |

---

### 3.3 类间交互关系

```
                           GameController
                          (主控制器 / 中枢)
                    ┌──────────┼──────────┐
                    │ 拥有     │ 拥有     │ 拥有
                    ▼          ▼          ▼
           ┌──────────┐ ┌──────────┐ ┌──────────┐
           │GameState │ │PuzzleMgr │ │Leaderboard│
           └────┬─────┘ └────┬─────┘ └────┬─────┘
                │ 包含        │ 包含        │ 包含
                ▼             ▼             ▼
           ┌──────────┐ ┌──────────┐ ┌────────────┐
           │  Grid    │ │  Puzzle  │ │ScoreRecord │
           └────┬─────┘ │  (题库)   │ │  (记录列表) │
                │ 包含   └──────────┘ └────────────┘
                ▼
           ┌──────────┐
           │  Cell    │ ◄── MergeRule::canMerge()
           │ (×625)   │     (静态调用, 无所有权)
           └──────────┘

    GameController ──► 持有 unique_ptr ──► IRenderer
                                              ▲
                                     ┌────────┴────────┐
                                     │                  │
                              ConsoleRenderer    SFMLRenderer
                              (终端 ASCII)       (SFML GUI)

    GameController ──► ReplayBuffer ──► 包含 vector<Move>
    GameController ──► InputHandler  (无状态, 坐标转换)
    GameController  ...静态调用...► MergeRule::canMerge()
```

**所有权总结：**

| 关系类型 | 示例 | 说明 |
|---------|------|------|
| **组合（值语义）** | `Grid` 包含 `vector<Cell>` | Grid 销毁时 Cell 自动销毁 |
| **独占所有（unique_ptr）** | `GameController` 持有 `GameState` | Controller 析构时自动释放全部子对象 |
| **静态调用** | `Controller` → `MergeRule::canMerge()` | 无所有权，纯算法函数 |
| **多态引用** | `Controller` 持有 `IRenderer*`（通过 `unique_ptr<IRenderer>`） | 运行时绑定到 Console 或 SFML 实现 |
| **引用传递** | `IRenderer::render(const GameState&)` | View 只读 Model，不拥有 |

---

## 4. 目录与文件结构规划 (Directory & File Structure)

```
E:\c++实训\
│
├── CMakeLists.txt                     # 根构建文件
├── game_rules.markdown                # 游戏规则文档
├── Architecture_Design.md             # 本架构设计文档
│
├── include/                           # 【头文件 - 全部 .hpp】
│   ├── core/                          # 核心游戏逻辑（无外部依赖）
│   │   ├── Cell.hpp                   #   格子类
│   │   ├── Grid.hpp                   #   网格类
│   │   ├── GameState.hpp              #   游戏状态类
│   │   ├── Move.hpp                   #   操作记录 + Direction 枚举
│   │   └── MergeRule.hpp              #   合并规则（纯静态）
│   │
│   ├── model/                         # 数据模型（依赖 core/）
│   │   ├── Player.hpp                 #   玩家类
│   │   ├── Puzzle.hpp                 #   题面类
│   │   ├── PuzzleManager.hpp          #   题库管理类
│   │   ├── ScoreRecord.hpp            #   答题记录类
│   │   ├── Leaderboard.hpp            #   排行榜类
│   │   ├── ReplayBuffer.hpp           #   回放缓冲类
│   │   └── FileManager.hpp            #   文件 I/O 工具类
│   │
│   ├── controller/                    # 控制器层（依赖 core/ + model/）
│   │   ├── GameController.hpp         #   主控制器
│   │   ├── InputHandler.hpp           #   输入解析器
│   │   └── AIPlayer.hpp               #   AI 对手 (Lv.2+)
│   │
│   └── view/                          # 视图层（依赖 core/，不依赖 controller/）
│       ├── IRenderer.hpp              #   渲染器抽象接口
│       ├── ConsoleRenderer.hpp        #   终端渲染器
│       └── SFMLRenderer.hpp           #   SFML 图形渲染器 (Lv.2+)
│
├── src/                               # 【源文件 - 全部 .cpp】
│   ├── main.cpp                       #   程序入口
│   ├── core/
│   │   ├── Cell.cpp
│   │   ├── Grid.cpp
│   │   ├── GameState.cpp
│   │   ├── Move.cpp
│   │   └── MergeRule.cpp
│   ├── model/
│   │   ├── Player.cpp
│   │   ├── Puzzle.cpp
│   │   ├── PuzzleManager.cpp
│   │   ├── ScoreRecord.cpp
│   │   ├── Leaderboard.cpp
│   │   ├── ReplayBuffer.cpp
│   │   └── FileManager.cpp
│   ├── controller/
│   │   ├── GameController.cpp
│   │   ├── InputHandler.cpp
│   │   └── AIPlayer.cpp
│   └── view/
│       ├── ConsoleRenderer.cpp
│       └── SFMLRenderer.cpp
│
├── assets/                            # 【外部资源文件（不参与编译）】
│   ├── fonts/
│   │   └── NotoSansSC-Regular.ttf     #   中文字体（SFML 渲染用）
│   └── sounds/                        #   (Lv.3) 音效文件
│
└── data/                              # 【运行时数据（读写）】
    ├── puzzles/                       #   题库目录
    │   ├── builtin_01.txt             #     预置题面 1
    │   ├── builtin_02.txt             #     预置题面 2
    │   ├── builtin_03.txt             #     预置题面 3
    │   └── custom/                    #     用户自建题面目录
    ├── leaderboard.txt                #   排行榜数据
    └── replays/                       #   回放文件目录
        └── *.txt                      #     按时间戳命名的回放文件
```

**命名与组织原则：**

- 头文件 `.hpp`，源文件 `.cpp`，一个类对应两个文件
- `include/` 与 `src/` 中的子目录结构完全镜像
- `core/` 子目录不依赖 `model/`、`controller/`、`view/` — 保证核心逻辑可独立编译和测试
- `data/` 和 `assets/` 不参与编译，由程序运行时按需读写

---

## 5. 内存与资源管理策略 (Memory & Resource Management Strategy)

### 5.1 总体原则：RAII + 智能指针

```
优先级链:  栈对象 / 值语义  >  unique_ptr  >  (不使用 shared_ptr / 裸 new)
```

**本游戏无任何场景需要 `shared_ptr`**：不存在多处同时拥有同一对象的需求。

### 5.2 各类场景的具体策略

| 场景 | 使用方式 | 理由 |
|------|---------|------|
| **网格存储（625 个 Cell）** | `std::vector<std::vector<Cell>>` — 栈上值语义 | Cell 为 4 个字段的小对象（~12 bytes），连续内存缓存友好，`vector` 自动管理生命周期 |
| **操作历史（Move 列表）** | `std::vector<Move>` — 值语义 | Move 是轻量 struct，直接存储避免指针开销 |
| **GameController 的子对象** | `std::unique_ptr<GameState>`、`std::unique_ptr<IRenderer>` 等 | 独占所有权：Controller 是唯一拥有者，析构时自动级联释放 |
| **题库中的题面列表** | `std::vector<Puzzle>` — 值语义 | Puzzle 对象在 Manager 内部整体存活，无共享需求 |
| **排行榜记录** | `std::vector<ScoreRecord>` — 值语义 | 同上 |
| **回放缓冲区** | `std::vector<Move>` — 值语义 | 直接存储操作序列 |
| **SFML 资源（Font、Texture）** | SFML 内部使用 RAII + 引用计数 | `sf::Font` 和 `sf::Texture` 自身实现析构释放，在其所属的 `SFMLRenderer` 对象中作为成员变量，随 Renderer 一起销毁 |
| **文件流** | `std::ifstream` / `std::ofstream` — 局部变量 | 在函数内创建，离开作用域自动关闭文件 |
| **AI 搜索树（Lv.3 高级 AI）** | `std::unique_ptr<SearchNode>` 构建子节点 | 递归析构，避免手动 `delete` |
| **字符串** | `std::string` — 值语义 | 不使用 `char*`，避免缓冲区溢出 |

### 5.3 外部资源（Assets）管理

| 资源类型 | 加载方式 | 生命周期 | 异常处理 |
|---------|---------|---------|---------|
| **字体文件** (.ttf) | `SFMLRenderer` 构造时通过 `sf::Font::loadFromFile()` 加载 | 与 `SFMLRenderer` 对象同生命周期 | 加载失败时回退到控制台模式或使用 SFML 内置默认字体 |
| **题面文件** (.txt) | `FileManager::readTextFile()` 返回 `std::string`，由 `Puzzle::deserialize()` 解析 | 解析后数据存入值对象，原始字符串即释放 | 文件不存在或格式错误时给出明确提示并返回主菜单 |
| **排行榜/回放** | 按需通过 `FileManager` 读写 | 瞬时操作，不常驻内存 | 写失败时提示用户（如磁盘已满） |

### 5.4 严禁事项

- ❌ 不使用 `new` / `delete` 裸指针管理堆内存
- ❌ 不使用 `malloc` / `free`
- ❌ 不使用 C 风格数组（`Cell arr[625]`）— 用 `std::vector` 或 `std::array`
- ❌ 不使用 `char*` 或 `char[]` 处理字符串 — 用 `std::string`

---

## 6. 分步实现计划 (Step-by-Step Implementation Plan)

### 阶段 1：核心数据结构 + 控制台最小可玩原型

> **时间预估：** 2~3 天 　　 **优先级：** ⭐⭐⭐ 必做  
> **目标：** 纯控制台下可完整游玩一局，验证合并规则正确。

**涉及类（6 个核心 + 2 个辅助）：**

```
Cell, Grid, MergeRule, Move, GameState
GameController (简化版), ConsoleRenderer, main.cpp
```

**交付物清单：**

- [x] `Cell` / `Grid` / `MergeRule` 完成并通过手动测试
- [x] 25×25 网格的控制台 ASCII 绘制（`ConsoleRenderer`）
- [x] 使用键盘方向键或坐标输入选择格子、执行合并
- [x] 合并规则正确：同字母不同数字 → 数字相加取个位；同数字不同字母 → 字母相加循环
- [x] 至少 1 道预置题面硬编码在代码中
- [x] 玩家选择 5 个起始点
- [x] 盘面更新与胜负判断

**此阶段完成后即可验证核心玩法，"1000+ 行代码"的骨架已经搭好。**

---

### 阶段 2：题库系统 + 排行榜 + 文件持久化

> **时间预估：** 1~2 天 　　 **优先级：** ⭐⭐⭐ 必做  
> **目标：** 满足 Level 1 全部要求，程序具备完整的数据存取能力。

**新增类（6 个）：**

```
Puzzle, PuzzleManager, ScoreRecord, Leaderboard, FileManager, Player
```

**交付物清单：**

- [x] 3 道以上预置题面（存储为 `data/puzzles/builtin_*.txt`）
- [x] 用户选题界面（主菜单 → 题面列表 → 确认）
- [x] 计时器（从首次操作开始计时）
- [x] 正确率、步数、综合得分自动计算
- [x] 提交答案后记录写入 `data/leaderboard.txt`
- [x] 排行榜查看功能（按总分降序显示）
- [x] 程序启动时自动加载题库和排行榜

---

### 阶段 3：SFML 图形化界面

> **时间预估：** 2~3 天 　　 **优先级：** ⭐⭐ 加分项  
> **目标：** 用 SFML 窗口替代控制台，实现更友好的视觉交互。

**新增类（2 个）：**

```
SFMLRenderer (实现 IRenderer), InputHandler (鼠标坐标转换)
```

**交付物清单：**

- [x] SFML 窗口正确显示 25×25 网格（格子用彩色矩形 + 白色文字）
- [x] 鼠标点击选中格子（高亮边框）
- [x] 可合并的邻格自动高亮提示
- [x] 方向键或点击邻格执行合并
- [x] 简易合并动画（格子颜色过渡 / 缩放）
- [x] 排行榜面板、选题界面图形化
- [x] 保留控制台模式（编译选项切换）

---

### 阶段 4：Level 2 扩展 — 回放 + 自定义题面 + 简单 AI

> **时间预估：** 2~3 天 　　 **优先级：** ⭐⭐ 选做  
> **目标：** 覆盖 Level 2 全部要求。

**新增 / 扩展类（4 个）：**

```
ReplayBuffer（新增）, AIPlayer（新增）
PuzzleManager（扩展：支持创建和保存）, Leaderboard（扩展：多维度排序）
```

**交付物清单：**

- [x] 每局操作自动录制到 `data/replays/`
- [x] 回放模式：加载回放文件，按固定间隔（如 500ms）逐步重现
- [x] 用户可设置盘面大小（10×10 ~ 30×30）
- [x] 用户可手动输入初始布局并保存为新题面
- [x] 简单 AI 对手（扫描全盘，随机选择合法操作执行）
- [x] 排行榜支持"单题最快""累计最高分""最高正确率"三种排序

---

### 阶段 5：Level 3 增强 — 高级 AI + 动画音效 + 闯关模式

> **时间预估：** 2~3 天 　　 **优先级：** ⭐ 选做加分  
> **目标：** 打磨游戏体验，深化 AI 策略。

**扩展内容：**

```
AIPlayer 增强（极小化极大搜索 / BFS 预判）
SFMLRenderer 增强（过渡动画、粒子效果）
闯关模式（递进难度）
```

**交付物清单：**

- [x] 高级 AI：预判 2~3 步，优先消灭大片孤立区域
- [x] 合并 / 错误操作均有过渡动画（平滑移动 + 颜色渐变）
- [x] 背景音效（SFML Audio 模块）
- [x] 闯关模式：关卡 1（20×20）→ 关卡 2（25×25）→ 关卡 3（30×30 + 步数限制）
- [x] 提示系统：高亮当前所有可合并位置

---

### 各阶段优先级与依赖关系

```
阶段 1 ─────────────────────────────────────────► (核心验证)
  │
  ├──► 阶段 2 ──────────────────────────────────► (Level 1 完成)
  │      │
  │      ├──► 阶段 3 ───────────────────────────► (GUI 加分)
  │      │      │
  │      │      └──► 阶段 5 ────────────────────► (Level 3 加分)
  │      │
  │      └──► 阶段 4 ───────────────────────────► (Level 2 选做)
  │             │
  │             └──► 阶段 5 ────────────────────► (Level 3 加分)
```

> **建议：** 阶段 1 → 阶段 2 为必做主线，完成后可根据剩余时间和兴趣选择性推进阶段 3 或 4。

---

### 下一步行动

**立即开始阶段 1**：创建 `Cell.hpp` / `Cell.cpp`、`Grid.hpp` / `Grid.cpp`、`MergeRule.hpp` / `MergeRule.cpp`，并用一个简单的 `main.cpp` 验证合并逻辑的正确性。这三个类是整个游戏的基石，后续所有功能均建立在此之上。
