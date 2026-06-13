# 密阵突围 (Matrix Breakthrough) — 课程设计报告

---

## 一、作品描述 (Artifact Description)

### 1.1 项目背景与定位

本项目是基于 C++17 开发的一款**益智策略对战游戏**，灵感来源于《最强大脑》第 13 季中的"密阵突围"赛题。游戏在一个 N×N（默认 25×25）的网格上进行，每个格子包含一个字母（A~Z）和一个数字（0~9）。玩家需要通过**合并 (Merge)** 和**滑行 (Slide)** 两种操作来消耗自己的棋子、攻占对手领地并获取分数。

该系统定位为**单机桌面游戏**，同时支持**单人闯关模式**和**人机对战模式**，满足不同层级的游戏需求。

### 1.2 核心功能概述

| 功能 | 说明 |
|------|------|
| **合并操作** | 玩家选中己方棋子，与相邻格子合并（字母或数字模运算），生成新的格子 |
| **滑行操作** | 将己方棋子移动到相邻的空格，扩展可操作范围 |
| **单人闯关模式** | 3 个关卡，每关设分数线，通过后可解锁下一关 |
| **VS AI 对战** | 玩家与 AI 轮流操作，各自拥有 5 个棋子，先无棋可走者出局 |
| **AI 对手** | 支持 Normal（随机策略）和 Hard（贪心策略）两种难度 |
| **排行榜系统** | 记录玩家成绩，按得分降序排列，数据本地持久化 |
| **游戏回放** | 完整录制每一步操作，可回放复盘 |
| **自定义题面** | 用户可放入自定义题面文件，自动加载 |
| **双渲染模式** | 支持控制台 ASCII 渲染和 SFML 图形界面渲染（可选） |
| **音效与动画** | SFML 模式下提供 5 种音效和合并动画效果 |

### 1.3 用户角色分析

| 角色 | 可执行操作 |
|------|-----------|
| **玩家（普通用户）** | 单人闯关、VS AI 对战、查看排行榜、录制/查看回放、自定义题面 |
| **访客（未输入姓名）** | 同上（默认名称为 "Player"） |

系统为单机设计，不设多用户权限体系，所有功能对所有用户开放。

### 1.4 预期效果与特色

**呈现效果：** 同时支持**控制台 CLI**（ASCII 字符界面，无需任何依赖）和 **SFML GUI**（图形窗口、鼠标操作、彩色渲染）。

**特色亮点：**
- 双渲染架构：通过 `IRenderer` 抽象接口实现 Console / SFML 自由切换
- 确定性题面生成：固定随机种子 + 连通性保证算法，每次构建生成相同的可玩题面
- BFS 可达性检测：从棋子出发沿空格 BFS 搜索可合并目标，而非简单扫描邻居
- AI 双策略：随机策略（带混沌因子）和贪心策略，适应不同玩家水平
- 完整数据持久化：排行榜、闯关进度、回放文件全部以纯文本格式本地存储
- RAII 内存管理：全部动态对象通过 `std::unique_ptr` 管理，无手动 delete

---

## 二、系统设计说明 (System Design Specification)

### 2.1 数据结构说明

#### 2.1.1 自定义类与结构体

本系统采用 MVC 架构，定义了 17 个自定义类和 3 个结构体。其中 Core（核心逻辑）和 Model（数据持久化）同属 **Model 层**的两个子层，加上 Controller 层和 View 层共三层。

##### Model 层 — Core 子层（核心数据模型）

**① Cell（格子单元）** — `include/core/Cell.hpp`

核心成员：
```cpp
char _letter;    // 字母 'A'~'Z'
int  _number;    // 数字 0~9
bool _empty;     // 是否已被合并清空
bool _selected;  // UI 选中状态（仅渲染用）
```

设计意图：游戏中最小的数据原子，采用**值语义**设计，支持拷贝和赋值。`Cell::makeEmpty()` 静态工厂创建空格子。`sameLetter()` 和 `sameNumber()` 在任意一方为空时返回 `false`，保证空格子不参与合并判定。每个 Cell 约 12 字节，连续存储时缓存友好。

**② Grid（网格容器）** — `include/core/Grid.hpp`

核心成员：
```cpp
int _rows;                            // 行数（默认 25）
int _cols;                            // 列数（默认 25）
std::vector<std::vector<Cell>> _cells; // 二维格子数组
```

设计意图：网格是游戏的核心容器，封装全部棋盘操作。提供带边界检查的 `at(r,c)` 访问、`mergeCells()` 合并、`slideCell()` 滑行、`findAllValidMoves()` 全盘扫描。序列化接口支持从文本文件重建网格。

**③ GameState（游戏状态）** — `include/core/GameState.hpp`

核心成员：
```cpp
Grid                                _grid;          // 当前棋盘
std::string                         _puzzleId;      // 题面 ID
std::set<std::pair<int,int>>        _playerCells;   // 玩家棋子坐标集合
std::set<std::pair<int,int>>        _aiCells;       // AI 棋子坐标集合
int                                 _stepsTaken;    // 总步数
int                                 _playerSteps;   // 玩家步数
int                                 _aiSteps;       // AI 步数
std::vector<Move>                   _moveHistory;   // 操作历史
std::chrono::steady_clock::time_point _startTime;   // 计时起点
bool                                _timerStarted;  // 计时器是否已启动
bool                                _gameOver;      // 游戏结束标记
```

设计意图：一次游戏的完整运行时快照，Controller 通过此对象读写全部游戏数据。`hasValidMoves(owner)` 使用 BFS 从棋子出发搜索所有可达合并。通过 `std::set` 分别追踪玩家和 AI 的棋子归属。

**④ Move（操作记录）** — `include/core/Move.hpp`

```cpp
struct Move {
    MoveType  moveType;   // MERGE 或 SLIDE
    int       srcRow, srcCol; // 源格坐标
    int       dstRow, dstCol; // 目标格坐标
    Direction dir;         // UP / DOWN / LEFT / RIGHT / NONE
    char      resultLetter; // 操作后结果字母
    int       resultNumber; // 操作后结果数字
};
```

设计意图：记录一步完整操作，用于回放系统和操作历史。包含 `serialize()` / `deserialize()` 支持文本序列化。

**⑤ MergeRule（合并规则引擎）**

纯静态工具类，构造函数被 `= delete`，不包含任何成员变量。只通过静态方法 `canMerge()`、`mergeLetters()`、`mergeNumbers()`、`getMergedCell()` 向外提供服务。

##### Model 层 — Persistence 子层（数据持久化）

**⑥ FileManager** — 封装全部底层文件 I/O，构造函数指定数据根目录（`"data"`）。提供 `readAll()`、`writeAll()`、`appendLine()`、`exists()`、`createDir()`。

