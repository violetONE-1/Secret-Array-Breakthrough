# 阶段 1 工作报告：核心数据结构 + 控制台最小可玩原型

> 目标：纯控制台下可完整游玩一局，验证合并规则正确

---

## 已实现的类及功能

### 1. Cell（格子类）

网格最小数据单元，存储字母、数字及显示状态。

| 函数 | 说明 |
|------|------|
| `Cell(char, int)` | 构造有内容的格子 |
| `makeEmpty()` | 静态工厂，创建空格子 |
| `getLetter()` / `setLetter()` | 字母访问器 |
| `getNumber()` / `setNumber()` | 数字访问器 |
| `isEmpty()` / `setEmpty()` | 空状态访问器 |
| `isSelected()` / `setSelected()` | 选中状态访问器 |
| `sameLetter(Cell&)` | 比较两格字母是否相同（空格子返回 false） |
| `sameNumber(Cell&)` | 比较两格数字是否相同 |
| `toString()` | 转为 3 字符宽度的显示字符串（如 "A1 "、 " . "） |

### 2. Grid（网格类）

管理 rows×cols 二维 Cell 数组，提供网格级操作。

| 函数 | 说明 |
|------|------|
| `Grid(int, int)` | 构造全空格网格 |
| `at(r, c)` | 访问指定格子（带边界检查，const/非 const 重载） |
| `setAt(r, c, Cell)` | 设置指定格子内容 |
| `mergeCells(sr, sc, dr, dc, result)` | 合并两格：源格置空，目标格更新为结果 |
| `findAllValidMoves()` | 扫描全盘，返回所有合法 Move 列表 |
| `hasAnyValidMove()` | 判断是否存在至少一个合法操作（死局检测） |
| `serialize()` | 序列化为文本格式（rows cols + 逐行格子数据） |
| `deserialize(str, grid)` | 从字符串反序列化重建网格 |
| `clear()` | 全部格子重置为空 |

### 3. MergeRule（合并规则类）

纯静态工具类，封装全部合并判定与计算逻辑。

| 函数 | 说明 |
|------|------|
| `canMerge(a, b)` | 判定两格是否可合并：字母相同+数字不同，或数字相同+字母不同 |
| `mergeLetters(a, b)` | 字母循环加法（A+B→C，Z+A→A） |
| `mergeNumbers(a, b)` | 数字加法取个位（9+8→7） |
| `getMergedCell(a, b)` | 返回合并后的新 Cell（不同属性相加，相同属性保留） |

### 4. Move + Direction（操作记录 + 方向枚举）

| 函数/成员 | 说明 |
|------|------|
| `Direction` 枚举 | UP / DOWN / LEFT / RIGHT / NONE |
| `directionToString()` | 方向枚举转字符串 |
| `stringToDirection()` | 字符串解析为方向（不区分大小写） |
| `Move` 结构体 | 字段：srcRow/Col、dstRow/Col、direction、resultLetter/Number |
| `serialize(seqNum)` | 序列化为回放格式 `[Move] 001 3,4 -> 3,5 RIGHT A5` |
| `deserialize(line, move, seq)` | 从回放行反序列化 |

### 5. GameState（游戏状态类）

聚合一局游戏的全部运行时数据。

| 函数 | 说明 |
|------|------|
| `GameState(grid, starts, id)` | 构造初始状态：盘面 + 5 个起始点 + 题面 ID |
| `grid()` | 获取盘面引用（const/非 const） |
| `recordMove(Move)` | 记录一步操作，首次调用时启动计时器 |
| `elapsedSeconds()` | 返回已用秒数（首次操作起计时） |
| `accuracy()` | 返回正确率 |
| `stepsTaken()` | 返回已执行步数 |
| `isOver()` / `endGame()` | 游戏结束状态管理 |

### 6. IRenderer（渲染器抽象接口）

定义所有渲染器必须实现的纯虚接口，支持多态切换。

| 函数 | 说明 |
|------|------|
| `render(GameState&)` | 渲染游戏界面 |
| `showMenu()` | 显示主菜单 |
| `showPuzzleList()` | 显示题面选择列表 |
| `showLeaderboard()` | 显示排行榜 |
| `showResult()` | 显示答题结果 |
| `showMessage(msg)` | 显示临时消息并等待按键 |
| `promptPlayerName()` | 提示输入玩家名 |
| `promptStartingCells()` | 提示选择起始格子 |
| `waitForAction()` | 阻塞等待用户操作，返回 UserAction 枚举 |

### 7. ConsoleRenderer（终端渲染器）

继承 IRenderer，在 Windows 控制台中用 ASCII 字符绘制 25×25 网格。

核心实现：`drawGrid()` 绘制带行列标号的网格框架；`parseKey()` 将 `_getch()` 按键映射为 `UserAction`；`setColor()` 控制文字颜色实现高亮选中格。

### 8. InputHandler（输入处理器）

坐标转换工具类。

| 函数 | 说明 |
|------|------|
| `screenToGrid(r, c)` | 屏幕光标位置 → 网格坐标 |
| `gridToScreen(r, c)` | 网格坐标 → 屏幕光标位置 |
| `isInGrid(r, c)` | 判断屏幕坐标是否在网格范围内 |

### 9. GameController（主控制器）

游戏中枢，持有全部 Model/View 对象（unique_ptr），编排完整生命周期。

| 函数 | 说明 |
|------|------|
| `run()` | 主循环：MENU → PUZZLE_SELECT → GAMEPLAY → RESULT → LEADERBOARD |
| `handleMenu()` | 菜单：开始游戏 / 排行榜 / 退出 |
| `handlePuzzleSelect()` | 题面列表选择，输入玩家名 |
| `startNewGame(Puzzle)` | 初始化 GameState + ReplayBuffer，设置 5 个默认起点 |
| `handleGameplay()` | 方向键移动光标，空格/回车执行合并，S 提交，死局自动提交 |
| `tryMerge()` | 校验 → 合并 → 记录 Move → 写入回放缓冲区 |
| `submitAnswer()` | 结算成绩 → 更新排行榜 → 保存回放文件 → 显示结果 |
| `handleLeaderboard()` | 读取并显示前 15 名 |

---

## 交付物检查

- [x] Cell / Grid / MergeRule / Move / GameState 完成
- [x] 25×25 网格的控制台 ASCII 绘制（ConsoleRenderer）
- [x] 键盘方向键选择格子、空格/回车执行合并
- [x] 合并规则正确：同字母+不同数字→数字相加取个位；同数字+不同字母→字母循环相加
- [x] 3 道预置题面（data/puzzles/builtin_*.txt）+ 题面选择菜单
- [x] 玩家选择 5 个起始点
- [x] 盘面更新、死局判断、胜负提交
- [x] 计时器、步数统计、正确率计算
- [x] 排行榜读写、答题结果展示
- [x] 回放录制与保存

---

