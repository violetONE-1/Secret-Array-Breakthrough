# 二、设计思路

## 2.1 系统功能模块划分 (System Functional Module Partition)

整个"密阵突围"游戏采用 **Model-View-Controller (MVC)** 架构，自上而下划分为 **四个主要功能模块**。各模块职责单一、边界清晰，通过接口契约实现松耦合通信。

### 模块架构总图

```
┌─────────────────────────────────────────────────────────────────────┐
│                    模块四：游戏流程控制模块 (Controller)                │
│            GameController / InputHandler / AIPlayer                  │
│  职责：主循环调度 → 接收输入 → 编排操作 → 驱动渲染 → 阶段切换          │
└──────┬─────────────────────────────┬────────────────────────────────┘
       │ 写入 / 调度                  │ 只读查询 + 渲染指令
       ▼                             ▼
┌──────────────────────┐  ┌──────────────────────────────────────────┐
│ 模块二：数据持久化模块 │  │       模块三：用户交互与渲染模块 (View)     │
│ (Model 存储子层)       │  │  IRenderer / ConsoleRenderer /          │
│ PuzzleManager         │  │  SFMLRenderer / InputHandler             │
│ Leaderboard           │  │ 职责：网格绘制 / 菜单与排行榜面板 /       │
│ FileManager           │  │       鼠标键盘事件捕获 / 像素坐标转换      │
│ ReplayBuffer          │  │                                          │
└──────────┬────────────┘  └──────────────────────────────────────────┘
           │ 上层依赖核心
           ▼
┌────────────────────────────────────────────────────────────────────┐
│                  模块一：游戏核心逻辑模块 (Model 核心层)               │
│          Cell / Grid / MergeRule / GameState / Move                 │
│  职责：25×25 网格数据表示 / 合并规则纯算法 / 一局状态快照 / 操作记录   │
└────────────────────────────────────────────────────────────────────┘
```

---

### 模块一：游戏核心逻辑模块 (Core Game Logic Module)

**所在 MVC 层：** Model 核心层

**功能概述：**
本模块是整个游戏的"大脑"，独立于任何界面和输入方式，包含全部游戏规则和状态管理逻辑。可脱离 GUI 独立编译和单元测试。

**具体功能：**

| 功能项 | 负责类 | 说明 |
|-------|-------|------|
| 格子数据建模 | `Cell` | 每个格子存储一个字母（A~Z）和一个数字（0~9），以及空格、选中两种状态位 |
| 网格容器管理 | `Grid` | 维护 rows×cols 的二维 Cell 数组（默认 25×25），提供创建、读写、查找合法操作、序列化等接口 |
| 合并规则判定 | `MergeRule` | 纯静态算法类：判断两格是否满足合并条件；执行字母相加（循环进位）与数字相加（取个位） |
| 游戏状态聚合 | `GameState` | 聚合当前盘面、已走步数、计时的起始点、操作历史，并提供胜负判断与正确率计算 |
| 操作原子记录 | `Move` | 记录单步操作的源/目标坐标、方向与合并结果，供回放和统计使用 |

**对外接口摘要：**
- `Grid::at(row, col)` — 获取指定格子
- `Grid::mergeCells(src, dst)` — 执行合并并更新盘面
- `Grid::hasAnyValidMove()` — 判断是否还存在可走之路
- `MergeRule::canMerge(a, b)` — 静态校验合并合法性
- `MergeRule::getMergedCell(a, b)` — 静态计算合并结果
- `GameState::recordMove(move)` — 记录一步操作

---

### 模块二：数据持久化与存储模块 (Data Persistence & Storage Module)

**所在 MVC 层：** Model 存储子层

**功能概述：**
本模块负责程序中全部数据的"落地"与"加载"。所有外部文件使用纯文本格式（`.txt`），通过 `FileManager` 统一封装底层 I/O，不引入任何数据库或第三方序列化库。

**具体功能：**

| 功能项 | 负责类 | 说明 |
|-------|-------|------|
| 题面管理 | `PuzzleManager` | 加载 ≥ 3 道预置题面；支持用户创建新题面并保存为文本文件；按 ID 检索题面 |
| 答题记录 | `ScoreRecord` | 封装单次答题的全部成绩字段（用时、步数、正确率、综合得分、时间戳） |
| 排行榜 | `Leaderboard` | 维护得分记录列表；支持按总分、用时、正确率三维度排序；启动时加载、提交时追加并即时保存到 `leaderboard.txt` |
| 回放录制 | `ReplayBuffer` | 录制整局 Move 序列；导出为带元信息的回放文本文件；从文件加载并逐步回放 |
| 文件 I/O 基础 | `FileManager` | 提供统一的 `readTextFile` / `writeTextFile` / `appendLine` / `fileExists` 接口，屏蔽 `fstream` 细节 |

