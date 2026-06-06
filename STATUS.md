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

## 剩余功能/待办

- [x] **音效系统** — 5 个音效已接入（无 BGM）：
  - `merge.wav` — 合并棋子时触发
  - `click.wav` — 方向键/鼠标点击菜单时触发
  - `error.wav` — 关卡未解锁/未达分数线时触发
  - `submit.wav` — 提交答案时触发
  - `result.wav` — 显示结果时触发

## 构建命令

```bash
cmake -B build -DENABLE_GUI=ON -DCMAKE_TOOLCHAIN_FILE=/d/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
```

编译产物：`build/Debug/matrix_breakthrough.exe`

## 下次打开终端

告诉 AI："读取 STATUS.md，继续推进音效系统"即可。