**⑦ Puzzle** — 单个题面的数据模型，包含 ID（`std::string`）、名称（`std::string`）和初始 Grid。支持 `serialize()` / `deserialize()`。

**⑧ PuzzleManager** — 题库管理器，持有 `std::vector<Puzzle>`。负责内置题面生成（确定性种子 + 连通性保证）和自定义题面从文件加载。

**⑨ ScoreRecord** — 单次游戏记录，包含玩家名、题面 ID、用时（秒）、步数、总分、时间戳。得分公式：`max(0, 500 - time×2) + steps×10`。

**⑩ Leaderboard** — 排行榜容器，持有 `std::vector<ScoreRecord>`，支持按分数/时间排序查询，数据持久化到文件。

**⑪ Progress** — 闯关进度管理，记录已解锁的最高关卡和各关最高分。支持 `updateLevel()` 自动解锁下一关。

**⑫ ReplayBuffer** — 回放录制器，录制完整 `Move` 序列，支持保存到文件和从文件加载回放。

**⑬ Player** — 玩家信息，包含玩家名和 5 个起始格坐标。

##### Controller 层（游戏控制）

**⑭ GameController（主控制器）** — 持有全部 Model 和 View 对象的 `unique_ptr`，通过 9 阶段状态机驱动游戏。

**⑮ AIPlayer（AI 对手）** — 封装 AI 策略（RANDOM / GREEDY），提供 `findBestMove()`、`findAllValidMoves()`、`pickStartingCells()`。

**⑯ InputHandler（输入处理器）** — 控制台模式下将屏幕坐标映射到网格坐标。

##### View 层（渲染）

**⑰ IRenderer（渲染器接口）** — 纯虚抽象基类（抽象接口），定义 `render()`、`showMenu()`、`showLeaderboard()`、`waitForAction()` 等 13 个纯虚方法。

**⑱ ConsoleRenderer（控制台渲染器）** — 继承自 IRenderer，使用 Windows Console API（`SetConsoleTextAttribute`、`SetConsoleCursorPosition`）实现 ASCII 渲染。

**⑲ SFMLRenderer（图形渲染器）** — 继承自 IRenderer（由 `#ifdef HAS_GUI` 条件编译），使用 SFML 3 实现图形窗口、鼠标交互、合并动画和音效播放。

**⑳ SoundManager（音效管理器）** — 封装 SFML Audio，管理 5 种音效的加载和播放。由 `#ifdef HAS_GUI` 保护。

##### 综合类继承关系图

```
IRenderer（抽象基类，13 个纯虚方法）
  ├── ConsoleRenderer（控制台实现，Windows Console API）
  └── SFMLRenderer（图形实现，SFML 3，由 HAS_GUI 条件编译）

MergeRule（纯静态工具类，构造函数 = delete）
```

#### 2.1.2 核心容器 / 数据组织方式

| 容器类型 | 使用位置 | 存储内容 | 选择理由 |
|----------|---------|---------|---------|
| `std::vector<std::vector<Cell>>` | `Grid._cells` | 二维网格所有 Cell | **连续内存，O(1) 随机访问**。Cell 是 ~12 bytes 的 POD 对象，vector 的连续存储保证了缓存局部性，适合频繁的棋盘遍历 |
| `std::set<std::pair<int,int>>` | `GameState._playerCells`, `_aiCells` | 双方棋子坐标 | **O(log n) 高效增删查**。棋子数量极少（≤5），但需要频繁判断某格归属（`count()` 方法）。set 的平衡树结构也方便遍历所有棋子 |
| `std::vector<Move>` | `GameState._moveHistory`, `ReplayBuffer` | 操作历史序列 | **顺序访问、尾部追加**。操作历史是按时间顺序追加的序列，vector 的 `push_back()` 均摊 O(1)，且连续内存方便序列化输出 |
| `std::vector<ScoreRecord>` | `Leaderboard._records` | 排行榜记录 | **排序查询**。排行榜需要按分数降序输出，vector 配合 `std::sort()` 实现 O(n log n) 排序 |
| `std::map<std::string, int>` | `Progress._bestScores` | 关卡最高分 | **键值查询**。以关卡名为 key 快速查找最高分，O(log n) |
| `std::unique_ptr<T>` | `GameController` 全部成员 | 所有 Model 和 View 对象 | **独占所有权、自动释放**。每个对象只有唯一的拥有者，析构时自动 delete，杜绝内存泄漏 |
| `std::chrono::steady_clock` | `GameState._startTime` | 计时起点 | **单调时钟**。不受系统时间调整影响，适合测量时间间隔 |

### 2.2 程序功能说明

#### 2.2.1 各模块功能

##### 系统功能模块图