**对外接口摘要：**
- `PuzzleManager::getAllPuzzles()` — 获取全部题面列表
- `PuzzleManager::getPuzzleById(id)` — 按 ID 获取题面
- `Leaderboard::addRecord(record)` — 追加一条记录并保存
- `Leaderboard::getTopByScore(n)` — 获取按总分排序的前 n 名
- `ReplayBuffer::saveToFile(path)` — 导出回放文件
- `ReplayBuffer::loadFromFile(path)` — 加载回放文件

---

### 模块三：用户交互与渲染模块 (User Interaction & Rendering Module)

**所在 MVC 层：** View

**功能概述：**
本模块将游戏状态"可视化"为网格界面并捕获用户输入。通过定义 `IRenderer` 抽象接口，实现了**控制台 ASCII 模式**和 **SFML 图形窗口模式**两种渲染实现的自由切换，Controller 层只需面向接口编程。

**具体功能：**

| 功能项 | 负责类 | 说明 |
|-------|-------|------|
| 抽象接口定义 | `IRenderer` | 声明纯虚函数 `render(GameState)`、`showMenu()`、`showLeaderboard()`、`handleEvents()` 等，所有渲染器必须实现 |
| 控制台绘制 | `ConsoleRenderer` | 用制表符和颜色码在 Windows 控制台绘制 25×25 网格；用 `_getch()` 捕获键盘方向键/回车 |
| GUI 绘制 | `SFMLRenderer` | 创建 SFML 窗口，用 `sf::RectangleShape` 绘制彩色格子，用 `sf::Text` 渲染字母数字，处理 `sf::Event` |
| 输入坐标转换 | `InputHandler` | 将鼠标点击的像素坐标转换为网格（row, col）；判断点击是否落在网格有效区域内 |

**对外接口摘要：**
- `IRenderer::render(const GameState&)` — 纯虚函数，绘制当前帧
- `IRenderer::handleEvents()` — 处理窗口/键盘事件
- `InputHandler::convertPixelToGrid(px, py)` — 像素坐标 → 网格坐标

---

### 模块四：游戏流程控制模块 (Game Flow Control Module)

**所在 MVC 层：** Controller

**功能概述：**
本模块是游戏的"神经系统"，负责编排整个游戏生命周期。它持有全部 Model 对象的指针和渲染器实例，按照固定流程（菜单 → 选题 → 游戏 → 结算 → 排行榜）调度各模块协同工作。

**具体功能：**

| 功能项 | 负责类 | 说明 |
|-------|-------|------|
| 主循环调度 | `GameController` | 初始化环境 → 进入事件-更新-绘制循环 → 管理阶段切换 → 程序退出 |
| 玩家身份管理 | `Player` | 记录当前玩家名称和选择的 5 个起始格子 |
| AI 自动操作 | `AIPlayer` | 扫描全盘合法操作，按策略（随机/贪心/搜索）选择并执行合并 |

**关键状态机转换：**

```
 MAIN_MENU  ──"开始游戏"──►  PUZZLE_SELECT  ──"选题确认"──►  GAMEPLAY
    ▲                           ▲                                │
    │                           │                                │
    └──"返回"──  LEADERBOARD ◄──┘              ┌──"提交答案"───┘
                  ▲      │                      │
                  │      └──"查看排行榜"────────┘
                  │                               │
                  └──"返回主菜单"── RESULT ◄──────┘
```

### 模块协同工作流程（以一次合并操作为例）

```
1. 用户点击格子
       │
       ▼
2. InputHandler::convertPixelToGrid(x, y) → (row, col)
   SFMLRenderer / ConsoleRenderer 根据 (row, col) 更新高亮
       │
       ▼
3. 用户再次选择方向或相邻格子
   GameController 获取 src (row, col) 和 dst (row, col)
       │
       ▼
4. GameController 调用 MergeRule::canMerge(srcCell, dstCell)
   ├── false → 通知 Renderer 显示"不可合并"提示
   └── true  → 继续执行
       │
       ▼
5. GameController 调用 Grid::mergeCells(src, dst)
   → Grid 内部更新 Cell 数组
       │
       ▼
6. GameController 调用：
   - GameState::recordMove(Move{...})     // 记录操作
   - ReplayBuffer::recordMove(Move{...})  // 录制回放
       │
       ▼
7. GameController 调用 GameState::checkGameOver()
   ├── 未结束 → 调用 IRenderer::render(GameState) 重绘
   └── 已结束 → 调用 submitAnswer() → 计算成绩 → 更新排行榜
```

---

## 2.2 类体系设计 (Class Hierarchy Design)

系统共包含 **18 个核心类**。除 `IRenderer` 作为抽象基类被继承外，其余类之间均通过**组合 (Composition)** 或**关联 (Association)** 建立关系，不涉及复杂的多重继承。

### 2.2.1 继承关系总览

