# 密阵突围 (Matrix Breakthrough) — 项目架构文档

## 一、项目概述

| 项目 | 内容 |
|------|------|
| 项目名称 | 密阵突围 (Matrix Breakthrough) |
| 开发语言 | C++17 |
| 构建工具 | CMake 3.20+ |
| 可选依赖 | SFML 3 (Graphics, Window, System, Audio) |
| 架构模式 | Model-View-Controller (MVC) |
| 核心玩法 | 在 N×N 网格上通过合并字母+数字格子获得分数，基于《最强大脑》第13季同名赛题 |

### 构建命令

```bash
# 控制台模式（仅 ASCII 渲染）
cmake -B build -DENABLE_GUI=OFF
cmake --build build

# SFML 图形模式（含音效、动画、鼠标操作）
cmake -B build -DENABLE_GUI=ON
cmake --build build
```

---

## 二、总体架构 (MVC 三层)

```
┌─────────────────────────────────────────────────────────┐
│                    View 层（渲染）                        │
│  IRenderer (抽象接口)                                    │
│    ├── ConsoleRenderer (控制台 ASCII)                    │
│    └── SFMLRenderer (图形窗口 + 动画 + 音效)             │
│  SoundManager (音效管理)                                 │
├─────────────────────────────────────────────────────────┤
│                  Controller 层（逻辑控制）                 │
│  GameController (主控制器，状态机)                        │
│    ├── AIPlayer (AI 对手，贪心/随机策略)                  │
│    └── InputHandler (坐标映射)                           │
├─────────────────────────────────────────────────────────┤
│                   Model 层（数据模型）                     │
│  核心逻辑:                                               │
│    Cell → Grid → GameState                              │
│    MergeRule (纯静态工具类)                               │
│    Move (操作记录)                                       │
│  持久化:                                                 │
│    FileManager (文件 I/O 封装)                            │
│    Puzzle / PuzzleManager (题库)                         │
│    ScoreRecord / Leaderboard (排行榜)                    │
│    ReplayBuffer (回放录制/回放)                           │
│    Progress (闯关进度)                                   │
│    Player (玩家信息)                                     │
└─────────────────────────────────────────────────────────┘
```

---

## 三、目录结构

```
E:\Matrix breakthrough/
├── CMakeLists.txt                 # 构建配置
├── ARCHITECTURE.md                # 本架构文档
├── include/                        # 全部头文件 (.hpp)
│   ├── core/                       # 核心游戏逻辑
│   │   ├── Cell.hpp                # 格子单元
│   │   ├── Grid.hpp                # 网格容器
│   │   ├── GameState.hpp           # 游戏运行时状态
│   │   ├── MergeRule.hpp           # 合并规则（静态工具类）
│   │   └── Move.hpp                # 操作记录结构体 + 枚举
│   ├── model/                      # 数据持久化
│   │   ├── FileManager.hpp         # 文件 I/O 封装
│   │   ├── Player.hpp              # 玩家信息
│   │   ├── Puzzle.hpp              # 题面
│   │   ├── PuzzleManager.hpp       # 题库管理
│   │   ├── ScoreRecord.hpp         # 得分记录
│   │   ├── Leaderboard.hpp         # 排行榜
│   │   ├── Progress.hpp            # 闯关进度
│   │   └── ReplayBuffer.hpp        # 回放录制
│   ├── controller/                 # 控制器
│   │   ├── GameController.hpp      # 主控制器（状态机）
│   │   ├── AIPlayer.hpp            # AI 对手
│   │   └── InputHandler.hpp        # 输入坐标映射
│   └── view/                       # 渲染层
│       ├── IRenderer.hpp           # 渲染器抽象接口
│       ├── ConsoleRenderer.hpp     # 控制台渲染器
│       ├── SFMLRenderer.hpp        # SFML 图形渲染器
│       └── SoundManager.hpp        # 音效管理器
├── src/                            # 全部源文件 (.cpp)
│   ├── main.cpp                    # 程序入口
│   ├── core/                       # 核心逻辑实现
│   │   ├── Cell.cpp
│   │   ├── Grid.cpp
│   │   ├── GameState.cpp
│   │   ├── MergeRule.cpp
│   │   └── Move.cpp
│   ├── model/                      # 持久化实现
│   │   ├── FileManager.cpp
│   │   ├── Player.cpp
│   │   ├── Puzzle.cpp
│   │   ├── PuzzleManager.cpp
│   │   ├── ScoreRecord.cpp
│   │   ├── Leaderboard.cpp
│   │   ├── Progress.cpp
│   │   └── ReplayBuffer.cpp
│   ├── controller/                 # 控制器实现
│   │   ├── GameController.cpp      # ~1225 行，工程最大文件
│   │   ├── AIPlayer.cpp
│   │   └── InputHandler.cpp
│   └── view/                       # 渲染实现
│       ├── ConsoleRenderer.cpp
│       ├── SFMLRenderer.cpp        # ~1900 行，最长文件
│       └── SoundManager.cpp
├── assets/
│   ├── fonts/NotoSansSC-Regular.otf   # SFML 字体
│   └── sounds/{merge,click,error,submit,result}.wav  # 音效
└── data/
    ├── puzzles/                     # 题面文件
    │   ├── builtin_01.txt ~ 03.txt
    │   └── custom/                  # 自定义题面目录
    ├── replays/                     # 回放文件存储
    ├── leaderboard.txt              # 排行榜持久化
    └── progress.txt                 # 闯关进度持久化
```

