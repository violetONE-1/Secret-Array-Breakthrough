
# 项目开发报告：密阵突围 (Matrix Breakthrough)

> 基于《最强大脑》第 13 季同名赛题，在 25×25 网格上进行字母+数字格子合并的益智游戏。
> 架构：MVC (Model-View-Controller) ｜ 语言：C++17 ｜ 构建：CMake 3.20+
> 当前阶段：已完成阶段 1~3 核心内容，阶段 4 部分完成。

---

## 一、项目概览

### 完整文件结构

```
E:\c++实训\
├── CMakeLists.txt                         # CMake 构建配置
├── Architecture_Design.md                 # 架构设计文档
├── report1.md / report2.md               # 阶段工作报告
│
├── include/                               # 【头文件集合 (.hpp)】
│   ├── core/                              # 核心游戏逻辑（无外部依赖）
│   │   ├── Cell.hpp                       #   格子类
│   │   ├── Grid.hpp                       #   网格类
│   │   ├── MergeRule.hpp                  #   合并规则（纯静态）
│   │   ├── Move.hpp                       #   操作记录 + Direction 枚举
│   │   └── GameState.hpp                  #   游戏状态
│   │
│   ├── model/                             # 数据模型（依赖 core/）
│   │   ├── FileManager.hpp                #   文件 I/O 工具
│   │   ├── Player.hpp                     #   玩家信息
│   │   ├── Puzzle.hpp                     #   题面数据
│   │   ├── PuzzleManager.hpp              #   题库管理
│   │   ├── ScoreRecord.hpp                #   答题记录
│   │   ├── Leaderboard.hpp                #   排行榜
│   │   └── ReplayBuffer.hpp               #   回放录制/加载
│   │
│   ├── controller/                        # 控制器层
│   │   ├── GameController.hpp             #   主控制器
│   │   ├── InputHandler.hpp               #   输入解析
│   │   └── AIPlayer.hpp                   #   AI 对手
│   │
│   └── view/                              # 视图层
│       ├── IRenderer.hpp                  #   渲染器抽象接口
│       ├── ConsoleRenderer.hpp            #   终端 ASCII 渲染
│       └── SFMLRenderer.hpp               #   SFML 图形渲染
│
├── src/                                   # 【源文件集合 (.cpp)】
│   ├── main.cpp                           # 程序入口
│   ├── core/   (Cell, Grid, MergeRule, Move, GameState)
│   ├── model/  (FileManager, Player, Puzzle, PuzzleManager,
│   │            ScoreRecord, Leaderboard, ReplayBuffer)
│   ├── controller/ (GameController, InputHandler, AIPlayer)
│   └── view/   (ConsoleRenderer, SFMLRenderer)
│
├── data/                                  # 【运行时数据】
│   ├── puzzles/
│   │   ├── builtin_01.txt                 # Beginner Training (25×25)
│   │   ├── builtin_02.txt                 # Advanced Challenge (25×25)
│   │   ├── builtin_03.txt                 # Matrix Master (25×25)
│   │   └── custom/                        # 用户自定义题面目录
│   ├── replays/                           # 对局回放文件
│   │   └── replay_*.txt                   #   按时间戳命名
│   └── leaderboard.txt                    # 排行榜 CSV 数据
```

**代码规模：** 约 3200 行（含头文件声明 + 源文件实现），共 18 个类。

---

## 二、已实现核心功能

### 2.1 核心游戏机制（Model 层）

#### 格子 Cell

网格中最小的数据单元，存储字母 + 数字 + 状态位。

| 功能 | 说明 |
|------|------|
| 数据存储 | `char _letter` (A~Z), `int _number` (0~9) |
| 状态管理 | `_empty` 是否被清空, `_selected` 是否被选中 |
| 比较接口 | `sameLetter()` / `sameNumber()` — 空格子自动屏蔽 |
| 格式化 | `toString()` → 对齐到 3 字符宽度的显示串 |

**实现位置：** `include/core/Cell.hpp` + `src/core/Cell.cpp`

---

#### 合并规则 MergeRule

纯静态工具类，封装全部合并判定算法。