```
IRenderer  (抽象接口, 定义纯虚函数)
    ▲
    ├── public 继承 ── ConsoleRenderer    (终端 ASCII 实现)
    └── public 继承 ── SFMLRenderer       (SFML GUI 实现, Lv.2+)
```

`MergeRule` 为全静态方法类，不可实例化，亦不参与继承。

---

### 2.2.2 游戏核心逻辑模块 — 类详细设计

#### Cell（格子类）

表示网格中的一个单元格，是最小的数据单元。

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_letter` | `char` | 格子的字母（'A'~'Z'） |
| 数据成员 | `_number` | `int` | 格子的数字（0~9） |
| 数据成员 | `_empty` | `bool` | 该格是否已被合并清空（true 时不显示内容） |
| 数据成员 | `_selected` | `bool` | 该格是否被玩家当前选中（渲染时加高亮边框） |
| 成员函数 | `Cell(char letter, int number)` | 构造函数，初始化有内容的格子 |
| 成员函数 | `static Cell makeEmpty()` | 静态工厂方法，返回一个空格子 |
| 成员函数 | `getLetter() / getNumber()` | 读取字母和数字 |
| 成员函数 | `setLetter(char) / setNumber(int)` | 修改字母和数字 |
| 成员函数 | `isEmpty() / setEmpty(bool)` | 空格状态的 getter / setter |
| 成员函数 | `isSelected() / setSelected(bool)` | 选中状态的 getter / setter |
| 成员函数 | `sameLetter(const Cell&) const → bool` | 判断另一个格子与本格字母是否相同 |
| 成员函数 | `sameNumber(const Cell&) const → bool` | 判断另一个格子与本格数字是否相同 |
| 成员函数 | `toString() const → std::string` | 返回显示字符串："A1"（正常）或 " . "（空格） |
| 成员函数 | `operator==(const Cell&) const` | 判断两格是否完全相同 |

---

#### Grid（网格类）

管理 rows×cols 的二维 Cell 数组，提供网格级操作。

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_rows` | `int` | 网格行数（默认 25） |
| 数据成员 | `_cols` | `int` | 网格列数（默认 25） |
| 数据成员 | `_cells` | `std::vector<std::vector<Cell>>` | 二维格子数组，所有 Cell 按值存储 |
| 成员函数 | `Grid(int rows, int cols)` | 构造函数，创建全空格网格 |
| 成员函数 | `rows() / cols() const → int` | 获取行数 / 列数 |
| 成员函数 | `at(int r, int c) → Cell&` | 获取指定坐标的格子引用（带 `std::out_of_range` 检测） |
| 成员函数 | `setAt(int r, int c, const Cell&)` | 设置指定坐标的格子 |
| 成员函数 | `mergeCells(int sr, int sc, int dr, int dc)` | 合并两格：目标格 dr,dc 更新为合并结果，源格 sr,sc 置空 |
| 成员函数 | `findAllValidMoves() const → std::vector<Move>` | 遍历全盘，返回所有（源坐标, 目标坐标, 方向, 结果）的合法操作列表 |
| 成员函数 | `hasAnyValidMove() const → bool` | 判断当前盘面是否还存在至少一个合法合并操作 |
| 成员函数 | `serialize() const → std::string` | 将网格序列化为"行数列数 + 所有格子字母数字"的文本格式 |
| 成员函数 | `static deserialize(const std::string&) → Grid` | 从序列化文本重建网格对象 |
| 成员函数 | `reset()` | 将全部格子重置为空状态 |

---

#### MergeRule（合并规则类 — 纯静态）

封装"密阵突围"的全部合并规则算法，无任何内部状态，所有方法均为 `static`。

| 类别 | 名称 | 说明 |
|------|------|------|
| 数据成员 | （无） | 纯静态工具类，不可实例化（构造函数 = delete） |
| 静态方法 | `canMerge(const Cell& a, const Cell& b) → bool` | 判断 a 与 b 是否满足合并条件（相邻 + 字母或数字有一项相同且另一项不同） |
| 静态方法 | `mergeLetters(char a, char b) → char` | 字母相加：'A'+'B'→'C'，'Z'+'A'→'A'（循环进位，取 `(a-'A' + b-'A') % 26 + 'A'`） |
| 静态方法 | `mergeNumbers(int a, int b) → int` | 数字相加：1+2→3，9+8→7（取个位，`(a + b) % 10`） |
| 静态方法 | `getMergedCell(const Cell& a, const Cell& b) → Cell` | 返回合并后的新 Cell：字母不同则相加字母，数字不同则相加数字，都相同则不变 |

**合并规则判定逻辑：**

```
canMerge(Cell a, Cell b):
    if a 为空 or b 为空           → return false
    if a与b字母相同 AND 数字不同   → return true   (合并后数字相加取个位)
    if a与b数字相同 AND 字母不同   → return true   (合并后字母相加循环进位)
    if a与b字母相同 AND 数字也相同 → return false  (合并后不变，无意义操作)
    return false                  (字母数字均不同，不可合并)
```