---

## 四、核心模块详解

### 4.1 Core 层 — 游戏核心逻辑

#### Cell (格子单元) — `include/core/Cell.hpp` / `src/core/Cell.cpp`

最基础的数据单元，表示网格中的一个格子。

| 成员 | 说明 |
|------|------|
| `char _letter` | 字母 A-Z |
| `int _number` | 数字 0-9 |
| `bool _empty` | 是否已被合并消耗 |
| `bool _selected` | UI 选中状态 |
| `Cell::makeEmpty()` | 静态工厂，创建空格子 |

#### Grid (网格) — `include/core/Grid.hpp` / `src/core/Grid.cpp`

使用 `std::vector<std::vector<Cell>>` 管理二维网格，默认 25×25。

| 方法 | 说明 |
|------|------|
| `at(r,c)` | 带边界检查的访问 |
| `mergeCells(src, dst)` | 合并：源置空，目标更新为合并结果 |
| `slideCell(src, dst)` | 滑行：源复制到目标，源清空 |
| `findAllValidMoves()` | 全盘扫描所有可合并对 |
| `hasAnyValidMove()` | 快捷判断是否还有可合并操作 |
| `serialize()` / `deserialize()` | 文本序列化，用于题面文件 I/O |

#### MergeRule (合并规则) — `include/core/MergeRule.hpp` / `src/core/MergeRule.cpp`

**纯静态工具类**，构造函数被 `= delete`，只通过静态方法调用。

```
合并规则核心逻辑:
  - 字母相同，数字不同 → 数字相加 mod 10，字母不变
    eg. A3 + A7 → A0   (3+7=10→0)
  - 数字相同，字母不同 → 字母相加 mod 26，数字不变
    eg. A3 + C3 → D3   (A=1, C=3, 1+3=4→D)
  - 字母数字全相同 → 允许但结果不变 (由 GameState 处理)
```

| 方法 | 说明 |
|------|------|
| `canMerge(a, b)` | 判断两格能否合并 |
| `mergeLetters(a, b)` | 字母相加 mod 26 |
| `mergeNumbers(a, b)` | 数字相加 mod 10 |
| `getMergedCell(a, b)` | 返回合并结果 Cell |

#### Move (操作记录) — `include/core/Move.hpp` / `src/core/Move.cpp`

记录一步操作的轻量值对象。

| 枚举 | 值 |
|------|-----|
| `Direction` | UP, DOWN, LEFT, RIGHT, NONE |
| `MoveType` | MERGE（合并）, SLIDE（滑行） |

`Move` 结构体包含：源坐标、目标坐标、方向、操作类型、结果字母和数字。
`serialize()` 输出格式：`[Move] 001 3,4 -> 3,5 RIGHT A5`

#### GameState (游戏状态) — `include/core/GameState.hpp` / `src/core/GameState.cpp`

聚合一次游戏的全部运行时状态，是整个游戏的核心数据对象。