```
合并条件（两格相邻，满足任一）：
  ① 字母相同 & 数字不同 → 合并后字母不变，数字相加取个位
     例：A1 + A2 → A3,  A9 + A8 → A7
  ② 数字相同 & 字母不同 → 合并后数字不变，字母循环相加 (A+Z→A)
     例：A1 + B1 → C1,  Z1 + A1 → A1
```

| 函数 | 说明 |
|------|------|
| `canMerge(a, b)` | 判断两格是否可合并 |
| `mergeLetters(a, b)` | 字母相加 (A+B→C, 超 Z 折返 A) |
| `mergeNumbers(a, b)` | 数字相加取个位 |
| `getMergedCell(a, b)` | 返回合并后的新 Cell |

**实现位置：** `include/core/MergeRule.hpp` + `src/core/MergeRule.cpp`

---

#### 网格 Grid

管理 rows×cols 二维 Cell 数组，提供网格级操作。

| 功能 | 说明 |
|------|------|
| 格子访问 | `at(r, c)` — const/非 const 双版，带边界检查 |
| 合并操作 | `mergeCells(sr,sc, dr,dc, result)` — 源格置空，目标格更新 |
| 滑行操作 | `slideCell(sr,sc, dr,dc)` — 棋子移入空格 |
| 合法扫描 | `findAllValidMoves()` — 遍历全盘找所有合法合并 |
| 死局判断 | `hasAnyValidMove()` — 检查盘面是否无路可走 |
| 序列化 | `serialize()` / `deserialize()` — 文本格式读写 |

**实现位置：** `include/core/Grid.hpp` + `src/core/Grid.cpp`

---

#### 操作记录 Move + Direction

记录单步操作的完整信息，支持回放序列化。

```
示例：[Move] 001 3,4 -> 3,5 RIGHT A5
```

字段：`moveType` (MERGE/SLIDE)、源坐标、目标坐标、方向、合并结果。

**实现位置：** `include/core/Move.hpp` + `src/core/Move.cpp`

---

#### 游戏状态 GameState

一局游戏的运行时快照。

| 功能 | 说明 |
|------|------|
| 盘面持有 | 包含 Grid 引用 |
| 有效棋子 | `activeCells()` — 起始 5 个棋子，合并后自动追踪新位置 |
| 步数统计 | `stepsTaken()` / `slideCount()` |
| 计时器 | 首次操作自动启动，`elapsedSeconds()` 返回已过秒数 |
| 死局检测 | `isDeadEnd()` — 5 个棋子均无合法邻居或空格可移动 |
| 操作历史 | `moveHistory()` — 完整 Move 列表 |

**实现位置：** `include/core/GameState.hpp` + `src/core/GameState.cpp`

---

### 2.2 数据持久化（Model 层）

#### FileManager — 文件 I/O

| 功能 | 说明 |
|------|------|
| `readAll(path)` | 读取文本文件全部内容 |
| `writeAll(path, content)` | 覆盖写入（自动创建父目录） |
| `appendLine(path, line)` | 追加一行到文件末尾 |
| `exists(path)` / `createDir(path)` | 文件存在性检查 / 递归创建目录 |

所有路径相对于 `data/`。使用 C++17 `<filesystem>` 实现跨平台。

---

#### Puzzle + PuzzleManager — 题面系统

**题面格式：**
```
# builtin_01 Beginner Training
25 25
X1 Q1 P9 I4 B9 ...     ← 25×25 格子数据
5
0,0 3,3 6,10 12,15 20,20   ← 推荐起点
```

**内置题面（3 道 25×25）：**

| ID | 名称 |
|-----|------|
| `builtin_01` | Beginner Training |
| `builtin_02` | Advanced Challenge |
| `builtin_03` | Matrix Master |

- 固定随机种子 (`srand(42)`) 保证每次运行生成相同题面
- 每道题面内置 10 个强制可合并对（5 对同字母 + 5 对同数字）确保可玩性
- 首次运行时自动生成并写入 `data/puzzles/`，之后从文件加载
- `PuzzleManager::addPuzzle()` 支持添加自定义题面

---

#### ScoreRecord + Leaderboard — 成绩与排行榜

**得分公式：**
```
总分 = accuracy × 1000 + max(0, 500 − time×2) + steps × 10
     ↑ 正确率分        ↑ 时间奖励              ↑ 步数分
```