---

#### Move（操作记录类）

记录单步合并操作的完整信息，用于回放序列化和正确率计算。

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_srcRow`, `_srcCol` | `int` | 被移动格子（邻格）的行列坐标 |
| 数据成员 | `_dstRow`, `_dstCol` | `int` | 中心格子（目标格）的行列坐标 |
| 数据成员 | `_direction` | `Direction` | 移动方向枚举（UP / DOWN / LEFT / RIGHT） |
| 数据成员 | `_resultLetter` | `char` | 合并后的字母 |
| 数据成员 | `_resultNumber` | `int` | 合并后的数字 |
| 成员函数 | `Move(sr, sc, dr, dc, dir, rl, rn)` | 构造函数，传入全部字段 |
| 成员函数 | `serialize() const → std::string` | 序列化为回放文件格式："[Move] 001 3,4 -> 3,5 RIGHT A5" |
| 成员函数 | `static deserialize(const std::string&) → Move` | 从回放文件文本行解析出 Move 对象 |

---

#### GameState（游戏状态类）

聚合一局游戏的瞬时状态，是 Controller 查询和修改 Model 的统一入口。

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_grid` | `Grid` | 当前盘面（组合关系，值语义） |
| 数据成员 | `_puzzleId` | `std::string` | 当前题面的唯一 ID |
| 数据成员 | `_playerStarts` | `std::vector<std::pair<int,int>>` | 玩家选择的 5 个起始格子坐标 |
| 数据成员 | `_stepsTaken` | `int` | 已执行的合并步数（初始 0） |
| 数据成员 | `_startTime` | `std::chrono::steady_clock::time_point` | 游戏开始时间点 |
| 数据成员 | `_moveHistory` | `std::vector<Move>` | 操作历史序列 |
| 数据成员 | `_gameOver` | `bool` | 游戏是否已结束 |
| 成员函数 | `GameState(Grid, starts, puzzleId)` | 构造函数 |
| 成员函数 | `grid() const → const Grid&` | 获取当前盘面的只读引用 |
| 成员函数 | `grid() → Grid&` | 获取当前盘面的可写引用 |
| 成员函数 | `recordMove(const Move&)` | 记录一步操作，`_stepsTaken` 自增 |
| 成员函数 | `elapsedSeconds() const → double` | 返回从首次操作到当前的已用秒数 |
| 成员函数 | `calculateAccuracy() const → double` | 正确步数 / 总步数（范围 0.0~1.0） |
| 成员函数 | `isOver() const → bool` | 返回游戏是否已结束 |
| 成员函数 | `endGame()` | 标记游戏结束，停止计时 |

---

### 2.2.3 数据持久化与存储模块 — 类详细设计

#### Puzzle（题面类）

封装一道题目的全部数据。

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_id` | `std::string` | 题面唯一标识符（如 "builtin_01"） |
| 数据成员 | `_name` | `std::string` | 题面显示名称（如 "初级训练题 1"） |
| 数据成员 | `_gridSize` | `int` | 网格尺寸（如 25） |
| 数据成员 | `_initialGrid` | `Grid` | 初始网格布局 |
| 成员函数 | `Puzzle(...)` | 构造函数 |
| 成员函数 | `id() / name() / gridSize() const` | 元信息 getter |
| 成员函数 | `initialGrid() const → const Grid&` | 获取初始盘面只读引用 |
| 成员函数 | `serialize() const → std::string` | 序列化为题面文件内容 |
| 成员函数 | `static deserialize(const std::string&) → Puzzle` | 从文件内容解析题面 |

---

#### PuzzleManager（题库管理类）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_puzzles` | `std::vector<Puzzle>` | 已加载的全部题面 |
| 数据成员 | `_puzzleDir` | `std::string` | 题面文件存放目录路径 |
| 成员函数 | `PuzzleManager(const std::string& dir)` | 构造函数 |
| 成员函数 | `loadBuiltin()` | 加载预置题面（硬编码 + 从 data/puzzles/ 读取） |
| 成员函数 | `loadCustom()` | 加载 data/puzzles/custom/ 下用户题面 |
| 成员函数 | `allPuzzles() const → const std::vector<Puzzle>&` | 返回全部题面列表 |
| 成员函数 | `findById(id) const → const Puzzle*` | 按 ID 查找，找不到返回 nullptr |
| 成员函数 | `add(const Puzzle&)` | 添加新题面（内存中） |
| 成员函数 | `saveToFile(const Puzzle&) → bool` | 将题面持久化到文本文件 |

---

