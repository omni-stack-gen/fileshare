# 资源体积优化记录

- 日期：2026-04-22
- 目标构建：`cargo build --target riscv64gc-unknown-linux-gnu --release`

## 优化前

- RISC-V release 可执行文件：`32,769,616` bytes
- Slint 实际引用图片资源：约 `28,669,774` bytes
- `assets/images/*.png`：316 个 Pencil 导出图标

主要体积来自 `images/` 中被 `@image-url` 编译进固件的大图：

- `image-import-6.png`：7.5 MB
- `image-import-12.jpg`：7.5 MB
- `image-import-5.png`：6.1 MB
- `image-import.jpg`：6.0 MB

## 已执行优化

1. 合并完全相同的 Pencil PNG 图标。
   - 原始 PNG：316 个
   - 合并后 PNG：71 个
   - 别名表：`assets/images/pencil-icon-aliases.txt`
   - Slint 仍保留 316 处图标引用，但引用落到 71 个唯一文件。

2. 生成大图压缩版本并更新 Slint 引用。
   - 输出目录：`assets/images/optimized/`
   - 生成文件：12 个
   - 被 Slint 实际引用图片资源降为约 `1,112,436` bytes

3. 重新验证。
   - `slint-compiler -o /dev/null ui/app.slint`：PASS
   - `/usr/bin/pencil_to_slint` 全量渲染 43 张截图：PASS
   - `cargo build --target riscv64gc-unknown-linux-gnu --release`：PASS

## 优化后

- RISC-V release 可执行文件：`5,305,936` bytes
- 比优化前减少：约 `27.46 MB`
- 降幅：约 `83.8%`

## SVG 结论

当前环境验证结果：

- SVG 可被 `slint-compiler` 和 `/usr/bin/pencil_to_slint` 加载。
- WebP 当前渲染器不支持，运行时报错：`The image format WebP is not supported`。

本次未批量把 Pencil 图标转换为 SVG，因为合并重复后剩余图标 PNG 总体很小，单个通常只有几百 bytes，SVG XML 对这些图标未必更小。后续如果新增非常简单且可人工确认形状的图标，可以优先使用 SVG；照片/背景仍应使用压缩 JPEG。