```
┌─────────────────────────────────────────────────────────────────┐
│                        GameController                           │
│                        （主控状态机）                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────────┐  │
│  │ 菜单处理  │ │ 单人游戏  │ │ VS AI   │ │ 排行榜/结果展示   │  │
│  │ handleMenu│ │ handle-  │ │ 对战     │ │ handleLeaderboard│  │
│  │          │ │ Gameplay │ │ handle-  │ │ handleResult      │  │
│  │          │ │          │ │ VSAIWatch│ │ finishVSAI        │  │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────────┬──────────┘  │
│       │            │            │                 │              │
│       ▼            ▼            ▼                 ▼              │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    核心逻辑层                              │    │
│  │  ┌──────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐   │    │
│  │  │Merge │ │GameState │ │AIPlayer  │ │InputHandler  │   │    │
│  │  │Rule  │ │(BFS检测) │ │(随机/贪心)│ │(坐标映射)     │   │    │
│  │  └──────┘ └──────────┘ └──────────┘ └──────────────┘   │    │
│  └─────────────────────────────────────────────────────────┘    │
│       │            │                                            │
│       ▼            ▼                                            │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    数据持久层                              │    │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────────┐  │    │
│  │  │FileMgr   │ │PuzzleMgr │ │Leaderboard│ │Progress    │  │    │
│  │  │(文件I/O)  │ │(题库)    │ │(排行榜)   │ │(闯关进度)   │  │    │
│  │  └──────────┘ └──────────┘ └──────────┘ └────────────┘  │    │
│  │  ┌──────────┐ ┌──────────┐                               │    │
│  │  │ReplayBuf │ │ScoreRec  │                               │    │
│  │  │(回放)     │ │(得分计算) │                               │    │
│  │  └──────────┘ └──────────┘                               │    │
│  └─────────────────────────────────────────────────────────┘    │
│       │                                                        │
│       ▼                                                        │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    渲染层 (IRenderer 接口)                  │    │
│  │  ┌──────────────────┐   ┌──────────────────┐              │    │
│  │  │ ConsoleRenderer   │   │ SFMLRenderer     │              │    │
│  │  │ (控制台 ASCII)    │   │ (图形窗口+动画)   │              │    │
│  │  │                  │   │ SoundManager     │              │    │
│  │  └──────────────────┘   └──────────────────┘              │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

##### 各模块详细功能

| 模块 | 所在文件 | 功能简述 |
|------|---------|---------|
| **Cell** | `core/Cell.*` | 格子数据单元，封装字母+数字+状态，提供比较和格式化 |
| **Grid** | `core/Grid.*` | 二维网格容器，提供合并/滑行/全盘扫描/序列化 |
| **MergeRule** | `core/MergeRule.*` | 纯静态规则引擎，判定合并条件并计算结果 |
| **GameState** | `core/GameState.*` | 游戏运行时状态聚合，棋子归属追踪，BFS 合法性检测，计时 |
| **Move** | `core/Move.*` | 操作记录结构体，支持文本序列化/反序列化 |
| **FileManager** | `model/FileManager.*` | 文件 I/O 底层封装（读/写/追加/创建目录） |
| **Puzzle** | `model/Puzzle.*` | 题面数据模型 + 文本序列化 |
| **PuzzleManager** | `model/PuzzleManager.*` | 题库加载、确定性生成、连通性保证、自定义题面管理 |
| **ScoreRecord** | `model/ScoreRecord.*` | 得分计算（`max(0, 500-time×2) + steps×10`）与 CSV 序列化 |
| **Leaderboard** | `model/Leaderboard.*` | 排行榜 CRUD，按分数/时间排序查询 |
| **Progress** | `model/Progress.*` | 闯关进度追踪，自动解锁下一关 |
| **ReplayBuffer** | `model/ReplayBuffer.*` | 全量操作序列录制，结构化回放文件 I/O |
| **GameController** | `controller/GameController.*` | 主控状态机（~1225 行），驱动全部游戏流程 |
| **AIPlayer** | `controller/AIPlayer.*` | AI 对手（~307 行），双策略 + 智能起手选子 |
| **InputHandler** | `controller/InputHandler.*` | 屏幕坐标 ↔ 网格坐标转换 |
| **IRenderer** | `view/IRenderer.hpp` | 渲染器抽象接口，定义 13 个纯虚方法 |
| **ConsoleRenderer** | `view/ConsoleRenderer.*` | 控制台 ASCII 渲染（~589 行），Windows Console API |
| **SFMLRenderer** | `view/SFMLRenderer.*` | 图形界面渲染（~1900 行），鼠标交互+动画+音效 |
| **SoundManager** | `view/SoundManager.*` | 5 种音效加载管理（由 HAS_GUI 条件编译） |

#### 2.2.2 算法描述

##### 算法 1：合并规则判定 — `MergeRule::canMerge()` — O(1)

```
输入：格子 A (letterA, numberA, isEmptyA), 格子 B (letterB, numberB, isEmptyB)
输出：布尔值

if isEmptyA OR isEmptyB:
    return false                         // 空格子不可合并

sameLetter ← (letterA == letterB)
sameNumber ← (numberA == numberB)

if sameLetter AND NOT sameNumber:
    return true                          // 字母同、数字异 → 数字相加 mod 10
if sameNumber AND NOT sameLetter:
    return true                          // 数字同、字母异 → 字母相加 mod 26
if sameLetter AND sameNumber:
    return true                          // 全相同 → 允许但结果不变
return false                             // 全不同 → 不可合并
```

**设计意图：** 本算法的核心是"部分相同即可合并"的规则。两格只要共享一个属性（字母或数字）即可触发合并，另一属性做模运算。这保证了棋盘上任意两格总有概率形成可合并对，增加了游戏的策略性。

##### 算法 2：BFS 可达性搜索 — `GameState::hasValidMoves()` — O(K × gridArea)

```
输入：owner（Player 或 AI）
输出：布尔值（该方是否存在至少一步合法操作）

cells ← 若 owner=Player 取 _playerCells，否则取 _aiCells

for each 棋子坐标 (r, c) in cells:
    if 该格为空: continue
    
    初始化 BFS 队列 queue，起点 (r, c)
    初始化 visited 集合，加入 (r, c)
    
    while queue 不为空:
        (cr, cc) ← queue.pop()
        
        for each 方向 d in {上,下,左,右}:
            (nr, nc) ← (cr + dirs[d][0], cc + dirs[d][1])
            if (nr, nc) 超出棋盘范围: continue
            
            neighbor ← grid.at(nr, nc)
            
            if neighbor 是空格 AND (nr, nc) 未被访问过:
                visited.insert((nr, nc))
                queue.push((nr, nc))       // ← 空格可作为滑行路径继续延伸
                
            elif neighbor 非空 AND (nr, nc) ≠ (r, c)：
                if MergeRule::canMerge(原始棋子, neighbor):
                    return true             // ← 找到一个可达的合法合并

return false  // 所有棋子均无合法操作
```

**为什么用 BFS？** 如果只检查棋子的 4 个直接邻居，会遗漏"先滑行穿过一段空格，再与远处的格子合并"的情况。BFS 沿空格传播搜索前沿，确保所有可达位置都被检查到。

**场景示例：** 玩家棋子在 (5,5)，左边 (5,4) 是空格，(5,3) 是空格，(5,2) 是对手棋子 B3。BFS 从 (5,5) → (5,4) → (5,3)，然后检查 (5,2) 的 B3 是否可与 (5,5) 的棋子合并。若用直接邻居扫描，则只能检查到 (5,4) 是空格，漏掉了 (5,2)。

##### 算法 3：AI 贪心策略 — `AIPlayer::greedyPick()` — O(N log N)

```
输入：moves（所有合法操作列表）
输出：选中的最佳操作

将 moves 分为两组：merges（合并操作）和 slides（滑行操作）

if 随机数 [1,100] ≤ 15:           // 15% 混沌因子
    return 从 moves 中均匀随机选一个

if merges 不为空:
    对每个 merge 操作 m 计算得分：
        score(m) = (m.resultLetter - 'A' + 1) + m.resultNumber
    return 得分最高的 merge 操作     // ← 选择字母+数字总值最大的合并

if slides 不为空:
    return 随机选一个 slide 操作

return nullopt  // 无合法操作
```

##### 算法 4：AI 随机策略 — `AIPlayer::randomPick()` — O(N)

```
输入：moves（所有合法操作列表）
输出：选中的操作

将 moves 分为 merges 和 slides