#### ScoreRecord（答题记录类）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_playerName` | `std::string` | 玩家名称 |
| 数据成员 | `_puzzleId` | `std::string` | 题面 ID |
| 数据成员 | `_timeSeconds` | `double` | 答题用时（秒） |
| 数据成员 | `_steps` | `int` | 完成步数 |
| 数据成员 | `_accuracy` | `double` | 操作正确率（0.0~1.0） |
| 数据成员 | `_score` | `int` | 综合得分（由 calculateScore 自动计算） |
| 数据成员 | `_timestamp` | `std::string` | 答题完成时间戳 |
| 成员函数 | `ScoreRecord(...)` | 构造函数，自动调用 `calculateScore()` |
| 成员函数 | `calculateScore() → int` | 计算公式：`accuracy×1000 + max(0,500-time×2) + steps×10` |
| 成员函数 | `serialize() const → std::string` | 序列化为 CSV 风格一行 |
| 成员函数 | `static deserialize(const std::string&) → ScoreRecord` | 从 CSV 行反序列化 |

---

#### Leaderboard（排行榜类）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_records` | `std::vector<ScoreRecord>` | 全部排行榜记录 |
| 数据成员 | `_filePath` | `std::string` | 排行榜数据文件路径 |
| 成员函数 | `Leaderboard(const std::string& path)` | 构造函数 |
| 成员函数 | `load() → bool` | 从文件加载记录 |
| 成员函数 | `save() → bool` | 将记录写入文件 |
| 成员函数 | `add(const ScoreRecord&)` | 追加记录（内存 + 文件追加一行） |
| 成员函数 | `topByScore(int n) → std::vector<ScoreRecord>` | 按总分降序，取前 n 名 |
| 成员函数 | `topByTime(int n) → std::vector<ScoreRecord>` | 按用时升序，取前 n 名 |
| 成员函数 | `topByAccuracy(int n) → std::vector<ScoreRecord>` | 按正确率降序，取前 n 名 |
| 成员函数 | `recordsForPuzzle(const std::string& id)` | 筛选特定题面的全部记录 |

---

#### ReplayBuffer（回放缓冲区类）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_puzzleId` | `std::string` | 对应的题面 ID |
| 数据成员 | `_startTime` | `std::string` | 录制开始时间戳 |
| 数据成员 | `_moves` | `std::vector<Move>` | 操作序列 |
| 数据成员 | `_recording` | `bool` | 是否正在录制 |
| 成员函数 | `ReplayBuffer(const std::string& puzzleId)` | 构造函数 |
| 成员函数 | `start() / stop()` | 开始 / 停止录制 |
| 成员函数 | `record(const Move&)` | 录制一步操作 |
| 成员函数 | `saveToFile(const std::string& path) → bool` | 导出回放文件（含元信息头部 + Move 序列） |
| 成员函数 | `loadFromFile(const std::string& path) → bool` | 从回放文件加载操作序列 |
| 成员函数 | `moves() const → const std::vector<Move>&` | 获取操作序列（供回放模式使用） |

---

#### FileManager（文件管理类）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_dataRoot` | `std::string` | 数据目录根路径 |
| 成员函数 | `FileManager(const std::string& root)` | 构造函数 |
| 成员函数 | `readAll(const std::string& relativePath) → std::string` | 读取文本文件全部内容并返回 |
| 成员函数 | `writeAll(const std::string& relativePath, const std::string& content) → bool` | 将内容写入文本文件（覆盖） |
| 成员函数 | `appendLine(const std::string& relativePath, const std::string& line) → bool` | 向文件末尾追加一行 |
| 成员函数 | `exists(const std::string& relativePath) → bool` | 检查文件是否存在 |
| 成员函数 | `createDir(const std::string& relativePath) → bool` | 创建目录 |

---

### 2.2.4 用户交互与渲染模块 — 类详细设计

#### IRenderer（渲染器抽象接口）

| 类别 | 名称 | 说明 |
|------|------|------|
| 数据成员 | （无） | 纯虚接口，不可实例化 |
| 纯虚函数 | `render(const GameState&) = 0` | 渲染整个游戏界面 |
| 纯虚函数 | `showMenu() = 0` | 显示主菜单 |
| 纯虚函数 | `showPuzzleList(const std::vector<Puzzle>&) = 0` | 显示题面选择列表 |
| 纯虚函数 | `showLeaderboard(const std::vector<ScoreRecord>&) = 0` | 显示排行榜 |
| 纯虚函数 | `showResult(const ScoreRecord&) = 0` | 显示答题结果 |
| 纯虚函数 | `showMessage(const std::string&) = 0` | 显示临时提示信息 |
| 纯虚函数 | `handleEvents() → UserAction = 0` | 处理输入事件并返回用户操作枚举 |
| 纯虚函数 | `isOpen() const → bool = 0` | 窗口 / 控制台是否仍然有效 |
| 虚析构函数 | `virtual ~IRenderer() = default` | 保证正确析构派生类 |

---

