# 阶段 2 工作报告：题库系统 + 排行榜 + 文件持久化

> 对应架构文档阶段 2（数据持久化）。
> 目标：从控制台最小原型升级为具备完整数据存取能力的程序。

---

## 一、阶段 2 — 数据持久化层（6 个新增类）

架构文档规划的阶段 2 目标：满足 Level 1 全部要求，程序具备完整的题库管理、成绩记录与文件 I/O 能力。

### 1.1 FileManager — 文件 I/O 工具类

封装底层文件操作，所有路径相对于 `data/` 根目录，提供统一错误处理。

| 函数 | 说明 |
|------|------|
| `readAll(path)` | 读取文本文件全部内容，文件不存在时返回空串 |
| `writeAll(path, content)` | 覆盖写入文本文件，父目录不存在时自动创建 |
| `appendLine(path, line)` | 追加一行到文件末尾（用于排行榜即时持久化） |
| `exists(path)` | 检查文件是否存在 |
| `createDir(path)` | 递归创建子目录（如 `puzzles/custom/`、`replays/`） |
| `dataRoot()` | 返回数据根目录 `"data/"` |

**实现位置：** `src/model/FileManager.cpp`

---

### 1.2 Puzzle — 题面类

封装单道题目的完整数据，支持文本文件序列化/反序列化。

| 数据成员 | 说明 |
|---------|------|
| `_id` | 唯一标识符（如 `"builtin_01"`） |
| `_name` | 显示名称（如 `"Beginner Training"`） |
| `_initialGrid` | 初始盘面 Grid 对象 |

| 函数 | 说明 |
|------|------|
| `serialize()` | 序列化为题面文件格式：首行为 `# id name` 注释 + Grid 数据 |
| `deserialize(data, puzzle)` | 从文本反序列化：解析首行元信息 + 调用 Grid::deserialize 重建盘面 |

**文件格式：**
```
# builtin_01 Beginner Training
25 25
X1 Q1 P9 I4 B9 ...（25×25 格子数据）
```

**实现位置：** `src/model/Puzzle.cpp`

---

### 1.3 PuzzleManager — 题库管理类

管理全部题面的加载、检索、添加与文件持久化。

| 函数 | 说明 |
|------|------|
| `loadAll()` | 初始化 3 道内置题面 + 加载 `custom/` 目录下用户自建题面 |
| `all()` | 返回全部题面列表（只读） |
| `findById(id)` | 按 ID 检索题面，用于回放加载 |
| `addPuzzle(puzzle)` | 添加自定义题面，内存 + 文件双重持久化 |
| `reload()` | 从磁盘重新加载全部题面 |

**内置题面生成策略（首次运行自动创建）：**
- 使用固定随机种子 (`srand(42)`) 保证每次生成相同题面
- 25×25 全随机填充字母 A~Z 和数字 0~9
- 随机放置 5 个同字母不同数字的邻格对 + 5 个同数字不同字母的邻格对，确保盘面有解
- 兜底机制：若仍无合法操作则在 (0,0)/(0,1) 强制放置可合并对

| 内置题面 | ID | 大小 |
|---------|-----|------|
| Beginner Training | `builtin_01` | 25×25 |
| Advanced Challenge | `builtin_02` | 25×25 |
| Matrix Master | `builtin_03` | 25×25 |

**实现位置：** `src/model/PuzzleManager.cpp`

---

### 1.4 ScoreRecord — 答题记录类

存储单次答题的完整成绩，自动计算综合得分并支持 CSV 序列化。

**得分公式：** `score = accuracy × 1000 + max(0, 500 − timeSeconds × 2) + steps × 10`

| 数据成员 | 说明 |
|---------|------|
| `_playerName` | 玩家名称 |
| `_puzzleId` | 题面 ID |
| `_timeSeconds` | 答题用时（秒） |
| `_steps` | 完成步数 |
| `_accuracy` | 正确率（0.0 ~ 1.0） |
| `_score` | 综合得分（构造时自动计算） |
| `_timestamp` | 完成时间戳 |

| 函数 | 说明 |
|------|------|
| `serialize()` | 序列化为 CSV 行：`Name, PuzzleID, Time, Steps, Accuracy, Score, Timestamp` |
| `deserialize(line, record)` | 从 CSV 行反序列化 |

**实现位置：** `src/model/ScoreRecord.cpp`

---

### 1.5 Leaderboard — 排行榜类

维护得分记录列表，支持多维度排序与文件持久化。

| 函数 | 说明 |
|------|------|
| `load()` | 启动时从 `data/leaderboard.txt` 加载全部历史记录 |
| `save()` | 覆盖写回文件 |
| `add(record)` | 追加新记录（内存 + 文件追加一行，即时持久化） |
| `topByScore(n)` | 按总分降序，取前 n 名 |
| `topByTime(n)` | 按用时升序，取前 n 名 |
| `topByAccuracy(n)` | 按正确率降序，取前 n 名 |
| `recordsForPuzzle(id)` | 筛选特定题面的全部记录 |