**排行榜功能：**
- `topByScore(n)` — 按总分降序
- `topByTime(n)` — 按用时升序
- `topByAccuracy(n)` — 按正确率降序
- `recordsForPuzzle(id)` — 按题面筛选
- 每次 `add()` 即时追加一行到 `data/leaderboard.txt`

---

#### ReplayBuffer — 回放系统

**每局自动录制**（从 `startNewGame` 到 `submitAnswer`），导出到 `data/replays/`。

**回放文件格式：**
```
[Puzzle] builtin_01
[Grid] 25 25
[Player] aa
[Starts] 0,0 24,0 24,24 13,14 5,24
[Start] 2026-05-23 13:21:10
[End] 2026-05-23 13:22:27
[TotalSteps] 3
[Move] 001 0,1 -> 0,0 LEFT N1
[Move] 002 18,3 -> 18,2 LEFT I8
[Move] 003 17,2 -> 16,2 UP C9
```

支持 `loadFromFile()` 加载回放用于复盘。

---

### 2.3 游戏流程（Controller 层）

#### 主控制器 GameController

**状态机流转：**
```
MENU ──→ PUZZLE_SELECT ──→ GAMEPLAY ──→ RESULT ──→ MENU
  │                                              ↑
  ├──→ LEADERBOARD ──────────────────────────────┘
  │
  └──→ VS_AI_MENU ──→ VS_AI_WATCH ──→ VS_AI_RESULT
```

**主循环 (`run()`)**：通过 `switch (_phase)` 分派各阶段处理函数。

| 阶段 | 功能 |
|------|------|
| MENU | 显示主菜单：开始游戏 / 排行榜 / VS AI / 退出 |
| PUZZLE_SELECT | 题面列表选择，输入玩家名 |
| GAMEPLAY | 方向键导航，空格/回车合并，S 提交，E 由 AI 代走一步 |
| RESULT | 展示成绩与操作历史 |
| LEADERBOARD | 显示排行榜（前 15 名） |
| VS_AI_MENU | AI 难度选择（普通/高级） |
| VS_AI_WATCH | AI 自动走棋（带延迟动画） |
| VS_AI_RESULT | 玩家 vs AI 成绩对比 |

**合并执行流程 (`tryMerge`)**：
1. 校验源格是否为有效棋子 (`isActiveCell`)
2. 校验目标格非空
3. `MergeRule::canMerge()` 判定可行性
4. `MergeRule::getMergedCell()` 计算结果
5. `grid.mergeCells()` 执行合并（源格置空，目标格更新）
6. `GameState::recordMove()` 记录操作
7. `ReplayBuffer::record()` 写入回放缓冲区
8. `GameState::updateActiveCells()` 追踪棋子新位置

**滑行操作 (`trySlide`)**：棋子可滑入相邻空格，同样记入回放。

---

#### AIPlayer — AI 对手

支持两种策略：

| 策略 | 说明 |
|------|------|
| **RANDOM（普通）** | 70% 概率选合并，30% 选滑行；随机选择目标 |
| **GREEDY（高级/贪心）** | 优先合并，选合并后字母+数字总值最大的操作；无合并时随机滑行 |

**AI 起始格选择：**
- 扫描全盘所有非空格
- 按可合并邻居数 × 10 + 非空格邻居数评分
- 取前 5 名，确保最小间距 ≥ 3 格
- 间距不足时放宽限制补齐

**VS AI 对战模式：**
1. 玩家选难度 → 选题 → 正常游玩
2. 提交后系统记录玩家成绩
3. AI 从同一盘面自选 5 起始格开始自动走棋
4. AI 每步带延迟动画（普通 300ms / 高级 500ms）
5. 对比玩家与 AI 成绩，判定胜负

---

### 2.4 视图层 (View)

#### IRenderer 抽象接口

定义全部渲染器共用的纯虚接口，包含 `render()`, `showMenu()`, `showPuzzleList()`, `showLeaderboard()`, `showResult()`, `showVSAIMenu()`, `showVSResult()`, `waitForAction()`, `promptPlayerName()`, `promptStartingCells()` 等。