#### ConsoleRenderer（终端渲染器）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_cellWidth` | `int` | 每格显示宽度（默认 3，显示如 "A1 "） |
| 数据成员 | `_hConsole` | `HANDLE` | Windows 控制台句柄（用于设置光标和颜色） |
| 成员函数 | `ConsoleRenderer(int gridSize)` | 构造函数 |
| 成员函数 | `render(const GameState&)` | 清屏后用制表符绘制网格，空格子显示 " . "，选中格子加 `[]` 标记 |
| 成员函数 | `showMenu()` | 打印主菜单选项（1.选题 2.排行榜 3.退出） |
| 成员函数 | `showPuzzleList(...)` | 列出题面编号与名称 |
| 成员函数 | `showLeaderboard(...)` | 打印表格形式的排行榜 |
| 成员函数 | `showResult(...)` | 显示用时、步数、正确率、总分 |
| 成员函数 | `handleEvents() → UserAction` | 使用 `_getch()` 捕获方向键（0xE0 前缀）和回车 |

---

#### SFMLRenderer（SFML 图形渲染器 — Level 2+）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_window` | `sf::RenderWindow` | SFML 窗口 |
| 数据成员 | `_font` | `sf::Font` | 中文字体 |
| 数据成员 | `_cellSize` | `float` | 每个格子的像素尺寸（int 值从网格规模动态计算） |
| 数据成员 | `_gridOriginX`, `_gridOriginY` | `float` | 网格在窗口中的起始像素坐标 |
| 数据成员 | `_selectedCell` | `std::optional<std::pair<int,int>>` | 当前选中的格子坐标 |
| 成员函数 | `SFMLRenderer(int rows, int cols)` | 创建窗口，根据网格尺寸自适应窗口大小 |
| 成员函数 | `render(const GameState&)` | 遍历 Grid 中每个 Cell 绘制对应矩形 + 文字，高亮选中格和可合并邻格 |
| 成员函数 | `showMenu()` | 绘制带文字按钮的主菜单界面 |
| 成员函数 | `showLeaderboard(...)` | 绘制排行榜面板 |
| 成员函数 | `handleEvents() → UserAction` | 轮询 `sf::Event`，将鼠标点击/按键转换为 `UserAction` 枚举 |

---

### 2.2.5 游戏流程控制模块 — 类详细设计

#### GameController（主控制器）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_state` | `std::unique_ptr<GameState>` | 当前游戏状态（独占所有） |
| 数据成员 | `_puzzleManager` | `std::unique_ptr<PuzzleManager>` | 题库管理器 |
| 数据成员 | `_leaderboard` | `std::unique_ptr<Leaderboard>` | 排行榜 |
| 数据成员 | `_replayBuffer` | `std::unique_ptr<ReplayBuffer>` | 回放录制 |
| 数据成员 | `_renderer` | `std::unique_ptr<IRenderer>` | 渲染器（多态） |
| 数据成员 | `_inputHandler` | `std::unique_ptr<InputHandler>` | 输入处理器 |
| 数据成员 | `_player` | `Player` | 当前玩家 |
| 数据成员 | `_phase` | `GamePhase` | 当前阶段 (枚举：MENU / SELECT / PLAY / RESULT / LEADERBOARD / REPLAY) |
| 数据成员 | `_running` | `bool` | 主循环运行标志 |
| 成员函数 | `GameController()` | 构造函数：初始化所有子对象、加载题库、创建渲染器 |
| 成员函数 | `run()` | 启动主循环：`while(_running) { handleEvents(); update(); render(); }` |
| 成员函数 | `selectPuzzle(const std::string& id)` | 加载题面并初始化 GameState |
| 成员函数 | `executeMerge(int sr, int sc, int dr, int dc)` | 校验→合并→记录Move→录制→检查胜负 |
| 成员函数 | `submitAnswer()` | 计算成绩→写入排行榜→显示结果 |
| 成员函数 | `saveReplay()` | 调用 ReplayBuffer 导出回放文件 |
| 成员函数 | `loadReplay(const std::string& path)` | 加载回放文件，切换至 REPLAY 阶段 |

---

#### Player（玩家类）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_name` | `std::string` | 玩家名称 |
| 数据成员 | `_startingCells` | `std::vector<std::pair<int,int>>` | 选中的 5 个起始格子坐标 |
| 成员函数 | `Player(const std::string& name)` | 构造函数 |
| 成员函数 | `name() / setName()` | 名称的 getter / setter |
| 成员函数 | `startingCells() / setStartingCells()` | 起始格的 getter / setter |

---

#### AIPlayer（AI 玩家类 — Level 2+）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_difficulty` | `AIDifficulty` | 难度枚举（EASY / MEDIUM / HARD） |
| 成员函数 | `AIPlayer(AIDifficulty d)` | 构造函数 |
| 成员函数 | `selectMove(const Grid&) → std::optional<Move>` | 分析盘面，返回一个最佳操作（无合法操作时返回 nullopt） |
| 成员函数 | `allValidMoves(const Grid&) → std::vector<Move>` | 扫描全盘所有合法操作 |
| 成员函数 | `evaluate(const Grid&) → int` | 启发式评估盘面得分（空格越多分越高） |