| 成员/方法 | 说明 |
|-----------|------|
| `Grid _grid` | 当前棋盘 |
| `_playerCells`, `_aiCells` | `std::set<pair<int,int>>` 追踪双方棋子位置 |
| `moveOwnCell(owner, src, dst)` | 滑行时更新归属集合 |
| `conquerCell(owner, src, dst)` | 合并时，目标被攻击方占领 |
| `selfMerge(owner, src, dst)` | 合并己方两格，源从集合移除 |
| `hasValidMoves(owner)` | **核心方法**：从己方棋子出发 BFS，探索所有可达空格，检查能否与邻居合并 |
| `recordMove(move, owner)` | 记录操作到历史 |
| `stepsTakenBy(owner)` | 统计某一方步数 |
| `_elapsed` | `std::chrono::steady_clock` 计时 |

---

### 4.2 Model 层 — 数据持久化

#### FileManager — `include/model/FileManager.hpp` / `src/model/FileManager.cpp`

所有文件操作的底层封装，构造函数指定数据根目录（"data"）。

| 方法 | 说明 |
|------|------|
| `readAll(path)` | 读取完整文件内容 |
| `writeAll(path, content)` | 覆写文件 |
| `appendLine(path, line)` | 追加一行 |
| `exists(path)` | 检查文件是否存在 |
| `createDir(path)` | 递归创建目录 |

#### Puzzle / PuzzleManager — `include/model/Puzzle*.hpp` / `src/model/Puzzle*.cpp`

题库系统。

| 组件 | 说明 |
|------|------|
| `Puzzle` | 单一题面：ID + 名称 + 初始 Grid |
| `PuzzleManager` | 管理多个 Puzzle |
| `createBuiltinPuzzles()` | 用确定性随机种子(42/1042/2042)生成 3 道初始题面，保证每次构建相同 |
| `loadPuzzlesFromDir()` | 从 `data/puzzles/` 和 `data/puzzles/custom/` 加载自定义题面 |
| 连通性保证 | 生成后全盘扫描，确保每格至少有一个可合并邻居 |

#### ScoreRecord / Leaderboard — `include/model/ScoreRecord.hpp` / `include/model/Leaderboard.hpp`

得分与排行榜系统。

```
得分公式: max(0, 500 - timeSeconds×2) + steps×10
VS AI 模式: steps×10 (不计时)
```

| 组件 | 说明 |
|------|------|
| `ScoreRecord` | 单次成绩：玩家名、题面ID、用时、步数、得分、时间戳 |
| `Leaderboard` | 排行榜容器，支持 `topByScore(n)` 和 `topByTime(n)` 排序查询 |
| 持久化 | CSV 格式，`#` 开头为注释行 |

#### Progress — `include/model/Progress.hpp` / `src/model/Progress.cpp`

闯关进度管理。

| 功能 | 说明 |
|------|------|
| 关卡解锁 | 初始解锁第 1 关，通关后自动解锁下一关 |
| 分数线 | 第 1 关 ≥10 分，第 2 关 ≥300 分，第 3 关 ≥600 分 |
| 最高分记录 | 每关记录历史最高分 |

#### ReplayBuffer — `include/model/ReplayBuffer.hpp` / `src/model/ReplayBuffer.cpp`

游戏回放系统。

| 阶段 | 操作 |
|------|------|
| `startRecording()` | 记录题面、网格大小、玩家名、起始坐标、时间戳 |
| `record(move)` | 每走一步记录一个 Move |
| `saveToFile()` | 写入结构化回放文件，含 `[Puzzle]`, `[Grid]`, `[Player]`, `[Move]` 等节点头 |
| `loadFromFile()` | 解析回放文件，还原完整操作序列 |

---

### 4.3 Controller 层 — 游戏逻辑控制

#### GameController — `include/controller/GameController.hpp` / `src/controller/GameController.cpp`

**工程中枢**，约 1225 行。持有全部 Model 和 View 对象（unique_ptr），通过 9 阶段状态机驱动游戏。

##### 状态机

```
MENU → PUZZLE_SELECT → GAMEPLAY → RESULT → MENU
                    → LEADERBOARD → MENU
       VS_AI_MENU → VS_AI_WATCH → VS_AI_RESULT → MENU
```