**用户操作统一枚举 `UserAction`**：从键盘/鼠标抽象出 UP/DOWN/LEFT/RIGHT/CONFIRM/SELECT_CELL/SUBMIT/AI_EXECUTE 等操作。

---

#### ConsoleRenderer — 终端 ASCII 渲染

- 使用 `SetConsoleCursorPosition` 定位光标，`SetConsoleTextAttribute` 控制颜色
- `_getch()` 捕获方向键（224 前缀）、回车、空格、ESC 等
- 25×25 满网格显示，带行列标号
- 选中格亮黄色高亮，空格灰色 `.` 显示，正常格 "A1" 格式
- 信息栏：题面 ID、步数、计时实时更新
- 操作提示：方向键移动、空格/回车合并、S 提交、Q 返回、E AI 代走

---

#### SFMLRenderer — SFML 图形渲染（阶段 3）

通过 `#ifdef HAS_GUI` 条件编译切换。

| 功能 | 说明 |
|------|------|
| 彩色网格 | 白色格子 + 淡蓝背景，空格浅灰 |
| 光标高亮 | 浅蓝底色标记当前光标位置 |
| 选中高亮 | 浅黄底色 + 蓝色粗边框 |
| 有效棋子 | 金黄色粗边框（3px）突出 5 个棋子 |
| 可合并邻格提示 | 绿色边框标记选中格的可合并邻居 |
| 信息面板 | 右侧面板显示题面、步数、计时、棋子数、选中详情 |
| 菜单界面 | 按钮式主菜单 + 题面列表 + 排行榜 + 结果页 |
| 输入框 | 圆角输入框，文本实时显示，闪烁竖线光标 |
| 合并动画 | 合并后 300ms 黄色淡出闪烁 |
| 全屏切换 | F11 键在窗口/全屏间切换 |
| 窗口自适应 | 格子大小随窗口缩放动态调整 |
| 鼠标交互 | 点击选择/合并，菜单按钮点击 |
| 键盘交互 | 方向键移动，Space 两段式选中+合并，Enter 执行 |

---

## 三、架构亮点

### 3.1 MVC 严格分层

```
Controller (GameController)
  │ 调用 Model 写入接口
  │ 将 Model 只读引用传给 View
  ▼                          ▼
Model (Cell/Grid/GameState)   View (IRenderer → Console/SFML)
  └─ 纯游戏逻辑                  └─ 纯绘制逻辑
  └─ 不依赖 View/Controller       └─ 从不修改 Model
```

### 3.2 内存安全

- 所有堆对象通过 `std::unique_ptr` 独占管理
- 集合类使用 `std::vector` 值语义
- 不使用 `new`/`delete`/`malloc`/`free`/C 风格数组

### 3.3 两种渲染器可切换

通过 CMake 选项 `ENABLE_GUI=ON/OFF` 在编译时选择 SFML 图形界面或控制台终端界面。

### 3.4 文件 I/O 统一入口

所有文件操作通过 `FileManager` 类，路径相对于 `data/` 根目录，错误集中处理。

---

## 四、与架构文档对照