if 随机数 [1,100] ≤ 40:           // 40% 纯随机混沌因子
    return 从所有 moves 中均匀随机选一个

if merges 不为空 AND slides 不为空:
    if 随机数 [1,100] ≤ 70:       // 70% 概率选合并
        return 从 merges 中均匀随机选一个
    else:
        return 从 slides 中均匀随机选一个
else if merges 不为空:
    return 从 merges 中随机选一个
else if slides 不为空:
    return 从 slides 中随机选一个
else:
    return nullopt
```

##### 算法 5：题面生成与连通性保证 — `PuzzleManager::createBuiltinPuzzles()` — O(gridArea)

```
输入：无（内置固定种子）
输出：3 道 25×25 的 Puzzle

for pid = 0 to 2:
    std::srand(42 + pid × 1000)          // 确定性种子，每次编译生成相同题面
    grid ← new Grid(25, 25)
    
    // 步骤 1：随机填充
    for r = 0 to 24:
        for c = 0 to 24:
            letter ← 随机 A~Z
            number ← 随机 0~9
            grid[r][c] ← Cell(letter, number)
    
    // 步骤 2：连通性保障扫描
    for r = 0 to 24:
        for c = 0 to 24:
            if 存在邻居可与 grid[r][c] 合并:
                continue                // 已满足，跳过
            
            搜集所有在界内的邻居坐标
            pick ← 随机选一个邻居
            if 随机选择为"改字母":
                改本格字母，使其与邻居字母不同但数字相同
            else:
                改本格数字，使其与邻居数字不同但字母相同
    
    // 步骤 3：安全兜底
    if grid 无任何合法合并:
        在 (0,0) 放 A1，(0,1) 放 A2        // 保证至少一对可合并
    
    构造 Puzzle(builtin_0X, name_X, grid)
    保存到文件 data/puzzles/builtin_0X.txt
```

##### 算法 6：AI 起手选子 — `AIPlayer::pickStartingCells()` — O(gridArea)

```
输入：grid, count (=5)
输出：count 个起始格坐标

for each 非空格 (r,c):
    统计 4 个邻居中可与本格合并的数量 mergeableCount
    统计 4 个邻居中非空格的数量 nonEmptyCount
    score ← mergeableCount × 10 + nonEmptyCount    // 合并潜力评分

按 score 降序排列所有候选格

selected ← []
for each 候选格按分数从高到低:
    if selected 为空 OR 与已选所有格的曼哈顿距离 ≥ 3:
        selected.push(本格)
        if selected.size() == count:
            break

// 若数量不足，放宽距离限制补满
while selected.size() < count:
    从剩余候选中选分数最高的加入

return selected
```

**设计意图：** AI 优先选择"周围有更多可合并邻居"的格子作为起点，评分公式中 mergeableCount × 10 权重远高于 nonEmptyCount，确保 AI 早期就有活跃的合并机会。

#### 2.2.3 接口说明

##### (A) IRenderer 抽象接口（13 个纯虚方法）

| 方法签名 | 功能 | 参数含义 | 返回值 |
|----------|------|---------|--------|
| `void render(const GameState& state)` | 渲染游戏主界面 | state: 当前游戏状态 | — |
| `void showMenu()` | 显示主菜单 | — | — |
| `void showPuzzleList(const vector<Puzzle>&)` | 显示题面列表 | puzzles: 可选题面列表 | — |
| `void showLevelList(const vector<Puzzle>&, int maxUnlocked, const vector<int>& bestScores)` | 显示闯关关卡 | maxUnlocked: 最大已解锁, bestScores: 最高分 | — |
| `void showLeaderboard(const vector<ScoreRecord>&)` | 显示排行榜 | records: 成绩记录列表 | — |
| `void showResult(const ScoreRecord&, const vector<Move>&)` | 显示单人结果 | record: 成绩, moveHistory: 操作历史 | — |
| `void showVSResult(const ScoreRecord&, const ScoreRecord&, const string& winner)` | VS AI 结果对比 | player/AI记录, winner: 胜者 | — |
| `void showMessage(const string& msg)` | 弹窗消息 | msg: 消息内容 | — |
| `string promptPlayerName()` | 提示玩家输入姓名 | — | 玩家名 |
| `vector<pair<int,int>> promptStartingCells(const Grid&, int count)` | 提示选择起始格 | count: 需选数量 | 所选坐标列表 |
| `UserAction waitForAction()` | 等待用户操作 | — | UserAction 枚举 |
| `bool isOpen() const` | 窗口是否有效 | — | true/false |
| `void setTurnMessage(const string&)` | 设置回合提示 | msg: 提示文字 | — |

**UserAction 枚举**（22 种）：
`QUIT`, `CONFIRM`, `BACK`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `SELECT_CELL`, `MERGE_DIRECTION`, `SUBMIT`, `VIEW_LEADERBOARD`, `SELECT_PUZZLE_1`~`5`, `CUSTOM_PUZZLE`, `RESIZE_BOARD`, `TOGGLE_REPLAY`, `NEXT_FRAME`, `AI_EXECUTE`, `VS_AI_NORMAL`, `VS_AI_ADVANCED`, `NONE`

##### (B) GameController 关键接口

```cpp
// 主循环入口
void run();    // while(_running) 状态机调度

// 阶段处理函数
void handleMenu();              // 主菜单 → 1闯关 2排行榜 3VS AI 4退出
void handleLevelSelect();       // 关卡选择（含解锁校验）
void handleGameplay();          // 单人游戏主循环
void handleLeaderboard();       // 显示排行榜
void handleVSAIWatch();         // VS AI 交替回合主循环
void finishVSAI();              // VS AI 结算

// 操作执行
bool tryMerge(int srcR, int srcC, int dstR, int dstC);     // 玩家合并
bool trySlide(int srcR, int srcC, int dstR, int dstC);     // 玩家滑行
bool doAIMerge(CellOwner owner, int srcR, int srcC, int dstR, int dstC);  // 通用合并
bool doAISlide(CellOwner owner, int srcR, int srcC, int dstR, int dstC);  // 通用滑行

// VS AI 回合
bool processVSAIPlayerAction();  // 玩家回合，返回 false 表示无棋可走
bool executeVSAIAIAction();      // AI 走一步，返回 false 表示无棋可走
void submitAnswer();             // 提交结算
```

##### (C) AIPlayer 接口

```cpp
// 设置/获取策略
void setStrategy(AIStrategy s);          // RANDOM / GREEDY
AIStrategy strategy() const;

// 寻找最佳操作
optional<AIMove> findBestMove(const GameState& state) const;                              // 从全部棋子中搜索
optional<AIMove> findBestMove(const GameState&, const set<pair<int,int>>& myCells) const;  // 从指定集合搜索