##### 各阶段处理函数

| 函数 | 行号 | 功能 |
|------|------|------|
| `handleMenu()` | 138 | 主菜单：1.闯关 2.排行榜 3.VS AI 4.退出 |
| `handleLevelSelect()` | ~280 | 关卡列表，显示解锁/最高分 |
| `startNewGame()` | 217 | 初始化：选择 5 个起始格 → 创建 GameState |
| `handleGameplay()` | 370 | **单人模式主循环**：光标移动、合并、滑行、提交 |
| `tryMerge()` / `trySlide()` | 500+ | 执行合并/滑行：验证归属 → 执行 → 更新集合 → 记录 → 动画 |
| `submitAnswer()` | 606 | 结算：停止回放 → 计算得分 → 保存排行榜 → 闯关判定 |
| `handleVSAIMenu()` | 723 | VS AI 难度选择（Normal=随机 / Hard=贪心） |
| `handleVSAIWatch()` | 762 | **VS AI 主循环**：交替回合，检测出局，后台结算 |

##### VS AI 对战核心流程

```
handleVSAIWatch():
  while (双方都未出局):
    1. 玩家回合: processVSAIPlayerAction()
       - 检测无棋可走 → 弹窗 "You have no more valid moves!"
       - 后台静默执行 AI 剩余回合（贪心策略）
       - break 进入结算
    2. AI 回合: executeVSAIAIAction()
       - findBestMove() 选择最优操作
       - 执行并延时渲染
  → finishVSAI() 结算
```

##### 关键设计

| 特性 | 实现 |
|------|------|
| 单人模式 | 光标选择→确认合并，E 键 AI 提示，S 键提交 |
| VS AI 对战 | 双方轮流操作，独立追踪棋子归属，独立步数统计 |
| 玩家无棋可走 | `showMessage()` 弹窗告知 → 后台静默走完 AI → 结算 |
| 投降 | S 键提交 = 认输，直接结算 |
| AI 后台结算 | 临时切换 GREEDY 策略，`_aiDelayMs=0` 无延迟 |
| 触发音效 | `dynamic_cast<SFMLRenderer*>` 运行时检测渲染器类型 |

#### AIPlayer — `include/controller/AIPlayer.hpp` / `src/controller/AIPlayer.cpp`

AI 对手，约 307 行。

##### 两种策略

**RANDOM (普通模式)**
- 40% 纯随机选择所有合法操作
- 60% 按 70/30 权重在 merge/slide 之间选择
- 适合新手玩家，有随机性，不总是最优

**GREEDY (困难模式)**
- 15% 纯随机（混沌因子）
- 85% 优先选 merge，选 `(letter值+number)` 最大的合并
- 无 merge 时随机 slide
- 适合有经验玩家，进攻性强

##### 起手选子策略

`pickStartingCells()` 对所有非空格评分：
```
得分 = 可合并邻居数 × 10 + 非空邻居数
```
选前 5 名，且相互曼哈顿距离 ≥ 3。若数量不足则放宽距离限制。

#### InputHandler — `include/controller/InputHandler.hpp` / `src/controller/InputHandler.cpp`

控制台模式下，将屏幕坐标（行列字符位置）映射到网格坐标。考虑 ASCII 边框、行列标题的偏移量。

---

### 4.4 View 层 — 渲染与交互

#### IRenderer — `include/view/IRenderer.hpp`

**抽象接口**，定义所有渲染器必须实现的方法。

| 纯虚方法 | 用途 |
|----------|------|
| `render(state)` | 渲染游戏主界面 |
| `showMenu()` | 主菜单 |
| `showPuzzleList()` | 题面列表 |
| `showLevelList()` | 关卡列表（含解锁状态/最高分） |
| `showLeaderboard()` | 排行榜 |
| `showResult()` | 单人结果 |
| `showVSResult()` | VS AI 结果对比 |
| `showMessage(msg)` | 弹窗消息，等待按键 |
| `waitForAction()` | 阻塞等待用户操作 |
| `setTurnMessage(msg)` | 设置回合提示文字 |