**AI 策略简述：**

| 难度 | 策略 |
|------|------|
| EASY | 从 `allValidMoves()` 中随机选择 |
| MEDIUM | 贪心：选择合并后产生最大字母/数字值的操作 |
| HARD | 极小化极大搜索 + α-β 剪枝，预判 2~3 步 |

---

#### InputHandler（输入处理类）

| 类别 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 数据成员 | `_cellSize` | `float` | 每格像素尺寸（GUI 模式下用于坐标转换） |
| 数据成员 | `_gridX`, `_gridY` | `float` | 网格在窗口中的起始像素位置 |
| 成员函数 | `convertPixelToGrid(int px, int py) → std::pair<int,int>` | 像素坐标 → 网格 (row, col) |
| 成员函数 | `isInGrid(int px, int py) → bool` | 判断像素坐标是否落在网格区域内 |

---

### 2.2.6 类间关系总结

```
继承关系 (Inheritance):
  IRenderer  (抽象基类, 定义纯虚接口)
      ▲ public
      ├── ConsoleRenderer    (终端 ASCII 实现)
      └── SFMLRenderer       (SFML GUI 实现)

组合关系 (Composition — 值语义, 生命周期绑定):
  Grid         ◆── std::vector<std::vector<Cell>>    (Grid 拥有全部 Cell)
  GameState    ◆── Grid                              (GameState 拥有盘面)
  GameState    ◆── std::vector<Move>                 (GameState 拥有操作历史)
  Leaderboard  ◆── std::vector<ScoreRecord>           (Leaderboard 拥有全部记录)
  ReplayBuffer ◆── std::vector<Move>                  (ReplayBuffer 拥有操作序列)
  PuzzleManager◆── std::vector<Puzzle>                (PuzzleManager 拥有题面列表)

独占所有权 (unique_ptr, 组合的指针形式):
  GameController ◆── GameState, PuzzleManager, Leaderboard,
                     ReplayBuffer, IRenderer, InputHandler
                     (全部通过 unique_ptr 独占)

关联关系 (Association — 无所有权, 仅引用 / 调用):
  GameController ──► 调用 MergeRule::canMerge()      (静态调用)
  GameController ──► 持有 GameState*                  (通过 unique_ptr 间接持有)
  IRenderer      ──► 读取 const GameState&            (只读渲染)
  AIPlayer       ──► 读取 const Grid&                 (只读分析)
```

---

## 2.3 数据文件设计 (Data File Design)

### 2.3.1 持久化需求分析

| 数据类型 | 触发存储的时机 | 触发加载的时机 | 文件路径 |
|---------|-------------|-------------|---------|
| 题面（Puzzle） | 用户新建题面时保存 | 程序启动时、选题时 | `data/puzzles/builtin_*.txt` 或 `data/puzzles/custom/*.txt` |
| 排行榜（Leaderboard） | 每次答题提交后追加 | 程序启动时、进入排行榜界面时 | `data/leaderboard.txt` |
| 回放记录（Replay） | 每局游戏结束时导出 | 用户选择回放模式时 | `data/replays/replay_YYYYMMDD_HHMMSS.txt` |

### 2.3.2 文件格式选择：纯文本 `.txt`

**选择理由：**

| 考量维度 | 说明 |
|---------|------|
| **实验硬性要求** | 题目明确规定"数据存储不允许使用数据库"，纯文本是最直接且合规的替代方案 |
| **人类可读** | 所有文件用记事本即可查看、编辑、调试，极大降低开发阶段的排错难度 |
| **零依赖** | 仅需 C++ 标准库 `<fstream>` 和 `<sstream>`，无需引入 JSON/XML 等第三方解析器 |
| **跨平台一致** | `.txt` 无字节序问题，Windows / Linux 下行为完全一致 |
| **Git 友好** | 文本文件支持 diff，方便追踪数据变更和提交实验报告 |
| **增量写入** | 排行榜文件采用行追加模式，新记录直接 `appendLine`，无需重写整个文件 |
| **符合千行代码要求** | 自编序列化/反序列化逻辑计入代码行数，而非依赖第三方库 |

> **为什么不选 JSON / XML / 二进制：** JSON 虽结构清晰，但需要引入 `nlohmann/json` 之类第三方库（增加非自编代码）；XML 标签冗长、解析逻辑繁琐；二进制格式不可读、不同编译器间 `struct` 内存布局可能不一致。

### 2.3.3 数据文件格式定义与示例

---

#### (1) 题面文件 — `data/puzzles/builtin_01.txt`