// 列举所有合法操作
vector<AIMove> findAllValidMoves(const GameState&) const;
vector<AIMove> findAllValidMoves(const GameState&, const set<pair<int,int>>&) const;

// 起手选子
vector<pair<int,int>> pickStartingCells(const Grid& grid, int count) const;
```

##### (D) 序列化接口

| 类 | 序列化格式示例 | 用途 |
|-----|---------------|------|
| `Move` | `[Move] 001 3,4 -> 3,5 RIGHT A5` | 回放文件中的操作记录 |
| `ScoreRecord` | `Player1,builtin_01,120.50,45,0.00,500,2026-06-11 10:30` | 排行榜 CSV |
| `Puzzle` | `# builtin_01 Beginner Training\n25 25\nA5 B3 C1 .0 D7...` | 题面文件 |
| `Grid` | `25 25\nA5 B3 .0 D7...` | 网格数据（被 Puzzle 序列化包含） |
| `ReplayBuffer` | `[Puzzle] builtin_01\n[Grid] 25x25\n[Player] ... [Move] ...` | 回放文件 |
| `Progress` | `max_unlocked = 3\nlevel_1_best = 500` | 闯关进度文件 |

#### 2.2.4 关键变量说明

##### (A) GameController 核心成员

| 变量 | 类型 | 说明 |
|------|------|------|
| `_state` | `unique_ptr<GameState>` | 当前游戏运行时状态，游戏结束后 `reset()` |
| `_player` | `Player` | 玩家信息（姓名 + 起始格） |
| `_aiPlayer` | `AIPlayer` | AI 对手实例 |
| `_phase` | `GamePhase` | **状态机核心**：当前游戏阶段（9 种枚举值） |
| `_running` | `bool` | **主循环控制标志**：`true` 时持续运行，`false` 时退出 |
| `_cursorRow, _cursorCol` | `int` | 光标位置（控制台模式下的用户焦点） |
| `_gridRows, _gridCols` | `int` | 网格尺寸（由 Puzzle 加载时设置，默认 25×25） |
| `_currentLevel` | `int` | 闯关模式当前关卡号（0 = 非闯关模式） |
| `_vsAI` | `bool` | VS AI 对战模式标记 |
| `_aiDelayMs` | `int` | AI 走子间的延迟毫秒数（Normal=300, Hard=500） |
| `_aiStrategy` | `AIStrategy` | AI 策略（RANDOM 或 GREEDY） |

##### (B) SFMLRenderer 核心成员

| 变量 | 类型 | 说明 |
|------|------|------|
| `_window` | `sf::RenderWindow` | SFML 渲染窗口（1400×900） |
| `_screen` | `UIScreen` | 当前 UI 屏幕枚举（Menu / Gameplay / Leaderboard 等 11 种） |
| `_font` | `sf::Font` | 渲染字体（多路径回退加载：项目 → 系统字体） |
| `_text` | `unique_ptr<sf::Text>` | 文字渲染对象 |
| `_rect` | `sf::RectangleShape` | 矩形绘制对象（按钮、格子背景、分隔线共用） |
| `_cellSize` | `float` | 每个格子的像素大小（窗口尺寸 / 网格尺寸，动态计算） |
| `_cursorRow, _cursorCol` | `int` | 光标/焦点所在的网格位置 |
| `_selectedRow, _selectedCol` | `int` | 两段式选中模式的源格位置 |
| `_hasSelection` | `bool` | 是否已选中一个源格 |
| `_anim` | `AnimState` | 合并动画状态（含时钟、源/目标坐标、结果格子数据） |
| `_soundMgr` | `SoundManager` | 音效管理器 |

##### (C) GameState 核心成员

| 变量 | 类型 | 说明 |
|------|------|------|
| `_grid` | `Grid` | 当前棋盘（值语义，所有格子数据） |
| `_playerCells` | `set<pair<int,int>>` | 玩家棋子在棋盘上的全部坐标 |
| `_aiCells` | `set<pair<int,int>>` | AI 棋子在棋盘上的全部坐标 |
| `_playerSteps, _aiSteps` | `int` | 双方各自的累计步数（用于 VS AI 评分） |
| `_stepsTaken` | `int` | 总步数（用于单人模式评分） |
| `_startTime` | `steady_clock::time_point` | 计时起点 |
| `_timerStarted` | `bool` | 是否首次执行操作（用于延迟计时起点） |
| `_gameOver` | `bool` | 游戏是否已结束 |

---

## 三、实现环境 (Implementation Environment)

### 3.1 设计平台要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 11 专业版（中文） |
| 处理器 | 12th Gen Intel Core i5-12500H |
| 开发工具 | Visual Studio Code + CMake Tools 插件 |
| 编译器 | MSVC 2022 (Visual Studio 17 Build Tools) |
| C++ 标准 | C++17 |
| 构建系统 | CMake ≥ 3.20 |
| 第三方依赖 | SFML 3.0（Graphics / Window / System / Audio），通过 vcpkg 管理 |
| 版本控制 | Git |

### 3.2 运行环境要求

| 模式 | 最低配置 |
|------|---------|
| **控制台模式** | Windows 10+，支持 UTF-8 的控制台窗口，无需任何外部依赖 |
| **SFML 图形模式** | 控制台模式要求 + 支持 OpenGL 3.3+ 的显卡 + SFML 3 运行时 DLL |
| **屏幕分辨率** | 推荐 1400×900 以上（SFML 窗口固定 1400×900） |
| **音效** | 扬声器或耳机（仅 SFML 模式支持） |

**启动方式：**
```bash
# 必须从项目根目录启动（否则相对路径解析失败）
cd E:/Matrix\ breakthrough
build/Release/matrix_breakthrough.exe    # Release 版
# 或
build/Debug/matrix_breakthrough.exe      # Debug 版
```

---

## 四、设计实现及分析 (Design Implementation and Analysis)

### 4.1 程序设计和调试过程出现的问题及解决方法

#### 问题 1：内置题面三道相同

- **问题表现：** 首次运行生成的 3 道内置题面（builtin_01 ~ 03）内容完全一样，玩家每次都面对相同的棋盘。
- **原因分析：** `std::srand()` 放在生成每道题的循环末尾，导致下次循环时使用的是上一轮最后的随机状态。由于三道题生成逻辑相同，随机种子退化到相同轨迹。
- **调试方法：** 运行时实际对比 3 个题面文件的内容发现相同。在 `srand` 调用处加 `std::cerr` 输出每次的随机数序列前 5 个值，确认种子退化。
- **解决方案：** 将 `std::srand(42 + pid * 1000)` 移到每轮循环开头，用 `pid × 1000` 确保三道题使用不同种子空间。