`UserAction` 枚举定义了所有可能的用户操作（QUIT, CONFIRM, UP, DOWN, SELECT_CELL, SUBMIT, VIEW_LEADERBOARD 等 22 种）。

#### ConsoleRenderer — `include/view/ConsoleRenderer.hpp` / `src/view/ConsoleRenderer.cpp`

控制台 ASCII 渲染，约 589 行。使用 Windows Console API。

| 特性 | 实现 |
|------|------|
| 颜色 | `SetConsoleTextAttribute()` 设置控制台颜色 |
| 光标 | `SetConsoleCursorPosition()` 精准定位 |
| 网格绘制 | ASCII 边框 `+---+`，行列标题 |
| 玩家格子 | 黄色方括号 `[A1]` |
| AI 格子 | 红色尖括号 `<A1>` |
| 空格子 | 灰色点 `.` |
| 键盘输入 | `_getch()` 读取，`parseKey()` 映射 |

#### SFMLRenderer — `include/view/SFMLRenderer.hpp` / `src/view/SFMLRenderer.cpp`

**图形渲染器**，约 1900 行，工程最大文件。由 `#ifdef HAS_GUI` 保护。

| 特性 | 实现细节 |
|------|----------|
| 窗口 | 1400×900, SFML 3, 垂直同步 |
| 网格 | 动态计算格子大小，随窗口缩放自适应 |
| 鼠标操作 | 两段式：点击选中源格 → 点击相邻目标格执行合并 |
| 键盘操作 | 方向键移动光标，Space 选/确认，Esc 取消，F11 全屏 |
| 合并动画 | 幽灵棋子飞移（75% 时间）+ 金色光圈落定（25% 时间），线性插值 |
| 格子着色 | 玩家=黄色边框，AI=红色边框，光标=蓝色，选中=淡黄，可合并目标=绿色 |
| 信息面板 | 右侧显示题面名、步数、计时、双方棋子数、选中信息、操作提示 |
| 按钮菜单 | 所有界面均支持鼠标点击按钮 |
| 字体 | 多路径回退：项目字体 → 系统字体 |
| 坐标转换 | `pixelToGrid()` / `gridToPixel()` |

##### UI 界面枚举

```cpp
enum class UIScreen {
    Menu,           // 主菜单
    PuzzleList,     // 题面选择
    LevelList,      // 关卡选择
    Gameplay,       // 游戏界面
    Leaderboard,    // 排行榜
    Result,         // 单人结果
    VSAIMenu,       // VS AI 难度选择
    VSAResult,      // VS AI 结果对比
    NamePrompt,     // 玩家名输入
    StartSelect,    // 起始格选择
    Message         // 消息弹窗
};
```

#### SoundManager — `include/view/SoundManager.hpp` / `src/view/SoundManager.cpp`

由 `#ifdef HAS_GUI` 保护，SFML Audio 封装。

| 音效 | 触发时机 |
|------|----------|
| Click | 菜单点击、光标移动 |
| Merge | 成功合并格子 |
| Error | 操作失败、关卡未通过 |
| Submit | 提交答案 |
| Result | 显示结果界面 |

支持 5 种音效，`load()` 从 `assets/sounds/` 加载 `.wav` 文件，文件缺失时打印警告不崩溃。

---

## 五、数据流与关键交互

### 5.1 单局游戏数据流

```
用户操作 → waitForAction() → UserAction
    → GameController::tryMerge/trySlide
        → GameState (更新网格、归属集合、步数)
        → MergeRule::canMerge/getMergedCell (验证规则)
        → ReplayBuffer::record (记录操作)
        → SFMLRenderer::triggerMergeAnim (播放动画)
        → SoundManager::play (播放音效)
    → render() 刷新界面
```

### 5.2 VS AI 对战数据流

```
玩家回合:
  用户操作 → processVSAIPlayerAction()
    → doAIMerge/doAISlide
    → 检测无棋可走 → showMessage → 后台 AI 结算

AI 回合:
  executeVSAIAIAction()
    → AIPlayer::findBestMove()
      → findAllValidMoves() 枚举所有合法操作
      → greedyPick() / randomPick() 按策略选择
    → doAIMerge/doAISlide 执行

结算:
  finishVSAI()
    → 双方步数 × 10 计算得分
    → 胜负判定 → Leaderboard::add() 保存
    → showVSResult() 展示结果
```