```
# 密阵突围 题面文件
# 格式: 首行 行数 列数, 后续为 row×col 个格子(空格分隔), 最后为起点列表
25 25
A1 B2 C3 D4 E5 F6 G7 H8 I9 J0 K1 L2 M3 N4 O5 P6 Q7 R8 S9 T0 U1 V2 W3 X4 Y5
A6 B7 C8 D9 E0 F1 G2 H3 I4 J5 K6 L7 M8 N9 O0 P1 Q2 R3 S4 T5 U6 V7 W8 X9 Y0
A2 B3 C4 D5 E6 F7 G8 H9 I0 J1 K2 L3 M4 N5 O6 P7 Q8 R9 S0 T1 U2 V3 W4 X5 Y6
...（共 25 行，每行 25 个格子）
5
0,0 2,3 6,10 12,15 20,20
```

**字段说明：**
| 行 | 内容 | 格式 |
|----|------|------|
| 1 | 注释 | `#` 开头，解析时跳过 |
| 2 | 网格尺寸 | `行数 列数` |
| 3~27 | 格子数据 | 每行 25 个格子，每个格子为"大写字母 + 一位数字"（如 "A1"），空格分隔 |
| 28 | 起点数量 | 整数（默认 5） |
| 29 | 起点坐标 | `行,列` 对，空格分隔 |

---

#### (2) 排行榜文件 — `data/leaderboard.txt`

```
# 密阵突围 排行榜
# 格式: 玩家名, 题面ID, 用时(秒), 步数, 正确率, 总分, 完成时间
Alice, builtin_01, 45.23, 12, 1.00, 980, 2026-05-15 14:23:10
Bob,   builtin_01, 52.10, 15, 0.93, 870, 2026-05-15 14:30:45
Alice, builtin_02, 38.75,  9, 1.00,1020, 2026-05-15 15:01:22
Charlie,builtin_01,61.80, 18, 0.89, 750, 2026-05-15 15:12:33
```

**字段说明（每行一条记录，逗号分隔）：**

| 字段 | 类型 | 示例 | 说明 |
|------|------|------|------|
| `player_name` | `string` | Alice | 玩家名称（不可含逗号） |
| `puzzle_id` | `string` | builtin_01 | 对应题面的唯一 ID |
| `time_seconds` | `double` | 45.23 | 答题用时，保留两位小数 |
| `steps` | `int` | 12 | 完成的合并步数 |
| `accuracy` | `double` | 1.00 | 正确率（正确步数 / 总步数） |
| `score` | `int` | 980 | 综合得分 |
| `timestamp` | `string` | 2026-05-15 14:23:10 | 答题完成时间 |

**得分计算公式：**
```
score = accuracy × 1000 + max(0, 500 - time_seconds × 2) + steps × 10
```

> 设计理念：正确率权重最高（1000 分），速度快加分（最多 500），步数提供少量正反馈（每步 10 分）。

---

#### (3) 回放文件 — `data/replays/replay_20260515_142310.txt`

```
# 密阵突围 回放记录
[Puzzle] builtin_01
[Grid] 25 25
[Player] Alice
[Starts] 0,0 2,3 6,10 12,15 20,20
[Start] 2026-05-15 14:22:25
[End]   2026-05-15 14:23:10
[TotalSteps] 12
#
# 操作序列: 每行格式
# [Move] 序号 源行,源列 -> 目标行,目标列 方向 合并后字母数字
[Move] 001 3,4 -> 3,5 RIGHT A5
[Move] 002 3,5 -> 3,6 RIGHT C6
[Move] 003 6,10 -> 5,10 UP    H2
[Move] 004 5,10 -> 4,10 UP    I0
[Move] 005 12,15 -> 12,14 LEFT A8
[Move] 006 12,14 -> 12,13 LEFT G4
[Move] 007 0,0  -> 1,0  DOWN  B1
[Move] 008 1,0  -> 1,1  RIGHT C1
[Move] 009 1,1  -> 2,1  DOWN  F3
[Move] 010 2,1  -> 2,0  LEFT  E0
[Move] 011 2,0  -> 3,0  DOWN  H0
[Move] 012 20,20 -> 19,20 UP   N9
```

**文件结构说明：**

| 区域 | 格式 | 说明 |
|------|------|------|
| 元信息头部 | `[Key] Value` | 记录题面 ID、网格尺寸、玩家名、起始点、起止时间、总步数 |
| 注释行 | `#` 开头 | 格式说明，解析时跳过 |
| 操作序列 | `[Move] 序号 src -> dst 方向 结果` | 按时间顺序排列的每一步操作详情 |

**回放播放逻辑：**
1. 读取元信息头部，重建初始盘面
2. 按顺序逐条解析 `[Move]` 行
3. 以固定时间间隔（如 500ms/步）在网格上执行对应合并操作
4. 到达 `[End]` 后暂停，用户可按键退出回放模式