```cpp
// 修复前：srand 在循环末尾，种子退化
for (int pid = 0; pid < 3; ++pid) {
    // ... 生成逻辑 ...
    std::srand(time(nullptr));  // 太晚且粒度不够
}

// 修复后：每轮循环开头设置独立种子
for (int pid = 0; pid < 3; ++pid) {
    std::srand(42 + pid * 1000);  // 每个 pid 不同，确定性种子
    // ... 生成逻辑 ...
}
```

#### 问题 2：棋盘死局（无法操作）

- **问题表现：** 随机生成的棋盘上，某些格子周围没有任何可以合并的邻居。若玩家选择了这些格子作为起点，游戏开局即死局。
- **原因分析：** 纯随机填充不能保证每个格子都可合并。25×25 棋盘上有 625 个格子，随机分布下部分格子可能所有邻居的字母和数字都不同。
- **调试方法：** 遍历棋盘统计每个格子的可合并邻居数，发现有大量格子的 count = 0。
- **解决方案：** 新增"连通性保障扫描"（Connectivity Pass），随机填充后遍历全盘，为每个孤立格子修改字母或数字使其与某邻居可合并。附加安全兜底：若全盘仍无可合并对，强制在位置 (0,0) 和 (0,1) 放置 A1、A2。

```cpp
// 连通性保障扫描伪代码
for each 格子 (r,c):
    if 无任何邻居可与本格合并:
        选一个随机邻居
        if 随机选 0：改本格数字 = 邻居数字 + 随机偏移 (mod 10)
        else：改本格字母 = 邻居字母 + 随机偏移 (mod 26)
```

#### 问题 3：合并动画渲染异常

- **问题表现：** SFML 图形模式下，触发合并时整个窗口被一个 320×900 像素的巨大黄色矩形覆盖。
- **原因分析：** 动画使用的 `sf::RectangleShape _rect` 在设置位置（`setPosition`）后，未调用 `setSize` 设置尺寸，使用了 SFML 默认的矩形尺寸（320×900 或其他默认值）。
- **调试方法：** 在触发动画处打断点，检查 `_rect` 的 `getSize()` 返回值，发现是未初始化的大尺寸。
- **解决方案：** 在 `triggerMergeAnim()` 中添加 `_rect.setSize(sf::Vector2f(cellSize, cellSize))`，确保使用正确的格子像素尺寸。

#### 问题 4：VS AI 模式 AI 无限循环

- **问题表现：** 玩家无棋可走出局后，使用 RANDOM 策略的 AI 在 25×25 大棋盘上漫无目的地滑动，游戏无法自然结束。
- **原因分析：** AI RANDOM 策略有 40% 的混沌因子，70% 情况下在合并/滑行间随机选择，且无合并时纯粹随机滑行。AI 不会主动走向"无棋可走"的状态。
- **调试方法：** 在 `executeVSAIAIAction()` 中添加步数计数器，观察 AI 连续 100+ 步仍在移动但分数无增长。
- **解决方案：** 当玩家出局时，弹窗告知后，在后台将 AI 临时切换为 GREEDY 策略、`_aiDelayMs = 0`（零延迟）快速走完剩余回合，然后立即进入结算。

```cpp
if (!_state->hasValidMoves(CellOwner::Player)) {
    _renderer->showMessage("You have no more valid moves!");
    // 后台静默执行 AI 剩余回合
    _aiPlayer.setStrategy(AIStrategy::GREEDY);
    _aiDelayMs = 0;
    while (_state->hasValidMoves(CellOwner::AI)) {
        auto move = _aiPlayer.findBestMove(*_state, _state->aiCells());
        if (!move) break;
        // 执行操作（无渲染）
    }
    break;  // 进入结算
}
```

#### 问题 5：排行榜显示崩溃（数组越界）

- **问题表现：** 点击主菜单的 "View Leaderboard" 后，程序直接退出，没有任何错误提示。
- **原因分析：** `SFMLRenderer::showLeaderboard()` 中绘制表头的循环 `for (int i = 0; i < 7; ++i)` 越界访问了数组 `headers[6]`，而 `headers[]` 只有 6 个元素。`sf::Text::setString()` 收到垃圾指针导致段错误 (SIGSEGV, exit code 139)。
- **调试方法：** 从命令行运行程序时通过管道模拟 'L' 键输入，观察到程序因 SIGSEGV 退出。阅读 `SFMLRenderer.cpp` 发现循环次数（7）与数组元素数（6）不匹配。
- **解决方案：** 将 `i < 7` 改为 `i < 6`。并检查全项目类似模式，确认无其他 off-by-one 循环错误。

#### 问题 6：控制台中文乱码

- **问题表现：** 控制台模式下中文文本显示为乱码。
- **原因分析：** MSVC 默认使用系统代码页（Windows 中文系统为 GBK/936），而源码以 UTF-8 编码保存，`std::cout` 输出 UTF-8 字节序列被解码为 GBK 导致乱码。
- **调试方法：** 在代码中插入 `std::cout << (int)str[0]` 观察字节流，确认是 UTF-8 多字节序列。
- **解决方案：** CMakeLists.txt 中添加 MSVC 专属编译选项 `/utf-8`，同时在项目中将界面文字逐步改为英文，彻底规避编码兼容问题。

#### 问题 7：双击 exe 音效无法加载

- **问题表现：** 在文件管理器中双击 `build/Debug/matrix_breakthrough.exe` 启动程序，音效不工作（所有操作无声）。
- **原因分析：** Windows 将工作目录设置为 exe 所在目录，即 `build/Debug/`。音效文件路径 `assets/sounds/` 相对解析为 `build/Debug/assets/sounds/`，该目录不存在。
- **调试方法：** 在 `SoundManager::load()` 中添加 `std::cerr` 输出完整路径，确认路径解析错误。
- **解决方案：** 在项目根目录创建 `run.bat` 启动脚本，先切换到项目根目录再启动 exe。同时在代码层面，后续可考虑使用 `GetModuleFileNameW` 获取 exe 绝对路径来定位资源目录。

### 4.2 程序测试方法和数据

#### 测试方法