### 5.3 主循环

```cpp
void GameController::run() {
    while (_running) {
        switch (_phase) {
            case MENU:          handleMenu(); break;
            case PUZZLE_SELECT: handlePuzzleSelect(); break;
            case GAMEPLAY:      handleGameplay(); break;
            case RESULT:        handleResult(); break;
            case LEADERBOARD:   handleLeaderboard(); break;
            case REPLAY:        handleReplay(); break;
            case VS_AI_MENU:    handleVSAIMenu(); break;
            case VS_AI_WATCH:   handleVSAIWatch(); break;
            case VS_AI_RESULT:  handleVSResult(); break;
            case QUIT:          _running = false; break;
        }
    }
}
```

---

## 六、各功能模块对应文件速查

| 功能 | 主要文件 |
|------|----------|
| 格子数据结构 | `Cell.hpp`, `Cell.cpp` |
| 网格管理 | `Grid.hpp`, `Grid.cpp` |
| 合并规则 | `MergeRule.hpp`, `MergeRule.cpp` |
| 游戏状态（棋子归属/步数/BFS检测） | `GameState.hpp`, `GameState.cpp` |
| 操作记录与回放序列化 | `Move.hpp`, `Move.cpp`, `ReplayBuffer.hpp`, `ReplayBuffer.cpp` |
| 主控制器 + 游戏状态机 | `GameController.hpp`, `GameController.cpp` |
| 主菜单 / 闯关模式 | `GameController::handleMenu()`, `handleLevelSelect()` |
| 单人游戏主循环 | `GameController::handleGameplay()` |
| 提交结算 + 闯关分数线 | `GameController::submitAnswer()` |
| VS AI 对战 | `GameController::handleVSAIWatch()`, `AIPlayer.hpp/cpp` |
| AI 策略（随机/贪心） | `AIPlayer.cpp` `randomPick()` / `greedyPick()` |
| 排行榜 | `Leaderboard.hpp/cpp`, `GameController::handleLeaderboard()` |
| 得分计算 | `ScoreRecord.cpp` `calculateScore()` |
| 题库生成与加载 | `PuzzleManager.hpp/cpp` |
| 闯关进度 | `Progress.hpp/cpp` |
| 文件 I/O | `FileManager.hpp/cpp` |
| 控制台 ASCII 渲染 | `ConsoleRenderer.hpp/cpp` |
| SFML 图形渲染 | `SFMLRenderer.hpp/cpp` |
| 合并动画 | `SFMLRenderer.cpp` `triggerMergeAnim()` |
| 鼠标点击处理 | `SFMLRenderer.cpp` `processMouseEvent()` |
| 键盘映射 | `ConsoleRenderer.cpp` `parseKey()`, `SFMLRenderer.cpp` `processKeyEvent()` |
| 音效系统 | `SoundManager.hpp/cpp` |
| 坐标映射 | `InputHandler.hpp/cpp` |
| 程序入口 | `main.cpp` |
| 构建配置 | `CMakeLists.txt` |

---

## 七、设计模式与工程实践

| 模式/实践 | 应用位置 |
|-----------|----------|
| **MVC 架构** | Model 层独立，View 通过接口与 Controller 解耦 |
| **策略模式** | `AIStrategy` 枚举 + `randomPick()`/`greedyPick()` 多态选择 |
| **策略模式** | `IRenderer` 接口 + `ConsoleRenderer`/`SFMLRenderer` 实现 |
| **状态机** | `GamePhase` 枚举驱动 9 阶段流转 |
| **RAII** | 所有资源通过 `std::unique_ptr` 管理，析构自动释放 |
| **纯静态工具类** | `MergeRule` 构造函数 `= delete`，只暴露静态方法 |
| **值对象** | `Cell`, `Move`, `ScoreRecord`, `Puzzle` 等轻量拷贝 |
| **编译期多态** | `#ifdef HAS_GUI` 条件编译 SFML 相关代码 |
| **异常安全** | `main()` 全局 catch `std::exception` |
| **确定性构造** | 题库生成使用固定随机种子，保证跨机器一致性 |
