# 密阵突围 (Matrix Breakthrough) — 项目状态

## 最近完成的工作

| 任务 | 改动文件 | 说明 |
|------|---------|------|
| 界面英文化 + ASCII 边框 | `src/view/ConsoleRenderer.cpp` | 全部中文界面/注释改为英文，Unicode 边框改为 `+-|` |
| 修复三道题面相同 | `src/model/PuzzleManager.cpp` | 种子从循环末尾改到循环开头，每轮使用不同种子 |
| 牌面连通性保证 | `src/model/PuzzleManager.cpp` | 删除原来只放 10 对的做法，改为全盘扫描确保每格至少一个可合并邻居 |
| 排行榜分离（闯关 + VS AI） | `src/controller/GameController.cpp` | VS AI 记录写入 `puzzleId=vs_ai`；单人结算显示个人最佳对比 |
| 闯关模式过关分数线 | `src/controller/GameController.cpp` | 第 1 关 ≥10 分、第 2 关 ≥300 分、第 3 关 ≥600 分才解锁下一关 |
| 合并动画黄色闪烁修复 | `src/view/SFMLRenderer.cpp` | 动画 `_rect` 缺 `setSize`，画出来是 320×900 的矩形块，已补回格子尺寸 |
| 音效系统 | `src/view/SFMLRenderer.cpp` 等 | 接入 5 个音效（merge/click/error/submit/result），无 BGM |

## 当前问题

| 问题 | 说明 | 解决方案 |
|------|------|----------|
| 双击 exe 音效不工作 | Windows 将工作目录设为 exe 所在目录（`build/Debug/`），相对路径 `assets/sounds/` 解析失败 | 在项目根目录建 `run.bat`，先 `cd` 到项目根目录再启动 exe；或后续在代码中用 `GetModuleFileNameW` 获取 exe 绝对路径来定位资源 |

## 已实现功能总览

| 功能模块 | 状态 |
|---------|------|
| 核心玩法（Cell/Grid/MergeRule） | ✅ 完成 |
| 控制台渲染 | ✅ 完成 |
| SFML 图形界面 | ✅ 完成 |
| 题库系统（3+ 预置题面） | ✅ 完成 |
| 排行榜 + 文件持久化 | ✅ 完成 |
| 回放系统 | ✅ 完成 |
| 自定义题面 | ✅ 完成 |
| AI 对手 | ✅ 完成 |
| 闯关模式（3 关） | ✅ 完成 |
| 音效系统（5 种音效，无 BGM） | ✅ 完成 |

## 构建命令

```bash
cmake -B build -DENABLE_GUI=ON -DCMAKE_TOOLCHAIN_FILE=/d/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
```

编译产物：`build/Debug/matrix_breakthrough.exe`