| 测试类型 | 方法 | 覆盖范围 |
|----------|------|----------|
| **编译测试** | `cmake -B build` + `cmake --build build`，开关 ENABLE_GUI | 全部源码通过 MSVC 编译，零警告 |
| **双模式测试** | 分别以 ENABLE_GUI=ON/OFF 构建并运行 | Console + SFML 双渲染通道 |
| **功能测试** | 手动完成一局完整游戏流程 | 选子→合并→滑行→提交→排行榜全链路 |
| **VS AI 测试** | Normal / Hard 各对战 3 局 | 双方出局判定、得分计算、后台结算 |
| **边界测试** | 故意走到无棋可走 | 游戏结束弹窗、AI 后台走完、结算流程 |
| **排行榜测试** | 多次游戏后查看排行榜 | 排序正确性、空榜显示、持久化恢复 |
| **空榜测试** | 删除 leaderboard.txt 后启动 | "No records yet" 提示正常显示 |
| **回放测试** | 保存回放文件并重新加载播放 | Move 序列完整还原 |
| **闯关测试** | 分别以高于/低于分数线完成 | 关卡解锁、进度保存、LOCKED 状态 |

#### 测试数据

| 测试项 | 输入数据 | 预期结果 | 实际结果 |
|--------|---------|---------|---------|
| 正常合并 | 源格 A3，目标格 A7 | → A0，源置空，步数+1 | ✅ 通过 |
| 同格合并 | A1 + A1（字母数字全同） | → A1，源置空（结果不变） | ✅ 通过 |
| 不可合并 | A1 + B2（字母数字全不同） | 操作被拒绝 | ✅ 通过 |
| 滑行 | 源格 A5，目标格为空 | 源复制到目标，源清空 | ✅ 通过 |
| 单人闯关 | 得分 350（第 2 关分数线 300） | 通关，解锁第 3 关 | ✅ 通过 |
| 单人闯关 | 得分 250（第 2 关分数线 300） | 未通过，"Level not passed" 提示 | ✅ 通过 |
| VS AI 正常 | 玩家正常操作到出局 | 弹窗 → AI 后台结算 → 对比结果 | ✅ 通过 |
| VS AI 投降 | 玩家按 S 键投降 | 直接进入结算，无弹窗 | ✅ 通过 |
| 空排行榜 | leaderboard.txt 不存在 | "No records yet" 显示 | ✅ 通过 |
| 起手选子冲突 | 玩家与 AI 选了相同格子 | AI 自动调换到最近可用位置 | ✅ 通过 |

---

## 五、讨论 (Discussion)

### 5.1 程序结构评价

#### 面向对象三大特性的体现

**① 封装 (Encapsulation)**

每个类将内部数据和实现细节隐藏在 private 成员中，外部通过 public 方法访问：

```cpp
// Cell 封装示例：外部只能通过 getter/setter 访问
class Cell {
private:
    char _letter;
    int  _number;
    bool _empty;
public:
    char  getLetter() const;
    void  setLetter(char);
    bool  isEmpty() const;
    bool  sameLetter(const Cell&) const;  // 封装备份逻辑
};
```

**② 继承 (Inheritance)**

`IRenderer` 作为抽象基类定义渲染接口，`ConsoleRenderer` 和 `SFMLRenderer` 分别实现：

```cpp
class IRenderer {  // 抽象基类
    virtual void render(const GameState&) = 0;
    virtual void showMenu() = 0;
    virtual UserAction waitForAction() = 0;
    // ... 共 13 个纯虚方法
};

class ConsoleRenderer : public IRenderer { /* 控制台实现 */ };
class SFMLRenderer : public IRenderer { /* SFML 实现 */ };
```

**③ 多态 (Polymorphism)**

Controller 通过 `IRenderer*` 指针调用渲染方法，运行时动态绑定具体实现：

```cpp
// GameController 中
std::unique_ptr<IRenderer> _renderer;

// 根据编译配置选择实现
#ifdef HAS_GUI
    _renderer = std::make_unique<SFMLRenderer>();
#else
    _renderer = std::make_unique<ConsoleRenderer>();
#endif

// 多态调用（无需关心具体渲染器类型）
_renderer->render(*_state);
_renderer->showMessage("You have no more valid moves!");
```

#### 耦合与内聚分析

**高内聚：**
- Core 层的 `MergeRule` 只做合并规则判定一个事，`Cell` 只封装格子数据
- Model 层的每个类只管理一种数据（排行榜、进度、题面各司其职）
- `AIPlayer` 专注于 AI 策略，不同策略通过枚举切换而非多类继承

**低耦合：**
- Model 层完全不引用 View 和 Controller（零依赖）
- View 层通过 `IRenderer` 接口与 Controller 交互，Controller 不依赖具体渲染器
- 渲染器切换只需修改 `#ifdef HAS_GUI` 处的 `make_unique` 行

**可改进的不足：**
- Controller 层偏重（约 1225 行），单人、VS AI、菜单、排行榜等功能集中在一个类中
- View 和 Controller 之间通过 `dynamic_cast<SFMLRenderer*>` 检测渲染器类型来触发专属功能（音效、动画），破坏了接口的纯粹性。理想的方案是在 `IRenderer` 中统一声明 `playSound()` 接口

### 5.2 时空效率分析

#### 时间复杂度

| 算法 | 最坏复杂度 | 平均复杂度 | 说明 |
|------|-----------|-----------|------|
| 合并判定 `canMerge()` | O(1) | O(1) | 常数次条件判断，无循环 |
| BFS 可达性搜索 `hasValidMoves()` | O(K × gridArea) | O(K × reachableEmpty) | K ≤ 5, gridArea=625, 最坏约 3125 步 |
| AI 全盘扫描 `findAllValidMoves()` | O(N × 4) | O(N × 4) | N=棋子数，常数检测邻居 |
| AI 贪心选择 `greedyPick()` | O(N log N) | O(N) | 按分数排序 |
| AI 随机选择 `randomPick()` | O(N) | O(N) | 均匀分布随机选 |
| 排行榜排序 `topByScore()` | O(M log M) | O(M log M) | M=总记录数，仅在查看时排序 |
| 题面生成 | O(gridArea) | O(gridArea) | 每格常数次操作 + 连通性扫描 |
| 序列化/反序列化 | O(gridArea) | O(gridArea) | 逐格读写 |

**结论：** 核心算法均在 O(gridArea) 或更低级别，25×25 的规模下无任何性能瓶颈。游戏帧率仅受 SFML 渲染（按 60fps 垂直同步）限制。

#### 空间复杂度

| 数据结构 | 空间占用 | 详细构成 |
|----------|---------|---------|
| Grid | ~7.5 KB | 625 格 × ~12 bytes/Cell |
| GameState | ~15 KB | Grid + 集合(2×5×~48bytes) + 操作历史 |
| ReplayBuffer | ~6 KB/百步 | 每步 ~60 bytes（Move 结构体序列化） |
| Leaderboard | ~50 KB/百条 | 每条 ~500 bytes（CSV 行） |
| SFMLRenderer | ~10 MB | 窗口缓冲区 + 纹理 + 字体 + 音效缓冲 |
| 总内存占用 | < 20 MB | 远低于现代桌面系统限制 |

