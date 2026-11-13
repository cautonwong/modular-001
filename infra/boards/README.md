# infra/boards

预留目录。board 默认放在 `targets/<product>/board`，因为一块 board 对应一个产品。

只有当第二个产品明确复用同一块 board 时，才在这里建 `infra/boards/<board>/`，并把原产品 target 的 board 提升为共享组件。

现有 `stategrid_single_phase/` 是该规则下的第一个 board 包，同时服务 645、698 和 IDIS 三个产品。它通过 `CMakeLists.txt` 输出 board 对象库和 `board.elf`，构建命令与各目标文件大小见 `docs/platform-layout.md`。