**当前使用：** 排行榜界面调用 `topByScore(15)` 显示前 15 名。

**实现位置：** `src/model/Leaderboard.cpp`

---

### 1.6 Player — 玩家类

轻量级值对象，封装玩家身份信息。

| 数据成员 | 说明 |
|---------|------|
| `_name` | 玩家昵称（由 `promptPlayerName()` 交互输入） |
| `_startingCells` | 玩家选择的 5 个起始格子坐标（备用字段） |

**实现位置：** `src/model/Player.cpp`

---

## 二、GameController 中的阶段 2 集成

### 2.1 初始化流程

GameController 构造时按依赖顺序初始化阶段 2 各模块：
1. `FileManager` — 以 `"data"` 为根目录，自动创建 `puzzles/custom/` 和 `replays/` 子目录
2. `PuzzleManager` — 调用 `loadAll()` 加载 3 道内置题面 + 自定义题面
3. `Leaderboard` — 调用 `load()` 从 `data/leaderboard.txt` 加载历史记录

### 2.2 答题流程中的数据持久化

```
选题 (handlePuzzleSelect)
  └─ PuzzleManager::all() → 展示题面列表
  └─ PuzzleManager 提供选中题面的 initialGrid
       └─ startNewGame() → 初始化 GameState
            └─ 游戏进行中 ...
                 └─ submitAnswer()
                      ├─ 构造 ScoreRecord（含自动计分）
                      ├─ Leaderboard::add(record) → 写入 data/leaderboard.txt
                      └─ 显示结果 → 返回主菜单
```

### 2.3 排行榜查看

```
主菜单 → LEADERBOARD 阶段
  └─ Leaderboard::topByScore(15) → 按总分降序取前 15 名
  └─ 渲染器展示排行榜
  └─ 按任意键返回主菜单
```

### 2.4 文件 I/O 时序

| 操作 | 文件 | 方向 | 时机 |
|------|------|------|------|
| 加载题库 | `data/puzzles/builtin_*.txt` | 读 | 程序启动 |
| 保存自定义题面 | `data/puzzles/custom/*.txt` | 写 | 用户添加新题面 |
| 加载排行榜 | `data/leaderboard.txt` | 读 | 程序启动 |
| 追加成绩 | `data/leaderboard.txt` | 追加写 | 每次提交答案 |

---

## 三、交付物检查清单

### 阶段 2（数据持久化）

- [x] 3 道预置题面，存储为 `data/puzzles/builtin_*.txt`，启动时自动加载
- [x] 题面选择界面（主菜单 → 题面列表 → 确认）
- [x] 计时器：首次操作启动，提交时结算
- [x] 正确率、步数、综合得分自动计算（`ScoreRecord` 得分公式）
- [x] 提交答案后成绩写入 `data/leaderboard.txt`（CSV 格式）
- [x] 排行榜查看功能：按总分降序显示前 15 名
- [x] 程序启动时自动加载题库和排行榜

---

## 四、目录结构现状（阶段 2 相关）

```
E:\c++实训\
├── CMakeLists.txt
├── Architecture_Design.md
├── report2.md                              ← 本报告
├── include/
│   ├── core/
│   │   ├── Cell.hpp          Grid.hpp       GameState.hpp
│   │   ├── MergeRule.hpp     Move.hpp
│   ├── model/
│   │   ├── FileManager.hpp   Player.hpp     Puzzle.hpp
│   │   ├── PuzzleManager.hpp ScoreRecord.hpp
│   │   └── Leaderboard.hpp
│   ├── controller/
│   │   └── GameController.hpp
│   └── view/
│       └── IRenderer.hpp
├── src/  （与 include/ 镜像结构）
├── data/
│   ├── puzzles/
│   │   ├── builtin_01.txt   builtin_02.txt   builtin_03.txt
│   │   └── custom/
│   └── leaderboard.txt
```

---

## 五、与架构文档的对照

| 架构计划（阶段 2） | 状态 | 说明 |
|---------|------|------|
| FileManager — 文件 I/O 工具类 | 已完成 | 封装读写、追加、目录创建 |
| Puzzle — 题面类 | 已完成 | 支持序列化/反序列化 |
| PuzzleManager — 题库管理类 | 已完成 | 内置 3 题 + 自定义题面加载 |
| ScoreRecord — 答题记录类 | 已完成 | 自动计分 + CSV 序列化 |
| Leaderboard — 排行榜类 | 已完成 | 多维度排序 + 文件持久化 |
| Player — 玩家类 | 已完成 | 轻量值对象 |
| 3 道以上预置题面 | 已完成 | 3 道 25×25 题面 |
| 用户选题界面 | 已完成 | PUZZLE_SELECT 阶段 |
| 计时器 | 已完成 | 首次操作启动 |
| 成绩自动计算 | 已完成 | 正确率 + 步数 + 时间 → 总分 |
| 成绩写入文件 | 已完成 | data/leaderboard.txt CSV |
| 排行榜查看 | 已完成 | topByScore(15) |
| 启动自动加载 | 已完成 | 题库 + 排行榜 |

---