| 架构计划内容 | 状态 | 备注 |
|------------|------|------|
| **阶段 1：核心数据结构 + 控制台原型** | **已完成** | |
| Cell / Grid / MergeRule / Move / GameState | 已完成 | 含边界检查、序列化 |
| ConsoleRenderer（25×25 ASCII 网格） | 已完成 | 含颜色高亮、光标导航 |
| 合并规则正确实现 | 已完成 | 字母循环加法 + 数字个位加法 |
| 键盘控制（方向键 + 空格/回车） | 已完成 | 扩展了 WASD 支持 |
| 盘面更新与胜负判断 | 已完成 | 死局自动提交 |
| **阶段 2：题库系统 + 排行榜 + 文件持久化** | **已完成** | |
| FileManager 文件 I/O | 已完成 | C++17 filesystem |
| Puzzle / PuzzleManager 题库 | 已完成 | 3 道内置 + 自定义 |
| ScoreRecord 自动计分 | 已完成 | 正确率×1000 + 时间奖励 + 步数分 |
| Leaderboard 排行榜 | 已完成 | 多维度排序，即时持久化 |
| Player 玩家信息 | 已完成 | 轻量值对象 |
| 计时器（首次操作启动） | 已完成 | `std::chrono::steady_clock` |
| **阶段 3：SFML 图形化界面** | **已完成** | 编译开关 `ENABLE_GUI=ON` |
| 彩色网格 + 鼠标交互 | 已完成 | 点击选中，点击邻居合并 |
| 可合并邻格自动高亮 | 已完成 | 绿色边框标记 |
| 合并动画 | 已完成 | 300ms 黄色淡出 |
| 排行榜/选题界面图形化 | 已完成 | 按钮 + 列表 |
| 全屏切换 | 已完成 | F11 |
| **阶段 4：回放 + 自定义题面 + AI** | **部分完成** | |
| ReplayBuffer 回放录制/加载 | 已完成 | 自动录制，导出/加载 |
| AIPlayer（RANDOM + GREEDY） | 已完成 | 含自选起始格策略 |
| VS AI 对战模式 | 已完成 | 玩家 vs AI 成绩对比 |
| 滑行操作（棋子移入空格） | 已完成 | 合并无路时滑行 |
| Leaderboard 多维度排序 | 已完成 | 按总分/用时/正确率 |
| 用户可设置盘面大小 | ❌ 未实现 | |
| 用户手动输入布局创建题面 | ❌ 未实现 | `addPuzzle()` API 已存在但无界面 |
| **阶段 5：高级 AI + 动画音效 + 闯关** | **未开始** | |
| AI 极小化极大搜索 / BFS 预判 | ❌ 未实现 | |
| 过渡动画（平滑移动+渐变） | ❌ 未实现 | 仅简单的闪烁动画 |
| 音效 | ❌ 未实现 | |
| 闯关模式（递进难度） | ❌ 未实现 | |
| 提示系统 | ❌ 未实现 | |

---

## 五、下一步工作

根据架构文档的阶段规划，建议优先完成阶段 4 的剩余内容：

### 优先级 1 （高）：完善阶段 4

1. **盘面尺寸自定义**：允许用户在 10×10 ~ 30×30 范围内选择网格大小
2. **题面创建工具**：提供可视化界面或控制台向导，让用户手动输入初始布局并保存为新题面
3. **回放复盘模式**：加载 `data/replays/` 下的回放文件，按固定间隔逐步重现游戏过程

### 优先级 2 （中）：阶段 5 高级功能

4. **高级 AI**：实现极小化极大搜索（Minimax）或 BFS 预判 2~3 步，优先消灭大片孤立区域
5. **闯关模式**：关卡 1（20×20）→ 关卡 2（25×25）→ 关卡 3（30×30 + 步数限制）
6. **过渡动画**：SFML 渲染器中实现格子平滑移动和颜色渐变
7. **提示系统**：高亮当前所有可合并位置

### 优先级 3 （低）：打磨优化

8. SFML 音效（合并音效、背景音乐）
9. 自定义题面批量导入/导出
10. 网络排行榜（扩展至 Level 3 要求）

---

## 六、构建与运行

```bash
# 终端版（默认）
cmake -B build -DENABLE_GUI=OFF
cmake --build build
./build/matrix_breakthrough

# GUI 版（需安装 SFML 2.6+）
cmake -B build -DENABLE_GUI=ON
cmake --build build
./build/matrix_breakthrough
```

**操作说明（终端版）：**

| 按键 | 功能 |
|------|------|
| 方向键 / WASD | 移动光标 |
| 空格 | 选中/合并（两段式） |
| 回车 | 选中格尝试合并（优先），其次滑行 |
| S | 提交答案 |
| E | AI 代走一步 |
| Q | 返回菜单 |
| 1~4 | 菜单选项选择 |
| L | 查看排行榜 |

**操作说明（GUI 版）：**

| 操作 | 功能 |
|------|------|
| 方向键 / 鼠标点击 | 移动光标 |
| 点击格子 → 点击邻居 | 选中并合并 |
| 空格 | 两段式选中/取消/合并 |
| 回车 | 执行选中格的合并 |
| S | 提交答案 |
| E | AI 代走一步 |
| Q / Esc | 返回菜单 |
| F11 | 全屏切换 |
