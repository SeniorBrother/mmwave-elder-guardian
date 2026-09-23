# project/ —— 你们的真实项目工作区

这里是甲、乙两人**正式开发、要交付/烧录/上线**的代码，与 `examples/`（AI 参考脚手架）完全独立。

## 现有内容

- `firmware/common/` —— **共享契约库**，已由 AI 实现并通过 **62 项单元测试**。
  纯 C、与硬件无关，三个固件目标共用。你们可直接复用并按需修改（改动需双方评审 + 过测试）。
- 其余目录（`firmware/stm32f103`、`firmware/esp32s3`、`firmware/esp32`、`host-app`、`hardware`、`tools`）
  为**空占位**，等你们从零建立正式实现。

## 起步建议

1. 参考 `../../examples/ai-scaffold/` 里的脚手架抄写/改写，但**代码落在本目录**。
2. 把示例的 `hardware/`、`tools/` 复制过来当起点。
3. 写各目标 BSP 时，include 指向本目录的 `firmware/common/`。

## 运行 common 层测试

```bash
cd firmware/common/test
bash build_and_test.sh          # WSL/Linux；或 gcc -Wall -Wextra -std=c99 -I.. test_main.c ../*.c -o run_tests && ./run_tests
```

## 分工与任务

- 目录结构权威定义：`../../docs/directory-structure.md`
- 两人分工：`../../docs/collaboration.md`
- 可认领任务：`../../docs/task-board.md`