**内存管理方式：** 全部动态对象通过 `std::unique_ptr` 管理，无手动 `new`/`delete`。
```cpp
// GameController 中所有动态成员
std::unique_ptr<FileManager>    _fileManager;
std::unique_ptr<PuzzleManager>  _puzzleManager;
std::unique_ptr<Leaderboard>    _leaderboard;
std::unique_ptr<Progress>       _progress;
std::unique_ptr<ReplayBuffer>   _replayBuffer;
std::unique_ptr<IRenderer>      _renderer;
std::unique_ptr<InputHandler>   _inputHandler;
std::unique_ptr<GameState>      _state;
```
当 `GameController` 析构时，其所有 `unique_ptr` 成员自动释放，无内存泄漏风险。

### 5.3 调试方法评价

| 调试方法 | 工具/手段 | 应用场景 | 效果评价 |
|----------|-----------|---------|---------|
| **编译警告** | MSVC /W4 | 未初始化变量、类型不匹配 | ★★★ 在编译阶段拦截低级错误 |
| **断言检查** | `std::out_of_range` | Grid::at() 越界访问 | ★★★ 迅速定位数组越界 |
| **运行时日志** | `std::cerr` | 文件加载失败、异常路径 | ★★ 适合追踪 I/O 类问题 |
| **异常捕获** | try-catch | 全局未捕获异常 | ★★ 防止静默崩溃 |
| **退出码分析** | `echo $?` | 程序崩溃类型判断（139=段错误） | ★★★ 快速区分逻辑错误与内存问题 |
| **代码审查** | 人工阅读 | 数组越界、逻辑错误 | ★★★ 排行榜 off-by-one 就是这样找到的 |
| **双模式对比** | Console vs SFML | 区分渲染层和逻辑层 bug | ★★★ 隔离问题来源的关键方法 |
| **进程监视** | 任务管理器 | 确认程序是否意外退出 | ★ 基础实用 |

**关键经验：** 调试排行榜崩溃问题时，通过管道输入模拟用户操作+检查退出码（139 = SIGSEGV），比手动点击 GUI 窗口更高效。也验证了"在 GUI bug 难以复现时，切换 Console 模式看问题是否依然存在"的双模式对比法非常有效。

### 5.4 系统局限性与改进方向

| 不足 | 说明 | 改进方向 |
|------|------|---------|
| **Controller 臃肿** | GameController (1225 行) 集中了全部业务逻辑 | 拆分为 MenuController、GameplayController、VSAIController 等子控制器 |
| **dynamic_cast 耦合** | 多处用 `dynamic_cast<SFMLRenderer*>` 调用音效/动画 | 在 IRenderer 接口中统一声明 `playSound()` / `triggerAnimation()` |
| **界面美观度** | Console 模式纯 ASCII，SFML 模式基础矩形绘制 | 使用纹理贴图、渐变背景、粒子特效 |
| **并发缺失** | 所有操作单线程，AI 计算期间 UI 无法响应 | 引入 `std::async` 或 `std::thread` 异步执行 AI 搜索 |
| **数据安全性** | leaderboard.txt 和 progress.txt 纯文本无加密 | 引入 JSON 格式、CRC 校验或简单 XOR 加密 |
| **网络对战** | 仅支持本地 VS AI | 使用 WebSocket 或 UDP 实现局域网/互联网对战 |
| **AI 智能程度** | 贪心策略仅看一步，无前瞻 | 实现 minimax 搜索或蒙特卡洛树搜索 (MCTS) |
| **无障碍访问** | 不支持键盘导航的图形模式 | 增加 Tab 导航、屏幕阅读器支持 |

---

## 六、工作小结 (Summary of Work)

### 6.1 知识与技能的提升

通过本次课程设计，我系统性地巩固并扩展了以下 C++ 核心知识与工程技能：

**C++ 核心语法：**
- **类与对象：** 深入理解了封装（private 成员+public 接口）、继承（IRenderer 抽象基类）、多态（虚函数动态绑定）三大面向对象特性在实战中的应用
- **STL 容器：** 熟练运用了 `std::vector`（二维网格）、`std::set`（棋子集合）、`std::map`（进度管理）等容器，理解了不同容器的适用场景和性能特征
- **智能指针：** 全面使用 `std::unique_ptr` 管理动态资源，掌握了独占所有权语义和 RAII（资源获取即初始化）思想，避免了手动 `new`/`delete` 带来的内存泄漏风险
- **文件流：** 使用 `std::fstream` + `std::filesystem` 实现了多格式文件的读写（CSV、键值对、自定义节点头格式）
- **C++17 新特性：** 使用了结构化绑定（`auto [r, c]`）、`std::optional`（AI 可选返回值）、`std::chrono`（游戏计时）、if 初始化语句等现代 C++ 特性

**工程规范：**
- **模块化设计：** MVC 分层架构使各模块职责清晰，Model 层零依赖 View 和 Controller
- **条件编译：** 使用 `#ifdef HAS_GUI` 实现同一套代码支持 Console/SFML 双模式，通过 CMake 选项编译控制
- **Git 版本控制：** 约 30+ 次 commit，每次聚焦一个功能或修复一个 bug
- **设计文档：** 编写了 ARCHITECTURE.md 架构文档和本设计报告，培养了文档化习惯

### 6.2 工程思维的建立

从"写单一功能的算法题"到"开发一个完整系统"的转变，我获得了以下重要认识：

**1. 架构设计先于编码。** 在写第一行代码之前，先确定 MVC 三层架构、定义好 IRenderer 接口、规划好 GamePhase 状态机，让后续开发有章可循。中途重构的成本远高于前期设计。

**2. 解耦带来灵活性。** IRenderer 接口的设计让双渲染模式成为可能，即使到开发后期新增 SFML 图形界面，也只需实现接口方法，无需修改 Controller 的任何逻辑。

**3. 问题的根因往往在表象之下。** 排行榜崩溃（段错误）表面像是内存问题，实际是数组越界访问垃圾指针。AI 无限循环看似是 AI 太强，实际是算法缺少终止条件。调试不只是修 bug，更是对代码逻辑的深度理解。

**4. 工程文档和代码同样重要。** 架构图、接口规格、问题记录不仅帮助自己理清思路，也是答辩和团队协作的基础。

**5. 软件工程不是"写代码"，而是"构建可持续演进的作品"。** 一个好的系统应该有清晰的边界（模块划分）、稳固的根基（数据结构和算法）和灵活的扩展点（接口和策略模式）。
